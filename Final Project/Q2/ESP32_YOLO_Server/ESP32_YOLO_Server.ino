#include "esp_camera.h"
#include "img_converters.h"
#include <WebServer.h>
#include <WiFi.h>

// === TensorFlow Lite Micro ===
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <tflm_esp32.h>

// Model Header
#include "model_data.h"

// === WiFi Credentials ===
const char *ssid = "ID";
const char *password = "PW";

// === Camera Pins (AI-Thinker) ===
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
#define FLASH_LED_PIN 4

// === Model Config ===
#define IMG_W 128
#define IMG_H 128
#define GRID 8
#define CLASS_COUNT 10
#define TOTAL_PER_CELL 15
#define CONF_THRESHOLD 0.5

// Flash Control
bool useFlash = false;

// === TFLite Globals ===
#define ARENA_SIZE (1024 * 1024 * 2)
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
tflite::MicroMutableOpResolver<10> *resolver = nullptr;
uint8_t *tensor_arena = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;

WebServer server(80);

// === Detection Result Storage ===
struct Detection {
  int cls;
  float conf;
  float cx, cy, w, h;
};
Detection detections[20];
int detection_count = 0;

// === Image Buffers ===
uint8_t *captured_gray = nullptr; // Raw grayscale 128x128
uint8_t *jpeg_buf = nullptr;      // JPEG output
size_t jpeg_len = 0;
bool hasImage = false;

// ============================================
// CAMERA INIT
// ============================================
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
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QCIF; // 176x144
  config.jpeg_quality = 10;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera Init Failed!");
    return;
  }
  Serial.println("Camera Initialized.");
}

// ============================================
// TFLITE INIT
// ============================================
void initTFLite() {
  if (!psramInit()) {
    Serial.println("PSRAM Init Failed!");
    return;
  }
  Serial.printf("PSRAM OK. Free: %d bytes\n", ESP.getFreePsram());

  tensor_arena = (uint8_t *)ps_malloc(ARENA_SIZE);
  if (!tensor_arena) {
    Serial.println("Arena Allocation Failed!");
    return;
  }

  model = tflite::GetModel(model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model Version Mismatch!");
    return;
  }

  resolver = new tflite::MicroMutableOpResolver<10>();
  resolver->AddConv2D();
  resolver->AddMaxPool2D();
  resolver->AddRelu();
  resolver->AddReshape();
  resolver->AddTranspose();
  resolver->AddAdd();
  resolver->AddLogistic();
  resolver->AddMean();
  resolver->AddQuantize();
  resolver->AddDequantize();

  interpreter = new tflite::MicroInterpreter(model, *resolver, tensor_arena,
                                             ARENA_SIZE, nullptr, nullptr);

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("AllocateTensors Failed!");
    return;
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  Serial.println("TFLite Ready.");
  Serial.printf("Input: %dx%dx%d\n", input->dims->data[1], input->dims->data[2],
                input->dims->data[3]);
}

// ============================================
// CAPTURE IMAGE (Stores grayscale + creates JPEG)
// ============================================
void captureImage() {
  // Flush stale frames from camera buffer (fixes "old image" issue)
  camera_fb_t *fb = nullptr;
  for (int i = 0; i < 3; i++) {
    fb = esp_camera_fb_get();
    if (fb)
      esp_camera_fb_return(fb);
  }

  // Flash with proper timing
  if (useFlash) {
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(200); // Give camera time to adjust to light
  }

  // Actual capture
  fb = esp_camera_fb_get();

  // Turn off flash after capture
  if (useFlash) {
    digitalWrite(FLASH_LED_PIN, LOW);
  }

  if (!fb) {
    Serial.println("Capture Failed!");
    return;
  }

  // Allocate grayscale buffer
  if (!captured_gray) {
    captured_gray = (uint8_t *)ps_malloc(IMG_W * IMG_H);
  }

  // Crop center 128x128 from 176x144
  int start_x = 24;
  int start_y = 8;
  int raw_width = 176;

  for (int y = 0; y < IMG_H; y++) {
    for (int x = 0; x < IMG_W; x++) {
      int idx = (start_y + y) * raw_width + (start_x + x);
      captured_gray[y * IMG_W + x] = fb->buf[idx];
    }
  }

  esp_camera_fb_return(fb);

  // Convert grayscale to JPEG for fast preview
  if (jpeg_buf) {
    free(jpeg_buf);
    jpeg_buf = nullptr;
  }

  bool ok = fmt2jpg(captured_gray, IMG_W * IMG_H, IMG_W, IMG_H,
                    PIXFORMAT_GRAYSCALE, 80, &jpeg_buf, &jpeg_len);

  if (ok) {
    Serial.printf("JPEG created: %d bytes\n", jpeg_len);
  } else {
    Serial.println("JPEG conversion failed!");
  }

  hasImage = true;
  Serial.println("Image Captured.");
}

