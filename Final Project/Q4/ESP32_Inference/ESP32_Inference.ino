#include "esp_camera.h"
#include "img_converters.h" // For fmt2jpg
#include <WiFi.h>
#include <string.h>

// ArduTFLite Library
#include <ArduTFLite.h>

#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>

// Model Headers
#include "mobilenet_model.h"
#include "resnet_model.h"
#include "squeezenet1_model.h"

// Prediction Struct Definition
struct Prediction {
  int digit;
  float confidence;
  float all_probs[10];
};

// WiFi Credentials
const char *ssid = "ID";
const char *password = "PW";

// Camera Pins (AI-Thinker)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// TFLite Globals
const int kArenaSize = 600 * 1024; // 600KB in PSRAM
uint8_t *tensor_arena = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
const tflite::Model *model = nullptr;
tflite::AllOpsResolver resolver;

TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;

WiFiServer server(80);

// Global Buffer for JPEG Image
uint8_t *last_image_buffer = nullptr;
size_t last_image_len = 0;
const size_t kImageBufferSize = 40 * 1024; // 40KB

void setupPSRAM() {
  if (psramInit()) {
    Serial.println("PSRAM Initialized");
    tensor_arena = (uint8_t *)heap_caps_malloc(kArenaSize, MALLOC_CAP_SPIRAM);
    if (tensor_arena == nullptr) {
      Serial.println("ERR: Failed to allocate tensor arena in PSRAM!");
    } else {
      Serial.printf("Allocated %d bytes in PSRAM for Tensor Arena\n",
                    kArenaSize);
    }

    // Allocate buffer for JPEG image
    last_image_buffer =
        (uint8_t *)heap_caps_malloc(kImageBufferSize, MALLOC_CAP_SPIRAM);
    if (last_image_buffer == nullptr) {
      Serial.println("ERR: Failed to allocate image buffer in PSRAM!");
    } else {
      Serial.printf("Allocated %d bytes in PSRAM for Image Buffer\n",
                    kImageBufferSize);
    }

  } else {
    Serial.println("ERR: PSRAM not available!");
  }
}

void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;   // 320x240
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera Init Success");
}

// Model Loading Logic
bool loadModel(const unsigned char *model_data) {
  if (interpreter != nullptr) {
    delete interpreter;
    interpreter = nullptr;
  }

  model = tflite::GetModel(model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println(
        "Model provided is schema version is not equal to supported version!");
    return false;
  }

  interpreter = new tflite::MicroInterpreter(model, resolver, tensor_arena,
                                             kArenaSize, nullptr);

  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("AllocateTensors() failed");
    return false;
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  return true;
}

void preprocessImage() {
  if (last_image_buffer == nullptr)
    return;

  // QVGA Fixed size
  int w = 320;
  int h = 240;
  size_t rgb_len = w * h * 2;

  uint8_t *rgb_buf = (uint8_t *)heap_caps_malloc(rgb_len, MALLOC_CAP_SPIRAM);
  if (!rgb_buf) {
    Serial.println("ERR: OOM for RGB Buffer");
    return;
  }

  bool decoded =
      jpg2rgb565(last_image_buffer, last_image_len, rgb_buf, JPG_SCALE_NONE);
  if (!decoded) {
    Serial.println("JPEG Decode Failed");
    heap_caps_free(rgb_buf);
    return;
  }

  int target_w = 64;
  int target_h = 64;

  // Center Crop: 240x240
  int crop_size = (w < h) ? w : h;   // 240
  int start_x = (w - crop_size) / 2; // (320-240)/2 = 80/2 = 40
  int start_y = (h - crop_size) / 2; // 0

  float ratio = (float)crop_size / target_w; // 240 / 64 = 3.75

  for (int y = 0; y < target_h; y++) {
    for (int x = 0; x < target_w; x++) {
      // Map target pixel to source pixel (with crop offset)
      int px = start_x + (int)(x * ratio);
      int py = start_y + (int)(y * ratio);

      // Clamp coordinates just in case
      if (px >= w)
        px = w - 1;
      if (py >= h)
        py = h - 1;

      // Index in RGB565 buffer (2 bytes per pixel)
      int idx = (py * w + px) * 2;

      uint8_t low = rgb_buf[idx];
      uint8_t high = rgb_buf[idx + 1];
      uint16_t pixel = (high << 8) | low;

      // Extract RGB
      uint8_t r = (pixel >> 11) & 0x1F;
      uint8_t g = (pixel >> 5) & 0x3F;
      uint8_t b = pixel & 0x1F;

      // Expand to 8-bit
      r = (r * 255) / 31;
      g = (g * 255) / 63;
      b = (b * 255) / 31;

      // Grayscale
      uint8_t gray = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);

      // INVERT COLORS: MNIST is White-on-Black, but camera sees Black-on-White
      gray = 255 - gray;

      // CONTRAST STRETCHING: Close the gap between paper and ink
      if (gray < 80) {
        gray = 0; // Clean black background
      } else {
        // Stretch the remaining range (80..255) to (0..255)
        // Scale factor: 255 / (255 - 80) ~= 1.45
        int stretched = (gray - 80) * 1.5;
        if (stretched > 255)
          stretched = 255;
        gray = (uint8_t)stretched;
      }

      // Force outer edges to Black to remove sensor noise/artifacts
      if (x < 4 || x >= (target_w - 4) || y < 4 || y >= (target_h - 4)) {
        gray = 0;
      }

      // Normalize: [0, 255] -> [-128, 127]
      int8_t quant_pixel = (int8_t)((int)gray - 128);

      input->data.int8[y * target_w + x] = quant_pixel;
    }
  }

  heap_caps_free(rgb_buf);

  // DEBUG: ASCII Art Visualization
  Serial.println("\n--- What the AI Sees (64x64) ---");
  for (int y = 0; y < target_h; y += 2) {
    for (int x = 0; x < target_w; x++) {
      int8_t val = input->data.int8[y * target_w + x];
      // -128 is Black, 127 is White
      if (val > 60)
        Serial.print("@"); // Bright White
      else if (val > 0)
        Serial.print("%"); // White
      else if (val > -60)
        Serial.print("."); // Grey
      else
        Serial.print(" "); // Black
    }
    Serial.println();
  }
  Serial.println("--------------------------------");
}

