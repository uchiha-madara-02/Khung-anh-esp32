#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include "FontMaker.h"

// Các chân SD (SPI)
#define SD_MOSI 35
#define SD_MISO 37
#define SD_SCK  36
#define SD_CS   38

// #define SD_MOSI 2
// #define SD_MISO 4
// #define SD_SCK  3
// #define SD_CS   1

// #define SD_MOSI 23
// #define SD_MISO 19
// #define SD_SCK  18
// #define SD_CS   5

const int bootButtonPin = 0;   

SPIClass spiSD(FSPI);  
TFT_eSPI tft = TFT_eSPI();
WebServer server(80);

bool apMode = false;
unsigned long buttonPressTime = 0;
const unsigned long longPressDur = 3000;
unsigned long lastSwitchTime = 0;

void setpx(int16_t x,int16_t y,uint16_t color)
{
  tft.drawPixel(x,y,color); 
}
MakeFont myfont(&setpx);

struct ImageInfo {
  String filename;  
  int x, y, duration;
};
std::vector<ImageInfo> imageList;

int pendingX = 0, pendingY = 0, pendingD = 2000;

void checkOrCreateMetadataFile() {
  if (!SD.exists("/metadata.txt")) {
    File f = SD.open("/metadata.txt", FILE_WRITE);
    if (f) f.close();
    Serial.println("Created metadata.txt");
  }
}

void loadMetadata() {
  imageList.clear();
  File f = SD.open("/metadata.txt", FILE_READ);
  if (!f) {
    Serial.println("Failed to open metadata.txt");
    return;
  }

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.isEmpty()) continue;

    int c1 = line.indexOf(','), c2 = line.indexOf(',', c1 + 1), c3 = line.indexOf(',', c2 + 1);
    if (c1 < 0 || c2 < 0 || c3 < 0) continue;

    String name = line.substring(0, c1);
    if (!name.startsWith("/")) name = "/" + name;
    name.toLowerCase();
    if (!name.endsWith(".jpg") && !name.endsWith(".jpeg")) name += ".jpg";

    ImageInfo img = {
      name,
      line.substring(c1 + 1, c2).toInt(),
      line.substring(c2 + 1, c3).toInt(),
      line.substring(c3 + 1).toInt()
    };

    if (SD.exists(img.filename)) {
      imageList.push_back(img);
    } else {
      Serial.printf("File %s not found, skipped.\n", img.filename.c_str());
    }
  }

  f.close();
  Serial.printf("Loaded %u image entries from metadata\n", imageList.size());
}

void saveMetadata() {
  File f = SD.open("/metadata.txt", FILE_WRITE);
  if (!f) {
    Serial.println("Failed to open metadata.txt for writing");
    return;
  }

  for (auto &img : imageList) {
    String name = img.filename;
    if (name.startsWith("/")) name = name.substring(1);
    f.printf("%s,%d,%d,%d\n", name.c_str(), img.x, img.y, img.duration);
  }

  f.close();
  Serial.println("metadata.txt saved");
}