// ============================================
// RUN INFERENCE
// ============================================
void runInference() {
  if (!hasImage || !captured_gray) {
    Serial.println("No image to process!");
    return;
  }

  detection_count = 0;

  // Auto-Contrast Normalization
  float min_val = 255.0;
  float max_val = 0.0;
  for (int i = 0; i < IMG_W * IMG_H; i++) {
    float val = captured_gray[i];
    if (val < min_val)
      min_val = val;
    if (val > max_val)
      max_val = val;
  }
  float range = max_val - min_val;
  if (range < 1.0)
    range = 1.0;

  Serial.printf("Input Stats - Min: %.0f, Max: %.0f\n", min_val, max_val);

  // Normalize and Fill Input Tensor
  for (int i = 0; i < IMG_W * IMG_H; i++) {
    float norm = (captured_gray[i] - min_val) / range;
    input->data.f[i] = norm;
  }

  // Run Inference
  uint32_t t = millis();
  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("Invoke Failed!");
    return;
  }
  Serial.printf("Inference Time: %d ms\n", millis() - t);

  // Parse Detections
  for (int i = 0; i < GRID; i++) {
    for (int j = 0; j < GRID; j++) {
      int idx = (i * GRID + j) * TOTAL_PER_CELL;
      float conf = output->data.f[idx + 0];

      if (conf > CONF_THRESHOLD) {
        float max_score = 0;
        int cls = -1;
        for (int c = 0; c < CLASS_COUNT; c++) {
          float score = output->data.f[idx + 5 + c];
          if (score > max_score) {
            max_score = score;
            cls = c;
          }
        }

        float cx = (j + output->data.f[idx + 1]) / GRID;
        float cy = (i + output->data.f[idx + 2]) / GRID;
        float w = output->data.f[idx + 3];
        float h = output->data.f[idx + 4];

        if (detection_count < 20) {
          detections[detection_count].cls = cls;
          detections[detection_count].conf = conf;
          detections[detection_count].cx = cx;
          detections[detection_count].cy = cy;
          detections[detection_count].w = w;
          detections[detection_count].h = h;
          detection_count++;
        }

        // Output format: [ID] digit=X score=0.YY box=(x1,y1,x2,y2) in pixels
        int x1 = (int)((cx - w / 2) * IMG_W);
        int y1 = (int)((cy - h / 2) * IMG_H);
        int x2 = (int)((cx + w / 2) * IMG_W);
        int y2 = (int)((cy + h / 2) * IMG_H);
        Serial.printf("[%d] digit=%d score=%.2f box=(%d,%d,%d,%d)\n",
                      detection_count, cls, conf, x1, y1, x2, y2);
      }
    }
  }

  Serial.printf(">> Total detections: %d\n", detection_count);
}

// ============================================
// WEB HANDLERS
// ============================================