Prediction predict(const unsigned char *model_data, const char *name) {
  if (last_image_buffer == nullptr) {
    Serial.println("No image captured!");
    Prediction p;
    p.digit = -1;
    p.confidence = 0;
    return p;
  }

  Serial.printf("Loading %s...\n", name);
  if (!loadModel(model_data)) {
    Serial.println("Model Load Failed");
    Prediction p;
    p.digit = -1;
    p.confidence = 0;
    return p;
  }

  preprocessImage();

  long start = millis();
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("Invoke failed");
    Prediction p;
    p.digit = -1;
    p.confidence = 0;
    return p;
  }
  long end = millis();
  Serial.printf("Inference time: %d ms\n", (end - start));

  // Process Output
  TfLiteTensor *output_tensor = interpreter->output(0);
  float scale = output_tensor->params.scale;
  int zero_point = output_tensor->params.zero_point;

  float max_conf = -1.0;
  int max_idx = -1;
  float probs[10];

  Serial.print("Scores: ");
  for (int i = 0; i < 10; i++) {
    int8_t val = output_tensor->data.int8[i];
    float real_val = (val - zero_point) * scale;
    probs[i] = real_val;
    Serial.printf("%d:%.2f ", i, real_val);

    if (real_val > max_conf) {
      max_conf = real_val;
      max_idx = i;
    }
  }
  Serial.println();

  Prediction p;
  p.digit = max_idx;
  p.confidence = max_conf;
  memcpy(p.all_probs, probs, sizeof(probs));
  return p;
}

void handleRoot(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");
  client.println("<!DOCTYPE HTML><html><head><meta name='viewport' "
                 "content='width=device-width, initial-scale=1'></head>");
  client.println(
      "<style>body{text-align:center; font-family:sans-serif;} "
      "button{padding:15px; margin:10px; font-size:18px;} img{margin:10px; "
      "border:1px solid #ddd; max-width:100%;}</style>");
  client.println("<body><h1>ESP32 Multi Model Handwritten Digit Recognition</h1>");

  // Image with Timestamp to force refresh
  client.println(
      "<img id='preview' src='/view?t=0' width='320' height='240'><br>");

  client.println("<button onclick='doCapture()'>1. Capture Photo</button><br>");

  client.println("<div id='controls' style='display:block;'>");
  client.println("<a href='/predict?model=mobilenet'><button>Predict "
                 "MobileNet</button></a>");
  client.println(
      "<a href='/predict?model=resnet'><button>Predict ResNet</button></a>");
  client.println("<a href='/predict?model=squeezenet'><button>Predict "
                 "SqueezeNet</button></a>");
  client.println("<a href='/predict?model=merged'><button "
                 "style='background-color:#4CAF50;color:white;'>Predict "
                 "MERGED</button></a>");
  client.println("</div>");

  client.println("<script>");
  client.println("function doCapture() {");
  client.println("  fetch('/capture').then(response => {");
  client.println("     if (response.ok) {");
  client.println("        document.getElementById('preview').src = '/view?t=' "
                 "+ new Date().getTime();");
  client.println("     } else { alert('Capture Failed'); }");
  client.println("  });");
  client.println("}");
  client.println("</script>");

  client.println("</body></html>");
}