void drawJpeg(const char *filename, int xpos, int ypos) {
  File jf = SD.open(filename);
  if (!jf) {
    Serial.printf("Cannot open %s\n", filename);
    return;
  }
  if (!JpegDec.decodeSdFile(jf)) {
    Serial.println("JPEG decode failed");
    jf.close(); 
    return;
  }

  tft.startWrite();
  while (JpegDec.read()) {
    uint16_t *pImg = JpegDec.pImage;
    if (!pImg) continue;

    int mw = JpegDec.MCUWidth;
    int mh = JpegDec.MCUHeight;
    int mx = JpegDec.MCUx * mw;
    int my = JpegDec.MCUy * mh;

    for (int y = 0; y < mh; y++) {
      for (int x = 0; x < mw; x++) {
        int px = mx + x;
        int py = my + y;
        if (px < JpegDec.width && py < JpegDec.height) {
          tft.drawPixel(xpos + px, ypos + py, *pImg++);
        } else {
          pImg++;
        }
      }
    }
  }
  tft.endWrite();
  jf.close(); 
}

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    :root {
      --primary: #4a90e2;
      --success: #2ecc71;
      --bg: #f5f7fa;
      --card-bg: #fff;
      --text: #333;
      --radius: 8px;
      --transition: 0.3s;
      --danger: #e74c3c;
      --danger-hover: #c0392b;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Segoe UI', Tahoma, sans-serif;
      background: var(--bg);
      color: var(--text);
      padding: 20px;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    h2 { margin: 30px 0 20px; font-size: 1.8rem; color: var(--primary); text-align: center; }
    form {
      background: var(--card-bg);
      padding: 20px;
      border-radius: var(--radius);
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
      width: 100%; max-width: 400px;
      display: flex;
      flex-direction: column;
      gap: 15px;
    }
    label { font-weight: bold; font-size: 0.9rem; }
    input[type="file"] {
      border: 2px dashed var(--primary);
      padding: 20px;
      border-radius: var(--radius);
      cursor: pointer;
      width: 100%;
      transition: border-color var(--transition);
    }
    input[type="file"]:hover { border-color: rgba(74,144,226,0.8); }
    input[type="number"], input[type="submit"] {
      padding: 10px;
      font-size: 1rem;
      border: 1px solid #ccc;
      border-radius: var(--radius);
      width: 100%;
      transition: border-color var(--transition);
    }
    input[type="number"]:focus { border-color: var(--primary); outline: none; }
    input[type="submit"] {
      background: var(--primary);
      color: #fff;
      border: none;
      cursor: pointer;
      font-weight: bold;
    }
    input[type="submit"]:hover { opacity: 0.9; }
    #progressContainer {
      display: none;
      width: 100%;
      background: #e0e0e0;
      border-radius: var(--radius);
      overflow: hidden;
      margin-top: 10px;
    }
    #progressBar {
      height: 20px;
      width: 0;
      background: var(--success);
      color: #fff;
      font-weight: bold;
      text-align: center;
      line-height: 20px;
      transition: width var(--transition);
    }
    #list {
      background: var(--card-bg);
      padding: 15px;
      border-radius: var(--radius);
      width: 100%; max-width: 800px;
      margin-top: 20px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
    }
    .empty-msg {
      text-align: center;
      color: #777;
      font-size: 1rem;
    }
    ul { list-style: none; padding-left: 0; }
    li {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 8px 0;
      border-bottom: 1px solid #eee;
      flex-wrap: wrap;
    }
    li span { margin-right: 10px; }
    li:last-child { border-bottom: none; }
    .delete-link {
      color: var(--danger);
      text-decoration: none;
      font-weight: bold;
      transition: color var(--transition);
    }
    .delete-link:hover { color: var(--danger-hover); }
    @media (max-width: 600px) {
      body { padding: 10px; }
      h2 { font-size: 1.5rem; }
      form { padding: 15px; }
      input[type="file"] { padding: 15px; }
      input[type="submit"] { font-size: 0.9rem; }
      li { flex-direction: column; align-items: flex-start; }
    }
  </style>
  <title>Uchiha Madara</title>
</head>
<body>
  <h2>Tải ảnh JPEG</h2>
  <form id="uploadForm" enctype="multipart/form-data">
    <label for="image">Chọn ảnh (JPEG):</label>
    <input type="file" id="image" name="image" accept=".jpg,.jpeg" required>
    <label for="x">Tọa độ X:</label>
    <input type="number" id="x" name="x" value="0">
    <label for="y">Tọa độ Y:</label>
    <input type="number" id="y" name="y" value="0">
    <label for="duration">Thời gian hiển thị (ms):</label>
    <input type="number" id="duration" name="duration" value="2000">
    <input type="submit" value="Tải lên">
    <div id="progressContainer">
      <div id="progressBar">0%</div>
    </div>
  </form>

  <h2>Các tệp đã lưu</h2>
  <div id="list">Đang tải…</div>

  <script>
    const progressContainer = document.getElementById("progressContainer");
    const progressBar = document.getElementById("progressBar");
    const listContainer = document.getElementById('list');

    document.getElementById("uploadForm").addEventListener("submit", async (e) => {
      e.preventDefault();
      const x = document.getElementById("x").value;
      const y = document.getElementById("y").value;
      const duration = document.getElementById("duration").value;
      const file = document.getElementById('image').files[0];
      if (!file) return alert("Vui lòng chọn ảnh");

      await fetch("/uploadMeta", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: `x=${x}&y=${y}&duration=${duration}`
      });

      progressContainer.style.display = "block";
      progressBar.style.width = "0%";
      progressBar.textContent = "0%";

      const formData = new FormData();
      formData.append("image", file);

      const xhr = new XMLHttpRequest();
      xhr.open("POST", "/upload", true);

      xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) {
          const percent = Math.round((e.loaded / e.total) * 100);
          progressBar.style.width = percent + "%";
          progressBar.textContent = percent + "%";
          // khi đạt 100% thì reload trang
          if (percent === 100) {
            setTimeout(() => window.location.reload(), 500);
          }
        }
      };

      xhr.send(formData);
    });

    async function loadList() {
      listContainer.innerHTML = 'Đang tải…';
      try {
        const html = await fetch('/list').then(res => res.text());
        listContainer.innerHTML = html || '<div class="empty-msg">Chưa có tệp nào.</div>';
      } catch (err) {
        listContainer.innerHTML = '<div class="empty-msg">Không thể tải danh sách.</div>';
      }
    }

    window.addEventListener('DOMContentLoaded', loadList);
  </script>
</body>
</html>
)rawliteral");
}

void handleUploadMetadata() {
  pendingX = server.arg("x").toInt();
  pendingY = server.arg("y").toInt();
  pendingD = server.arg("duration").toInt();
  Serial.printf("Received metadata: x=%d, y=%d, duration=%d\n", pendingX, pendingY, pendingD);
  server.send(200, "text/plain", "Metadata received");
}

