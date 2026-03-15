# 🖼️ ESP32 Digital Photo Frame - Uchiha Madara

Dự án biến **ESP32-S3** và màn hình **TFT** thành một khung ảnh kỹ thuật số thông minh. Thiết bị cho phép hiển thị hình ảnh JPEG từ thẻ nhớ SD và cung cấp giao diện Web để tải ảnh lên trực tiếp qua WiFi mà không cần tháo thẻ nhớ.

---

## ✨ Tính năng nổi bật

* **🌐 Web Dashboard:** Giao diện tải ảnh hiện đại với thanh tiến trình (Progress Bar) trực quan.
* **📂 Quản lý Metadata:** Tự động lưu trữ tọa độ hiển thị ($x, y$) và thời gian chuyển cảnh (ms) vào file `metadata.txt`.
* **⚡ Chế độ AP Mode:** Nhấn giữ nút BOOT để phát WiFi, giúp cập nhật kho ảnh mọi lúc mọi nơi.
* **🖼️ Hiển thị chất lượng cao:** Sử dụng thư viện `JPEGDecoder` để render ảnh JPEG mượt mà.
* **🇻🇳 Hỗ trợ Tiếng Việt:** Hiển thị thông tin hướng dẫn bằng tiếng Việt có dấu ngay trên màn hình TFT.

---

## 🛠️ Sơ đồ đấu nối (Hardware Pinout)

Dự án sử dụng chuẩn giao tiếp SPI để điều khiển thẻ nhớ SD. Bạn cần chú ý cấu hình các chân GPIO sau:

### 1. Kết nối thẻ nhớ SD
| Chân SD Card | Chân ESP32 (GPIO) |
| :--- | :---: |
| **MOSI** | 35 |
| **MISO** | 37 |
| **SCK** | 36 |
| **CS** | 38 |

### 2. Linh kiện khác
| Linh kiện | Chân GPIO |
| :--- | :---: |
| **Nút BOOT** | 0 |
| **Màn hình TFT** | Cấu hình trong `User_Setup.h` |

---

## 🚀 Hướng dẫn sử dụng

### 1. Chế độ Trình chiếu (SlideShow)
Khi khởi động, thiết bị sẽ tự động đọc danh sách ảnh từ thẻ nhớ và hiển thị xoay vòng dựa trên thời gian bạn đã cài đặt.

### 2. Chế độ Cấu hình (AP Mode)
Để thêm hoặc xóa ảnh, hãy thực hiện các bước sau:
1.  **Kích hoạt:** Nhấn và giữ nút **BOOT trong 3 giây**.
2.  **Kết nối:** Kết nối điện thoại/máy tính vào WiFi:
    * **SSID:** `Uchiha Madara`
    * **Password:** `12345678`
3.  **Truy cập:** Mở trình duyệt và nhập địa chỉ: `192.168.4.1`
4.  **Thao tác:** Chọn file JPEG, nhập tọa độ và nhấn **Tải lên**. Thiết bị sẽ tự động reload sau khi hoàn tất.

---

## ⚠️ Thư viện yêu cầu

Để biên dịch code thành công, bạn cần cài đặt các thư viện sau:
* `TFT_eSPI`: Cấu hình driver màn hình trong file `User_Setup.h`.
* `JPEGDecoder`: Giải mã hình ảnh JPEG từ SD Card.
* `WebServer`, `WiFi`, `SPI`, `SD`: Các thư viện tiêu chuẩn của ESP32.
* `FontMaker`: Thư viện xử lý Font Tiếng Việt (đi kèm trong repo).

---

## 📝 Lưu ý kỹ thuật

* **Định dạng thẻ nhớ:** Sử dụng thẻ MicroSD định dạng **FAT32**.
* **Định dạng ảnh:** Hệ thống chỉ nhận file `.jpg` hoặc `.jpeg`.
* **Nguồn điện:** Module WiFi và SD Card tiêu tốn khá nhiều năng lượng, hãy đảm bảo nguồn cấp ổn định để tránh hiện tượng sập nguồn khi upload.

---
*Dự án được thực hiện bởi Nguyễn Hoàng Nam. Nếu thấy thú vị, hãy tặng mình 1 ⭐ nhé!*