void handleRoot() {
  String flashState = useFlash ? "ON" : "OFF";
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    body { font-family: sans-serif; text-align: center; background: #1a1a2e; color: #eee; }
    h1 { color: #0f0; }
    #container { display: inline-block; position: relative; margin: 20px; }
    #preview { border: 2px solid #0f0; background: #000; max-width: 100%; }
    #boxes { position: absolute; top: 0; left: 0; pointer-events: none; }
    button { padding: 12px 24px; margin: 8px; font-size: 16px; cursor: pointer; 
             background: #0f0; color: #000; border: none; border-radius: 8px; }
    button:hover { background: #0a0; }
    .flash-btn { background: )rawliteral" +
                String(useFlash ? "#ff0" : "#666") + R"rawliteral(; }
    #info { margin: 20px; font-size: 16px; min-height: 50px; }
  </style>
</head>
<body>
  <h1>YOLO Digit Detector</h1>
  <div id='container'>
    <img id='preview' src='/view' width='256' height='256'>
    <svg id='boxes' width='256' height='256'></svg>
  </div>
  <br>
  <button onclick='capture()'>Capture</button>
  <button onclick='predict()'>Predict</button>
  <button class='flash-btn' onclick='toggleFlash()'>Flash: )rawliteral" +
                flashState + R"rawliteral(</button>
  <div id='info'>Press Capture to take a photo</div>
  
  <script>
    function capture() {
      document.getElementById('info').innerHTML = 'Capturing...';
      document.getElementById('boxes').innerHTML = '';
      fetch('/capture').then(r => r.text()).then(t => {
        document.getElementById('info').innerHTML = t;
        document.getElementById('preview').src = '/view?t=' + Date.now();
      });
    }
    
    function predict() {
      document.getElementById('info').innerHTML = 'Running inference...';
      fetch('/predict').then(r => r.json()).then(data => {
        var svg = '';
        var info = '';
        var colors = ['#ff0000','#00ff00','#0088ff','#ff8800','#ff00ff',
                      '#00ffff','#ffff00','#ff0088','#88ff00','#8800ff'];
        data.detections.forEach(function(d) {
          var x1 = (d.cx - d.w/2) * 256;
          var y1 = (d.cy - d.h/2) * 256;
          var bw = d.w * 256;
          var bh = d.h * 256;
          var c = colors[d.cls % 10];
          svg += '<rect x="'+x1+'" y="'+y1+'" width="'+bw+'" height="'+bh+'" stroke="'+c+'" fill="none" stroke-width="3"/>';
          svg += '<text x="'+x1+'" y="'+(y1-5)+'" fill="'+c+'" font-size="16" font-weight="bold">'+d.cls+' ('+Math.round(d.conf*100)+'%)</text>';
          info += 'Digit '+d.cls+': '+Math.round(d.conf*100)+'%<br>';
        });
        document.getElementById('boxes').innerHTML = svg;
        document.getElementById('info').innerHTML = info || 'No detections';
      });
    }
    
    function toggleFlash() {
      fetch('/flash').then(() => location.reload());
    }
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleCapture() {
  captureImage();
  server.send(200, "text/plain", "Captured! Click Predict to run inference.");
}

void handleView() {
  if (hasImage && jpeg_buf && jpeg_len > 0) {
    WiFiClient client = server.client();

    // Send HTTP headers manually
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: image/jpeg");
    client.println("Content-Length: " + String(jpeg_len));
    client.println("Connection: close");
    client.println();

    // Send JPEG data
    client.write(jpeg_buf, jpeg_len);
  } else {
    server.send(404, "text/plain", "No Image");
  }
}

void handlePredict() {
  runInference();

  String json = "{\"detections\":[";
  for (int i = 0; i < detection_count; i++) {
    if (i > 0)
      json += ",";
    json += "{\"cls\":" + String(detections[i].cls);
    json += ",\"conf\":" + String(detections[i].conf, 2);
    json += ",\"cx\":" + String(detections[i].cx, 3);
    json += ",\"cy\":" + String(detections[i].cy, 3);
    json += ",\"w\":" + String(detections[i].w, 3);
    json += ",\"h\":" + String(detections[i].h, 3) + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleFlash() {
  useFlash = !useFlash;
  Serial.printf("Flash: %s\n", useFlash ? "ON" : "OFF");
  server.send(200, "text/plain", useFlash ? "ON" : "OFF");
}

// ============================================
// SETUP & LOOP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== YOLO Digit Detector ===");

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  initCamera();
  initTFLite();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.on("/view", handleView);
  server.on("/predict", handlePredict);
  server.on("/flash", handleFlash);
  server.begin();

  Serial.println("Server Started.");
}

void loop() { server.handleClient(); }