void handleUpload() {
  HTTPUpload &up = server.upload();
  static File f;
  static String fn;

  if (up.status == UPLOAD_FILE_START) {
    fn = "/" + up.filename;
    fn.toLowerCase();
    if (!fn.endsWith(".jpg") && !fn.endsWith(".jpeg")) {
      fn += ".jpg";
    }

    Serial.printf("Start upload: %s\n", fn.c_str());
    f = SD.open(fn, FILE_WRITE);
    if (!f) Serial.println("Cannot open file to write!");
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (f) f.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END) {
    if (f) {
      f.close();
      Serial.printf("Uploaded: %s (%d bytes)\n", fn.c_str(), up.totalSize);
      imageList.push_back({fn, pendingX, pendingY, pendingD});
      saveMetadata();
    }
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

void handleList() {
  String h = "<ul>";
  for (auto &img : imageList) {
    h += "<li>" + img.filename + " (" + img.x + "," + img.y + "," + img.duration + "ms) "
         "<a href='/delete?file=" + img.filename + "'>Xóa</a></li>";
  }
  h += "</ul>";
  server.send(200, "text/html", h);
}

void handleDelete() {
  String fn = server.arg("file");
  SD.remove(fn);
  for (int i = 0; i < imageList.size(); i++) {
    if (imageList[i].filename == fn) {
      imageList.erase(imageList.begin() + i);
      break;
    }
  }
  saveMetadata();
  server.sendHeader("Location", "/");
  server.send(303);
}

void displayAPScreen() {
  tft.fillScreen(TFT_BLACK);
  myfont.set_font(vnnfont16);
  myfont.print(46, 10, "Chế Độ Tải Ảnh", TFT_YELLOW, TFT_BLACK);
  myfont.print(10, 50, "Tên Wifi: Uchia Madara", TFT_GREEN, TFT_BLACK);
  myfont.print(10, 80, "Mật khẩu: 12345678", TFT_GREEN, TFT_BLACK);
  myfont.print(10, 110, "Địa chỉ: 192.168.4.1", TFT_GREEN, TFT_BLACK);
}

void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Uchiha Madara", "12345678");
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  displayAPScreen();

  server.on("/", handleRoot);
  server.on("/uploadMeta", HTTP_POST, handleUploadMetadata);
  server.on("/upload", HTTP_POST, [](){}, handleUpload);
  server.on("/list", HTTP_GET, handleList);
  server.on("/delete", HTTP_GET, handleDelete);
  server.begin();
}

void stopAP() {     
  WiFi.softAPdisconnect(true);
  lastSwitchTime = 0;
  tft.fillScreen(TFT_BLACK);
}

void setup() {
  Serial.begin(115200);
  pinMode(bootButtonPin, INPUT_PULLUP);

  tft.begin(); 
  tft.setRotation(0); 
  tft.setSwapBytes(true); 
  tft.fillScreen(TFT_RED);

  spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, spiSD)) {
    Serial.println("Thẻ nhớ bị lỗi");
    tft.fillScreen(TFT_BLACK);
    myfont.set_font(vnfontdam22);
    // myfont.print(40,105,"Thẻ nhớ lỗi !!!", TFT_RED, TFT_BLACK);
    myfont.print(0,0,"Thẻ nhớ lỗi !!!", TFT_RED, TFT_BLACK);
    while (1);
  }
  Serial.println("SD Card OK");

  checkOrCreateMetadataFile();
  loadMetadata();
}

void loop() {
  if (digitalRead(bootButtonPin) == LOW) {
    if (buttonPressTime == 0) buttonPressTime = millis();
    else if (!apMode && millis() - buttonPressTime >= longPressDur) {
      apMode = true;
      startAP();
      buttonPressTime = millis();
    }
    else if (apMode && millis() - buttonPressTime >= longPressDur) {
      apMode = false;
      stopAP();
      buttonPressTime = millis();
    }
  } else buttonPressTime = 0;

  if (apMode) {
    server.handleClient();
    return;
  }

  static int currentIdx = 0;                   
  static unsigned long lastSwitchTime = 0;    
  unsigned long now = millis();

  if (imageList.empty()) {
    lastSwitchTime = now;
    return;
  }

  if (lastSwitchTime == 0) {
    tft.fillScreen(TFT_BLACK);
    drawJpeg(
      imageList[currentIdx].filename.c_str(),
      imageList[currentIdx].x,
      imageList[currentIdx].y
    );
    lastSwitchTime = now;
    return;
  }

  unsigned long waitTime = imageList[currentIdx].duration;

  if (now - lastSwitchTime >= waitTime) {
    currentIdx = (currentIdx + 1) % imageList.size();
    tft.fillScreen(TFT_BLACK);
    drawJpeg(
      imageList[currentIdx].filename.c_str(),
      imageList[currentIdx].x,
      imageList[currentIdx].y
    );
    lastSwitchTime = now;
  }
}