void setup() {
  Serial.begin(115200);
  setupPSRAM();
  initCamera();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String req = client.readStringUntil('\r');
    client.flush();

    if (req.indexOf("GET / ") != -1) {
      handleRoot(client);
    } else if (req.indexOf("GET /capture") != -1) {
      // Capture Workflow
      camera_fb_t *fb = NULL;
      for (int i = 0; i < 2; i++) {
        fb = esp_camera_fb_get();
        if (fb)
          esp_camera_fb_return(fb);
        fb = NULL;
      }

      // Actual Capture
      fb = esp_camera_fb_get();
      if (!fb) {
        client.println(
            "HTTP/1.1 500 Internal Server Error\r\n\r\nCapture Failed");
      } else {
        if (last_image_buffer) {
          // Copy JPEG to PSRAM buffer
          size_t to_copy =
              (fb->len < kImageBufferSize) ? fb->len : kImageBufferSize;
          memcpy(last_image_buffer, fb->buf, to_copy);
          last_image_len = to_copy; // Update actual length
          client.println(
              "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nCaptured");
        } else {
          client.println("HTTP/1.1 500 Internal Server Error\r\n\r\nNo Memory");
        }
        esp_camera_fb_return(fb);
      }
    } else if (req.indexOf("GET /view") != -1) {
      // View Workflow: Serve JPEG Buffer Directly
      if (last_image_buffer && last_image_len > 0) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: image/jpeg");
        client.println("Content-Length: " + String(last_image_len));
        client.println();
        client.write(last_image_buffer, last_image_len);
      } else {
        client.println("HTTP/1.1 404 Not Found\r\n\r\nNo Image");
      }
    } else if (req.indexOf("GET /predict") != -1) {
      if (last_image_buffer == nullptr) {
        client.println(
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>No Image "
            "Captured! Capture First.</h1><a href='/'>Back</a>");
        client.stop();
        return;
      }

      String resultHtml = "HTTP/1.1 200 OK\r\nContent-Type: "
                          "text/html\r\n\r\n<h1>Prediction Results</h1>";
      // Re-display image
      resultHtml += "<img src='/view?t=999' width='160'><br>";

      if (req.indexOf("model=merged") != -1) {
        Prediction p1 = predict(mobilenet_model_model, "MobileNet");
        Prediction p2 = predict(resnet_model_model, "ResNet");
        Prediction p3 = predict(squeezenet1_model_model, "SqueezeNet");

        float final_probs[10] = {0};
        float max_p = -1;
        int winner = -1;

        resultHtml += "<h3>Individual Scored</h3><ul>";
        resultHtml += "<li>MobileNet: Digit " + String(p1.digit) + " (" +
                      String(p1.confidence) + ")</li>";
        resultHtml += "<li>ResNet: Digit " + String(p2.digit) + " (" +
                      String(p2.confidence) + ")</li>";
        resultHtml += "<li>SqueezeNet: Digit " + String(p3.digit) + " (" +
                      String(p3.confidence) + ")</li></ul>";

        for (int i = 0; i < 10; i++) {
          final_probs[i] =
              (p1.all_probs[i] + p2.all_probs[i] + p3.all_probs[i]) / 3.0;
          if (final_probs[i] > max_p) {
            max_p = final_probs[i];
            winner = i;
          }
        }
        resultHtml +=
            "<h2>FINAL MERGED RESULT: Digit " + String(winner) + "</h2>";
        resultHtml += "<h3>Confidence: " + String(max_p) + "</h3>";

      } else if (req.indexOf("model=mobilenet") != -1) {
        Prediction p = predict(mobilenet_model_model, "MobileNet");
        resultHtml += "<h2>MobileNet: Digit " + String(p.digit) +
                      "</h2>Conf: " + String(p.confidence);
      } else if (req.indexOf("model=resnet") != -1) {
        Prediction p = predict(resnet_model_model, "ResNet");
        resultHtml += "<h2>ResNet: Digit " + String(p.digit) +
                      "</h2>Conf: " + String(p.confidence);
      } else if (req.indexOf("model=squeezenet") != -1) {
        Prediction p = predict(squeezenet1_model_model, "SqueezeNet");
        resultHtml += "<h2>SqueezeNet: Digit " + String(p.digit) +
                      "</h2>Conf: " + String(p.confidence);
      }
      resultHtml += "<br><a href='/'>Back</a>";
      client.print(resultHtml);
    }
    client.stop();
  }
}
