#include "esp_camera.h"
#include "img_converters.h"
#include <WebServer.h>
#include <WiFi.h>

// Camera pins
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

// WiFi credentials
const char *ssid = "ID";
const char *password = "PW";

// Algorithm parameters
const int TARGET_PIXELS = 1000;
const int TOLERANCE = 50;
const int MAX_ITERATIONS = 16;

// Image buffers stored in PSRAM
uint8_t *originalJpeg = NULL;
size_t originalJpegLen = 0;
uint8_t *originalFrame = NULL;
uint8_t *grayscaleBuffer = NULL;
uint8_t *binaryBuffer = NULL;
int imageWidth = 0;
int imageHeight = 0;
int imageSize = 0;

// Results
uint8_t calculatedThreshold = 128;
int finalPixelCount = 0;

WebServer server(80);

// Converts RGB565 format to grayscale
void convertToGrayscale(uint8_t *rgb565, uint8_t *gray, int width, int height) {
  int totalPixels = width * height;
  for (int i = 0; i < totalPixels; i++) {
    // RGB565 is 16 bits, stored in little endian
    uint16_t pixel = (rgb565[i * 2 + 1] << 8) | rgb565[i * 2];

    // Extract RGB components
    uint8_t r = (pixel >> 11) & 0x1F;
    uint8_t g = (pixel >> 5) & 0x3F;
    uint8_t b = pixel & 0x1F;

    // Scale to 8-bit range
    r = (r * 255) / 31;
    g = (g * 255) / 63;
    b = (b * 255) / 31;

    // Apply grayscale formula: Y = 0.299R + 0.587G + 0.114B
    gray[i] = (uint8_t)((r * 77 + g * 150 + b * 29) >> 8);
  }
}

// Applies threshold and counts resulting white pixels
int applyThresholdAndCount(uint8_t *gray, uint8_t *binary, int size,
                           uint8_t threshold) {
  int count = 0;
  for (int i = 0; i < size; i++) {
    if (gray[i] >= threshold) {
      binary[i] = 255;
      count++;
    } else {
      binary[i] = 0;
    }
  }
  return count;
}

// Binary search to find threshold that gives approximately target pixel count
uint8_t findOptimalThreshold(uint8_t *gray, uint8_t *binary, int size,
                             int targetPixels, int tolerance,
                             int maxIterations) {
  int low = 0;
  int high = 255;
  uint8_t bestThreshold = 128;
  int bestError = size;

  for (int iter = 0; iter < maxIterations; iter++) {
    int mid = (low + high) / 2;
    int count = applyThresholdAndCount(gray, binary, size, (uint8_t)mid);
    int error = abs(count - targetPixels);

    Serial.printf("Iter %d: T=%d, Count=%d, Error=%d\n", iter, mid, count,
                  error);

    // Track best result so far
    if (error < bestError) {
      bestError = error;
      bestThreshold = (uint8_t)mid;
    }

    // Check if within tolerance
    if (error <= tolerance) {
      Serial.printf("Found threshold %d within tolerance\n", mid);
      return (uint8_t)mid;
    }

    // Adjust search range
    if (count > targetPixels) {
      low = mid + 1; // Too many white pixels, increase threshold
    } else {
      high = mid - 1; // Too few white pixels, decrease threshold
    }

    if (low > high)
      break;
  }

  Serial.printf("Best threshold: %d with error: %d\n", bestThreshold,
                bestError);
  return bestThreshold;
}

