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

// Wi-Fi settings
const char *ssid = "ID";
const char *password = "PW";

// Variables to store images
uint8_t *originalFrame = NULL;
int originalWidth = 0;
int originalHeight = 0;

uint8_t *resizedFrame = NULL;
size_t resizedLen = 0;
int resizedWidth = 0;
int resizedHeight = 0;

WebServer server(80);

volatile bool shouldAbort = false;

// Function to UPSAMPLE by integer factor L
bool upsampleImage(uint8_t *src, int srcW, int srcH, int L, uint8_t **outBuf,
                   int *outW, int *outH, size_t *outLen) {
  int newW = srcW * L;
  int newH = srcH * L;
  size_t newSize = newW * newH * 2;

  // Safety Limit for Intermediate Buffer
  if (newSize > 4000000)
    return false;

  uint8_t *newBuf = (uint8_t *)heap_caps_malloc(newSize, MALLOC_CAP_SPIRAM);
  if (!newBuf)
    return false;

  for (int y = 0; y < newH; y++) {
    if (shouldAbort) {
      heap_caps_free(newBuf);
      return false;
    }

    // Map to source Y (Repeat rows)
    int srcY = y / L;
    int srcRowOffset = srcY * srcW;
    int dstRowOffset = y * newW;

    for (int x = 0; x < newW; x++) {
      // Map to source X (Repeat cols)
      int srcX = x / L;
      int srcIndex = (srcRowOffset + srcX) * 2;
      int dstIndex = (dstRowOffset + x) * 2;

      newBuf[dstIndex] = src[srcIndex];
      newBuf[dstIndex + 1] = src[srcIndex + 1];
    }
    if ((y % 50) == 0)
      delay(0);
  }

  *outBuf = newBuf;
  *outW = newW;
  *outH = newH;
  *outLen = newSize;
  return true;
}

// Function to DOWNSAMPLE by integer factor M
bool downsampleImage(uint8_t *src, int srcW, int srcH, int M, uint8_t **outBuf,
                     int *outW, int *outH, size_t *outLen) {
  int newW = srcW / M;
  int newH = srcH / M;
  // Handle edge case if image is smaller than M
  if (newW == 0)
    newW = 1;
  if (newH == 0)
    newH = 1;

  size_t newSize = newW * newH * 2;

  uint8_t *newBuf = (uint8_t *)heap_caps_malloc(newSize, MALLOC_CAP_SPIRAM);
  if (!newBuf)
    return false;

  for (int y = 0; y < newH; y++) {
    if (shouldAbort) {
      heap_caps_free(newBuf);
      return false;
    }

    // Map to source Y (Skip rows)
    int srcY = y * M;
    int srcRowOffset = srcY * srcW;
    int dstRowOffset = y * newW;

    for (int x = 0; x < newW; x++) {
      // Map to source X (Skip cols)
      int srcX = x * M;
      int srcIndex = (srcRowOffset + srcX) * 2;
      int dstIndex = (dstRowOffset + x) * 2;

      newBuf[dstIndex] = src[srcIndex];
      newBuf[dstIndex + 1] = src[srcIndex + 1];
    }
    if ((y % 50) == 0)
      delay(0);
  }

  *outBuf = newBuf;
  *outW = newW;
  *outH = newH;
  *outLen = newSize;
  return true;
}

// Web page HTML code
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Rational Resizer</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; text-align: center; background: #eee; padding: 20px; }
    .container { max-width: 800px; margin: auto; background: white; padding: 20px; border-radius: 8px; }
    button { padding: 10px 20px; font-size: 16px; margin: 10px; cursor: pointer; }
    input { padding: 8px; font-size: 16px; width: 60px; text-align: center; }
    img { max-width: 100%; border: 1px solid #ddd; margin-top: 5px; }
    .row { display: flex; flex-wrap: wrap; justify-content: space-around; }
    .col { flex: 1; min-width: 300px; margin: 10px; }
  </style>
</head>
<body>
<div class="container">
  <h1>Sampling: L / M</h1>
  <div><button onclick="capture()">Capture New Image</button></div>
  <hr>
  <div>
    <p>Scale = Upsample(L) / Downsample(M)</p>
    <label>Upsample (L):</label> <input type="number" id="L" min="1" max="10" value="3">
    <label>Downsample (M):</label> <input type="number" id="M" min="1" max="10" value="2">
    <br>
    <button onclick="process()">Process</button>
    <button onclick="clearImages()" style="background-color: #f44336;">Clear</button>
  </div>
  <p id="status" style="color: blue; font-weight: bold;">Ready</p>
  <div class="row">
    <div class="col"><h3>Original</h3><img id="origImg" src=""></div>
    <div class="col"><h3>Result</h3><img id="resImg" src=""></div>
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
  function process() {
    let L = document.getElementById('L').value;
    let M = document.getElementById('M').value;
    status("Processing L=" + L + ", M=" + M + " ...");
    fetch('/resize?L=' + L + '&M=' + M).then(r => {
      if(r.ok) { 
        r.text().then(t => { status(t); });
        document.getElementById('resImg').src = "/view?t=res&ts=" + Date.now(); 
      }
      else r.text().then(t => status("Error: " + t));
    });
  }
  function clearImages() {
    if(confirm('Clear all images?')) {
        fetch('/clear').then(r => {
            document.getElementById('origImg').src = "";
            document.getElementById('resImg').src = "";
            status("Cleared");
        });
    }
  }
</script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // Setup Camera
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
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2; // Use 2 buffers

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera Init Failed");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected. IP: " + WiFi.localIP().toString());

  // Set up web server pages
  server.on("/", []() { server.send_P(200, "text/html", index_html); });

  server.on("/capture", []() {
    // Capture a new image
    camera_fb_t *fb = NULL;

    // Clear buffer first to get fresh image
    for (int i = 0; i < 2; i++) {
      fb = esp_camera_fb_get();
      esp_camera_fb_return(fb);
      fb = NULL;
    }

    // Actual capture
    fb = esp_camera_fb_get();
    if (!fb) {
      server.send(500, "text/plain", "Cam Fail");
      return;
    }

    // Save image to memory
    if (originalFrame)
      heap_caps_free(originalFrame);
    originalFrame = (uint8_t *)heap_caps_malloc(fb->len, MALLOC_CAP_SPIRAM);

    if (!originalFrame) {
      esp_camera_fb_return(fb);
      server.send(500, "text/plain", "Out of memory");
      return;
    }

    memcpy(originalFrame, fb->buf, fb->len);
    originalWidth = fb->width;
    originalHeight = fb->height;
    esp_camera_fb_return(fb);
    server.send(200, "text/plain", "Captured");
  });

  server.on("/resize", []() {
    if (!originalFrame) {
      server.send(400, "text/plain", "No Image");
      return;
    }

    // Default L=1, M=1
    int L = 1;
    int M = 1;
    if (server.hasArg("L"))
      L = server.arg("L").toInt();
    if (server.hasArg("M"))
      M = server.arg("M").toInt();

    // Sanity
    if (L < 1)
      L = 1;
    if (M < 1)
      M = 1;

    // Clear previous
    if (resizedFrame) {
      heap_caps_free(resizedFrame);
      resizedFrame = NULL;
    }

    // STEP 1: UPSAMPLE
    uint8_t *tempBuf = NULL;
    int tempW, tempH;
    size_t tempLen;
    // Note: upsampleImage function handles L
    if (!upsampleImage(originalFrame, originalWidth, originalHeight, L,
                       &tempBuf, &tempW, &tempH, &tempLen)) {
      server.send(500, "text/plain", "Upsample MEM Fail");
      return;
    }

    // STEP 2: DOWNSAMPLE
    if (M == 1) {
      resizedFrame = tempBuf;
      resizedWidth = tempW;
      resizedHeight = tempH;
      resizedLen = tempLen;
    } else {
      if (!downsampleImage(tempBuf, tempW, tempH, M, &resizedFrame,
                           &resizedWidth, &resizedHeight, &resizedLen)) {
        heap_caps_free(tempBuf); // Free temp
        server.send(500, "text/plain", "Downsample OOM");
        return;
      }
      heap_caps_free(tempBuf); // Free temp after downsampling
    }

    String msg = "Done: L=" + String(L) + " M=" + String(M) + " -> " +
                 String(resizedWidth) + "x" + String(resizedHeight);
    server.send(200, "text/plain", msg);
  });

  // Clear memory
  server.on("/clear", []() {
    if (originalFrame) {
      heap_caps_free(originalFrame);
      originalFrame = NULL;
    }
    if (resizedFrame) {
      heap_caps_free(resizedFrame);
      resizedFrame = NULL;
    }
    server.send(200, "text/plain", "Cleared");
  });

  // View image (sends as JPEG)
  server.on("/view", []() {
    uint8_t *target = originalFrame;
    int w = originalWidth;
    int h = originalHeight;
    int len = w * h * 2;

    if (server.hasArg("t") && server.arg("t") == "res") {
      if (!resizedFrame) {
        server.send(404);
        return;
      }
      target = resizedFrame;
      w = resizedWidth;
      h = resizedHeight;
      len = resizedLen;
    } else if (!originalFrame) {
      server.send(404);
      return;
    }

    // Convert raw image to JPEG
    uint8_t *jpgBuf = NULL;
    size_t jpgLen = 0;

    bool success =
        fmt2jpg(target, len, w, h, PIXFORMAT_RGB565, 12, &jpgBuf, &jpgLen);

    if (!success) {
      server.send(500, "text/plain", "JPEG Error");
      return;
    }

    // Send the image manually
    WiFiClient client = server.client();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(jpgLen);
    client.println("Content-Disposition: inline; filename=capture.jpg");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();

    client.write(jpgBuf, jpgLen);
    client.flush();
    free(jpgBuf);
  });

  server.begin();
}

void loop() {
  server.handleClient();
  delay(10);
}