// HTML for web interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Adaptive Threshold</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; text-align: center; background: #eee; padding: 20px; }
    .container { max-width: 800px; margin: auto; background: white; padding: 20px; border-radius: 8px; }
    button { padding: 10px 20px; font-size: 16px; margin: 10px; cursor: pointer; }
    input, select { padding: 8px; font-size: 16px; width: 80px; text-align: center; }
    img { max-width: 100%; border: 1px solid #ddd; margin-top: 5px; }
    .row { display: flex; flex-wrap: wrap; justify-content: space-around; }
    .col { flex: 1; min-width: 300px; margin: 10px; }
  </style>
</head>
<body>
<div class="container">
  <h1>Adaptive Thresholding</h1>
  <div>
    <label>Target Pixels:</label>
    <input type="number" id="target" value="1000" min="1" max="100000">
    <small>(result may vary slightly)</small>
  </div>
  <hr>
  <div>
    <button onclick="capture()">Capture New Image</button>
    <button onclick="threshold()">Apply Threshold</button>
  </div>
  <p id="status" style="color: blue; font-weight: bold;">Ready</p>
  <div class="row">
    <div class="col"><h3>Original</h3><img id="origImg" src=""></div>
    <div class="col"><h3>Binary Result</h3><img id="binImg" src=""></div>
  </div>
</div>
<script>
  function status(msg) { document.getElementById('status').innerText = msg; }
  function capture() {
    status("Capturing...");
    fetch('/capture').then(r => {
      if(r.ok) { status("Captured"); document.getElementById('origImg').src = "/view?t=orig&ts=" + Date.now(); }
      else status("Capture Failed");
    });
  }
  function threshold() {
    status("Processing...");
    let target = document.getElementById('target').value;
    fetch('/threshold?target=' + target).then(r => {
      if(r.ok) {
        r.text().then(t => { status(t); });
        document.getElementById('binImg').src = "/view?t=bin&ts=" + Date.now();
      }
      else r.text().then(t => status("Error: " + t));
    });
  }
</script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32-CAM Adaptive Threshold");

  // Camera configuration
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
  config.pixel_format = PIXFORMAT_JPEG; // JPEG helps reduce noise
  config.frame_size = FRAMESIZE_QVGA;   // 320x240
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // Initialize camera
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed!");
    return;
  }
  Serial.println("Camera initialized");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Serve main page
  server.on("/", []() { server.send_P(200, "text/html", index_html); });

  // Capture endpoint - takes a new image
  server.on("/capture", []() {
    camera_fb_t *fb = NULL;

    // Flush old frames from buffer
    for (int i = 0; i < 2; i++) {
      fb = esp_camera_fb_get();
      esp_camera_fb_return(fb);
      fb = NULL;
    }

    // Get fresh frame
    fb = esp_camera_fb_get();
    if (!fb) {
      server.send(500, "text/plain", "Camera failed");
      return;
    }

    // Store original JPEG for display
    if (originalJpeg)
      heap_caps_free(originalJpeg);
    originalJpeg = (uint8_t *)heap_caps_malloc(fb->len, MALLOC_CAP_SPIRAM);
    if (originalJpeg) {
      memcpy(originalJpeg, fb->buf, fb->len);
      originalJpegLen = fb->len;
    }

    // Set dimensions (QVGA)
    imageWidth = 320;
    imageHeight = 240;
    imageSize = imageWidth * imageHeight;
    size_t rgb565Len = imageSize * 2;

    // Allocate buffers in PSRAM
    if (originalFrame)
      heap_caps_free(originalFrame);
    if (grayscaleBuffer)
      heap_caps_free(grayscaleBuffer);

    originalFrame = (uint8_t *)heap_caps_malloc(rgb565Len, MALLOC_CAP_SPIRAM);
    grayscaleBuffer = (uint8_t *)heap_caps_malloc(imageSize, MALLOC_CAP_SPIRAM);

    if (!originalFrame || !grayscaleBuffer) {
      esp_camera_fb_return(fb);
      if (originalFrame) {
        heap_caps_free(originalFrame);
        originalFrame = NULL;
      }
      if (grayscaleBuffer) {
        heap_caps_free(grayscaleBuffer);
        grayscaleBuffer = NULL;
      }
      server.send(500, "text/plain", "Out of memory");
      return;
    }

    // Decode JPEG to RGB565 (decompression helps reduce noise)
    if (!jpg2rgb565(fb->buf, fb->len, originalFrame, JPEG_IMAGE_SCALE_0)) {
      esp_camera_fb_return(fb);
      heap_caps_free(originalFrame);
      originalFrame = NULL;
      heap_caps_free(grayscaleBuffer);
      grayscaleBuffer = NULL;
      server.send(500, "text/plain", "Decode failed");
      return;
    }

    // Convert to grayscale for processing
    convertToGrayscale(originalFrame, grayscaleBuffer, imageWidth, imageHeight);
    esp_camera_fb_return(fb);

    server.send(200, "text/plain", "Captured");
  });

  // Threshold endpoint - applies adaptive thresholding
  server.on("/threshold", []() {
    if (!grayscaleBuffer || imageSize == 0) {
      server.send(400, "text/plain", "No image captured");
      return;
    }

    // Get target pixel count from request
    int targetPixels = TARGET_PIXELS;
    if (server.hasArg("target")) {
      targetPixels = server.arg("target").toInt();
      if (targetPixels < 1)
        targetPixels = 1;
    }

    // Tolerance is 1% of target
    int tolerance = targetPixels / 100;
    if (tolerance < 1)
      tolerance = 1;

    // Allocate binary buffer
    if (binaryBuffer)
      heap_caps_free(binaryBuffer);
    binaryBuffer = (uint8_t *)heap_caps_malloc(imageSize, MALLOC_CAP_SPIRAM);

    if (!binaryBuffer) {
      server.send(500, "text/plain", "Out of memory");
      return;
    }

    // Find optimal threshold using binary search
    calculatedThreshold =
        findOptimalThreshold(grayscaleBuffer, binaryBuffer, imageSize,
                             targetPixels, tolerance, MAX_ITERATIONS);

    // Apply threshold one more time to get final count
    finalPixelCount = applyThresholdAndCount(grayscaleBuffer, binaryBuffer,
                                             imageSize, calculatedThreshold);

    Serial.printf("Final: Threshold=%d, PixelCount=%d\n", calculatedThreshold,
                  finalPixelCount);

    String msg = "Done: T=" + String(calculatedThreshold) +
                 ", Pixels=" + String(finalPixelCount);
    server.send(200, "text/plain", msg);
  });

  // View endpoint - serves images as JPEG
  server.on("/view", []() {
    // For original image, send the stored JPEG directly
    if (!server.hasArg("t") || server.arg("t") != "bin") {
      if (!originalJpeg || originalJpegLen == 0) {
        server.send(404);
        return;
      }

      WiFiClient client = server.client();
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: image/jpeg");
      client.print("Content-Length: ");
      client.println(originalJpegLen);
      client.println("Connection: close");
      client.println();
      client.write(originalJpeg, originalJpegLen);
      client.flush();
      return;
    }

    // For binary image, need to convert to JPEG
    if (!binaryBuffer) {
      server.send(404);
      return;
    }

    size_t rgb565Size = imageSize * 2;
    uint8_t *rgb565Buf =
        (uint8_t *)heap_caps_malloc(rgb565Size, MALLOC_CAP_SPIRAM);
    if (!rgb565Buf) {
      server.send(500, "text/plain", "Memory error");
      return;
    }

    // Convert grayscale binary to RGB565 format
    for (int i = 0; i < imageSize; i++) {
      uint8_t gray = binaryBuffer[i];
      uint8_t r5 = gray >> 3;
      uint8_t g6 = gray >> 2;
      uint8_t b5 = gray >> 3;
      uint16_t pixel = (r5 << 11) | (g6 << 5) | b5;
      rgb565Buf[i * 2] = pixel & 0xFF;
      rgb565Buf[i * 2 + 1] = (pixel >> 8) & 0xFF;
    }

    uint8_t *jpgBuf = NULL;
    size_t jpgLen = 0;
    bool success = fmt2jpg(rgb565Buf, rgb565Size, imageWidth, imageHeight,
                           PIXFORMAT_RGB565, 80, &jpgBuf, &jpgLen);
    heap_caps_free(rgb565Buf);

    if (!success) {
      server.send(500, "text/plain", "JPEG error");
      return;
    }

    WiFiClient client = server.client();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(jpgLen);
    client.println("Connection: close");
    client.println();
    client.write(jpgBuf, jpgLen);
    client.flush();
    free(jpgBuf);
  });

  server.begin();
  Serial.println("Server started!");
}

void loop() {
  server.handleClient();
  delay(10);
}
