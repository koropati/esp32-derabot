# DeraBot v1.0 — ESP32-C3 Environmental Monitor

Firmware clean-architecture untuk ESP32-C3 Super Mini dengan sensor lingkungan, tampilan OLED, koneksi WiFi + MQTT, dan sistem alarm buzzer yang dapat dikonfigurasi.

---

## Daftar Isi

1. [Spesifikasi Hardware](#spesifikasi-hardware)
2. [Skema Wiring](#skema-wiring)
3. [Persyaratan Software](#persyaratan-software)
4. [Konfigurasi Project](#konfigurasi-project)
5. [Upload Firmware — Linux](#upload-firmware--linux)
6. [Upload Firmware — Windows](#upload-firmware--windows)
7. [Panduan Navigasi UI](#panduan-navigasi-ui)
8. [Pengaturan Alarm Buzzer](#pengaturan-alarm-buzzer)
9. [Struktur Project](#struktur-project)
10. [Troubleshooting](#troubleshooting)

---

## Spesifikasi Hardware

| Komponen | Model | Interface |
|----------|-------|-----------|
| Mikrokontroler | ESP32-C3 Super Mini | — |
| Sensor Suhu/Kelembaban/Tekanan | BME280 | I2C (0x76) |
| Mikrofon Digital | INMP441 | I2S |
| Display | SSD1306 OLED 0.96" 128×64 | I2C (0x3C) |
| Sensor Tegangan Baterai | Voltage Divider | ADC (GPIO0) |
| Buzzer | Passive Buzzer | PWM (GPIO5) |
| Tombol Kiri | Push Button + Pull-Up | GPIO6 |
| Tombol Tengah | Push Button + Pull-Up | GPIO7 |
| Tombol Kanan | Push Button + Pull-Up | GPIO10 |

---

## Skema Wiring

### I2C Bus (BME280 + OLED — shared bus)

```
ESP32-C3          BME280          OLED SSD1306
GPIO8 (SDA) ──────── SDA ─────────── SDA
GPIO9 (SCL) ──────── SCL ─────────── SCL
3.3V        ──────── VCC ─────────── VCC
GND         ──────── GND ─────────── GND
```

> Jika I2C tidak stabil: tambahkan resistor pull-up 4.7kΩ dari SDA ke 3.3V dan SCL ke 3.3V.

### INMP441 (I2S Microphone)

```
ESP32-C3          INMP441
GPIO2  ────────── SD  (Data)
GPIO3  ────────── WS  (Word Select / LRCK)
GPIO4  ────────── SCK (Serial Clock)
3.3V   ────────── VDD
GND    ────────── GND
GND    ────────── L/R (kiri channel)
```

### Sensor Tegangan & Buzzer

```
ESP32-C3
GPIO0  ←── Voltage Divider (R1=100kΩ dari VBAT, R2=100kΩ ke GND)
GPIO5  ──→ Buzzer (passive) → GND
```

### Push Buttons (Pull-Up Terintegrasi)

```
ESP32-C3
GPIO6  ←── Tombol KIRI   (LOW saat ditekan)
GPIO7  ←── Tombol TENGAH (LOW saat ditekan)
GPIO10 ←── Tombol KANAN  (LOW saat ditekan)
```

> Gunakan tombol dengan resistor pull-up terintegrasi, atau pasang pull-up eksternal 10kΩ ke 3.3V.

---

## Persyaratan Software

### Tool yang Dibutuhkan

- **Python 3.8+** — diperlukan oleh PlatformIO
- **PlatformIO Core (CLI)** — build system dan upload tool
- **Driver USB** — untuk komunikasi dengan ESP32-C3

### Instalasi PlatformIO

**Linux / macOS:**
```bash
pip install platformio
# atau jika ada pip3:
pip3 install --user platformio
```

**Windows (PowerShell):**
```powershell
pip install platformio
```

### Library yang Diinstall Otomatis

PlatformIO menginstall library berikut secara otomatis saat pertama kali build:

| Library | Fungsi |
|---------|--------|
| Adafruit BME280 | Sensor suhu/kelembaban/tekanan |
| Adafruit BusIO | Abstraksi I2C/SPI |
| Adafruit GFX | Graphics library |
| Adafruit SSD1306 | Driver OLED display |
| PubSubClient | MQTT client |
| ArduinoJson | JSON serialization |

---

## Konfigurasi Project

Semua konfigurasi ada di **satu file**: `src/config/config.h`

### Pin Assignment

```cpp
namespace Pins {
    constexpr int MIC_WS    = 3;   // INMP441 Word Select
    constexpr int MIC_SCK   = 4;   // INMP441 Clock
    constexpr int MIC_SD    = 2;   // INMP441 Data
    constexpr int I2C_SDA   = 8;   // I2C Data (BME280 + OLED)
    constexpr int I2C_SCL   = 9;   // I2C Clock
    constexpr int VOLT      = 0;   // ADC Voltage Sensor
    constexpr int BUZZER    = 5;   // Buzzer PWM
    constexpr int BTN_LEFT  = 6;   // Tombol Kiri
    constexpr int BTN_CENTER = 7;  // Tombol Tengah
    constexpr int BTN_RIGHT = 10;  // Tombol Kanan
}
```

### Konfigurasi MQTT (HiveMQ Cloud)

Edit bagian berikut di `src/config/config.h`:

```cpp
namespace Mqtt {
    constexpr const char* HOST   = "xxxx.s1.eu.hivemq.cloud"; // ganti dengan broker Anda
    constexpr int         PORT   = 8883;                       // TLS port
    constexpr const char* USER   = "username";                 // username HiveMQ
    constexpr const char* PASS   = "password";                 // password HiveMQ
    constexpr const char* CLIENT = "esp32-derabot";            // client ID unik
}
```

> **Catatan:** Firmware menggunakan `setInsecure()` untuk koneksi TLS — cocok untuk development.
> Untuk production, ganti dengan root CA certificate HiveMQ.

### Topic MQTT

| Topic | Arah | Isi |
|-------|------|-----|
| `derabot/sensor` | Publish | Data sensor JSON |
| `derabot/status` | Publish | Status device |
| `derabot/command` | Subscribe | Perintah (misal: `silence`) |

### Timing

```cpp
namespace Timing {
    constexpr uint32_t SENSOR_MS    = 2000;  // baca sensor setiap 2 detik
    constexpr uint32_t MQTT_MS      = 5000;  // publish MQTT setiap 5 detik
    constexpr uint32_t DISPLAY_MS   = 100;   // refresh display setiap 100ms
    constexpr uint32_t DEBOUNCE_MS  = 15;    // debounce tombol 15ms
    constexpr uint32_t LONG_PRESS_MS = 2500; // long press 2.5 detik
}
```

### Voltage Divider

```cpp
namespace Voltage {
    constexpr float R1    = 100000.0f;  // resistor atas (Ohm)
    constexpr float R2    = 100000.0f;  // resistor bawah (Ohm)
    constexpr float V_MAX = 4.2f;       // tegangan penuh LiPo
    constexpr float V_MIN = 3.0f;       // tegangan cutoff LiPo
}
```

---

## Upload Firmware — Linux

### 1. Install Driver (tidak diperlukan untuk kebanyakan distro)

ESP32-C3 Super Mini menggunakan USB native (USB-JTAG/Serial), biasanya terdeteksi langsung di Linux sebagai `/dev/ttyACM0`.

```bash
# Cek apakah device terdeteksi
ls /dev/ttyACM*
# atau
dmesg | tail -20
```

### 2. Tambah User ke Group `uucp` (atau `dialout`)

```bash
# Arch Linux / Manjaro:
sudo usermod -aG uucp $USER

# Ubuntu / Debian:
sudo usermod -aG dialout $USER

# Setelah menambah group, logout lalu login kembali
# atau gunakan: newgrp uucp
```

### 3. Clone / Buka Project

```bash
cd /path/ke/project
ls  # pastikan ada platformio.ini
```

### 4. Build dan Upload

```bash
# Build + Upload sekaligus:
pio run --target upload

# Jika permission denied, gunakan sg:
sg uucp -c "pio run --target upload"

# Atau jika pio belum di PATH:
~/.local/bin/pio run --target upload
```

### 5. Monitor Serial (opsional)

```bash
pio device monitor
# atau:
~/.local/bin/pio device monitor --baud 115200 --port /dev/ttyACM0
```

### Port Custom

Jika device muncul di port lain (misal `/dev/ttyACM1`), edit `platformio.ini`:

```ini
upload_port = /dev/ttyACM1
monitor_port = /dev/ttyACM1
```

---

## Upload Firmware — Windows

### 1. Install Driver

Untuk ESP32-C3 Super Mini dengan USB native, Windows mungkin perlu driver tambahan:

**Opsi A — Zadig (rekomendasi):**
1. Download Zadig dari [zadig.akeo.ie](https://zadig.akeo.ie)
2. Buka Zadig → Options → List All Devices
3. Pilih "USB Serial (CDC)" atau "CP210x" / "CH340" sesuai yang muncul
4. Pilih driver **WinUSB** atau **libusbK** → Install Driver

**Opsi B — Instalasi manual:**
- Download driver CP210x dari Silicon Labs jika menggunakan board dengan chip tersebut
- Download driver CH340 jika menggunakan board dengan chip tersebut
- ESP32-C3 Super Mini asli menggunakan USB native (tidak perlu chip CP210x/CH340)

### 2. Verifikasi Device di Device Manager

1. Buka **Device Manager** (tekan `Win+X` → Device Manager)
2. Cari **Ports (COM & LPT)**
3. Catat nomor COM yang muncul (misal: `COM3`, `COM4`)

### 3. Install Python + PlatformIO

```powershell
# Install Python dari python.org (centang "Add to PATH")
# Lalu install PlatformIO:
pip install platformio
```

### 4. Buat File Kredensial `config_secrets.h`

> **Wajib.** File `src/config/config_secrets.h` berisi kredensial MQTT dan
> **git-ignored**, jadi TIDAK ikut ter-clone dari repo / komputer lain.
> Tanpa file ini, build akan gagal dengan error:
> `fatal error: config_secrets.h: No such file or directory`.

Salin template lalu isi nilai asli Anda:

```powershell
# Dari folder project:
Copy-Item src\config\config_secrets.example.h src\config\config_secrets.h
```

Edit `src\config\config_secrets.h`:

```cpp
namespace Secrets {
namespace Mqtt {
    constexpr const char* HOST   = "xxxx.s1.eu.hivemq.cloud"; // broker Anda
    constexpr int         PORT   = 8883;                       // TLS port
    constexpr const char* USER   = "your-username";
    constexpr const char* PASS   = "your-password";
    constexpr const char* CLIENT = "esp32-derabot";            // harus unik per device
}
}
```

### 5. Build dan Upload

Buka PowerShell di folder project. **Cara yang direkomendasikan** adalah
meng-override port lewat command line — tanpa mengubah `platformio.ini`,
sehingga setting Linux (`/dev/ttyACM0`) tetap utuh:

```powershell
# Masuk ke folder project
cd C:\path\ke\project

# Build + Upload ke COM10 (ganti sesuai Device Manager):
pio run --target upload --upload-port COM10

# Jika error "pio not found":
python -m platformio run --target upload --upload-port COM10
```

**Alternatif** — set port permanen di `platformio.ini` (akan menimpa setting
Linux, jadi pakai ini hanya kalau komputer ini khusus Windows):

```ini
upload_port = COM10       ; <-- ganti sesuai Device Manager
monitor_port = COM10      ; <-- sama dengan upload_port
```

> **Catatan:** Saat pertama kali build, PlatformIO otomatis mengunduh
> toolchain RISC-V dan semua library (BME280, SSD1306, PubSubClient, dll).
> Proses ini butuh koneksi internet dan bisa memakan beberapa menit.

### 6. Monitor Serial (opsional)

```powershell
pio device monitor --port COM10 --baud 115200
# atau:
python -m platformio device monitor --port COM10 --baud 115200
```

### Menggunakan VS Code + PlatformIO Extension (rekomendasi)

1. Install [Visual Studio Code](https://code.visualstudio.com)
2. Install extension **PlatformIO IDE** dari marketplace
3. Buka folder project di VS Code
4. Klik tombol **Upload** (→) di toolbar bawah
5. Klik tombol **Serial Monitor** (🔌) untuk monitor

---

## Panduan Navigasi UI

### Halaman Utama (Home)

Menampilkan:
- **Header**: signal WiFi, status koneksi, nama device, persentase baterai
- **Data sensor**: suhu, kelembaban, tekanan, desibel, tegangan
- **Status bawah**: IP address (jika terhubung) atau "ALARM aktif"

Baris paling bawah adalah **bar tombol**: tiap label sejajar dengan tombol
fisik di bawah layar (Kiri / Tengah / Kanan), jadi label "Menu" yang berkotak
berada tepat di atas tombol **Tengah** — itulah pemicu menu.

```
┌─────────────────────────┐
│▮▮▮▮• DeraBot    87% ▓▓▓│
│─────────────────────────│
│28.5°C       RH:65%     │
│             1013hPa    │
│Suara:45.2dB     3.85V  │
│WiFi 192.168.1.105      │
│─────────────────────────│
│Mute      [Menu]        │
└─────────────────────────┘
```

**Tombol di halaman utama:**
| Tombol | Fungsi |
|--------|--------|
| Kiri | Matikan alarm (jika alarm aktif) |
| Tengah | Buka menu |
| Kanan | — |

### Menu Utama

```
┌─────────────────────────┐
│── MENU ─────────────────│
│> WiFi via HP           │
│  Sensor Detail         │
│  Compass               │
│  Bursa IHSG            │
│  Settings              │
│Atas      [Pilih] Bawah │
└─────────────────────────┘
```

**Tombol di menu** (label di bar bawah sejajar tombol fisiknya):
| Tombol | Fungsi |
|--------|--------|
| Kiri | Naik / Keluar menu (jika di item paling atas) |
| Tengah | Masuk ke sub-menu yang dipilih |
| Kanan | Turun (berputar) |

### WiFi via HP

Penyetelan WiFi dilakukan dari ponsel lewat portal web (tidak ada lagi input
password pakai tombol — fitur itu dihapus untuk menghemat flash):

1. Pilih **WiFi via HP** di menu → device menyalakan access point
2. Sambungkan WiFi ponsel ke SSID yang tertera di layar
3. Buka browser ke alamat IP yang tertera, pilih jaringan & isi password
4. Setelah submit, device menyimpan kredensial dan menyambung otomatis
5. Tekan **Kiri** untuk keluar dari portal

### Sensor Detail

Dua halaman data sensor:

**Halaman 1 — Environment:**
```
Temp  :   28.50 C
Humid :   65.0 %
Press : 1013.0 hPa
Sound :   45.2 dB
```

**Halaman 2 — Power & Network:**
```
Volt   :   3.85 V
Battery:    87 %
RSSI   :    -65 dBm
Signal :     70 %
```

**Tombol:**
| Tombol | Fungsi |
|--------|--------|
| Kiri (di halaman 1) | Kembali ke menu |
| Kiri (di halaman 2) | Halaman sebelumnya |
| Kanan | Halaman berikutnya |

---

## Pengaturan Alarm Buzzer

### Membuka Settings

Menu → **Settings**

### Tampilan Settings

```
┌─────────────────────────┐
│Alarm Settings          │
│─────────────────────────│
│>Alarm: NONAKTIF        │
│ TempMax: 40.0          │
│ TempMin:  0.0          │
│ HumMaks: 85.0          │
│─────────────────────────│
│< >Atur  Ctr:Lanjut     │
└─────────────────────────┘
```

### Field yang Tersedia

| Field | Default | Satuan | Keterangan |
|-------|---------|--------|------------|
| Alarm | NONAKTIF | — | Master switch alarm |
| TempMax | 40.0 | °C | Alarm jika suhu di atas nilai ini |
| TempMin | 0.0 | °C | Alarm jika suhu di bawah nilai ini |
| HumMaks | 85.0 | % | Alarm jika kelembaban di atas nilai ini |
| dBMaks | 80.0 | dB | Alarm jika kebisingan di atas nilai ini |
| VoltMin | 3.3 | V | Alarm jika tegangan baterai di bawah nilai ini |
| BatMin | 3.2 | V | Tegangan yang dibaca sebagai **0%** (baterai kosong) |
| BatMax | 3.7 | V | Tegangan yang dibaca sebagai **100%** (baterai penuh) |
| **[ SIMPAN ]** | — | — | Simpan semua pengaturan |

> **Kalibrasi indikator baterai:** `BatMin`/`BatMax` memetakan tegangan terukur
> ke persentase indikator. Kalau ganti baterai dengan karakteristik berbeda
> (mis. masih kuat di 3.0 V), turunkan `BatMin` agar indikator tidak cepat 0%.
> Nilai default (3.2–3.7 V) sama dengan perilaku lama, jadi setelan kamu tidak
> berubah sampai diatur ulang.

### Cara Mengatur

1. Tekan **Tengah** untuk pindah ke field berikutnya
2. Di field yang ingin diubah:
   - **Kiri**: kurangi nilai / set NONAKTIF (untuk field Alarm)
   - **Kanan**: tambah nilai / set AKTIF (untuk field Alarm)
3. Navigasi ke field **[ SIMPAN ]** dengan tekan Tengah berulang
4. Tekan **Tengah** di field `[ SIMPAN ]` → pengaturan tersimpan ke memori (NVS)

> **Catatan:** Tekan **Long Tengah (2.5 detik)** kapan saja untuk keluar tanpa menyimpan.

### Perilaku Alarm

- Alarm **default NONAKTIF** — harus diaktifkan manual dari Settings
- Alarm tidak berbunyi saat baterai sedang **charging** (tegangan fluktuatif)
- Alarm berbunyi selama **3 pembacaan berturut-turut** melebihi threshold (hysteresis)
- Tekan tombol **Kiri** di halaman utama untuk **mute alarm** (snooze 30 menit)
- Kirim command `silence` via MQTT topic `derabot/command` untuk mute dari jarak jauh

### Data MQTT yang Dipublish

```json
{
  "temperature": 28.5,
  "humidity": 65.2,
  "pressure": 1013.1,
  "soundDb": 45.3,
  "voltage": 3.85,
  "batteryPct": 87,
  "rssi": -65,
  "signalPct": 70,
  "timestamp": 12345678
}
```

---

## Struktur Project

```
esp32-derabot/
├── platformio.ini              # Konfigurasi build & library
├── README.md                   # Dokumentasi ini
└── src/
    ├── config/
    │   └── config.h            # ← SEMUA konfigurasi di sini
    ├── domain/
    │   ├── entities/           # Data structures (SensorData, ThresholdConfig)
    │   └── interfaces/         # Abstract interfaces (IBuzzer, IDisplay, dll)
    ├── application/            # Business logic (use cases)
    │   ├── SensorUseCase       # Baca semua sensor
    │   ├── WifiUseCase         # Manajemen koneksi WiFi
    │   ├── MqttUseCase         # Publish/subscribe MQTT
    │   └── BuzzerUseCase       # Logika alarm & threshold
    ├── infrastructure/         # Implementasi hardware
    │   ├── sensors/            # BME280, INMP441, VoltSensor
    │   ├── display/            # OLED SSD1306
    │   ├── buzzer/             # PWM Buzzer
    │   ├── mqtt/               # HiveMQ TLS client
    │   ├── wifi/               # ESP WiFi Manager
    │   └── storage/            # NVS (Non-Volatile Storage)
    ├── presentation/           # UI layer
    │   ├── UIManager           # Koordinator layar & navigasi
    │   ├── InputHandler        # State machine tombol
    │   └── screens/            # Layar individual
    │       ├── MainScreen      # Halaman utama + menu
    │       ├── WifiPortalScreen # Setup WiFi via portal HP
    │       ├── SensorScreen    # Detail data sensor
    │       ├── CompassScreen   # Kompas GY-271
    │       ├── StockScreen     # Bursa IHSG
    │       └── SettingsScreen  # Pengaturan alarm
    └── main.cpp                # Setup & loop utama
```

---

## Troubleshooting

### Device tidak terdeteksi

**Linux:**
```bash
# Cek apakah device muncul:
ls /dev/ttyACM*

# Cek log kernel:
dmesg | grep -i usb | tail -20

# Pastikan user ada di group uucp:
groups $USER
```

**Windows:**
- Buka Device Manager → cek apakah ada device dengan tanda seru (!)
- Coba port USB berbeda
- Install driver Zadig (lihat bagian Upload Windows)

### Upload gagal: "Permission denied"

```bash
# Linux — sementara:
sg uucp -c "~/.local/bin/pio run --target upload"

# Linux — permanen (butuh logout):
sudo usermod -aG uucp $USER
```

### Upload gagal: "Port not found"

```bash
# Cek port yang tersedia:
pio device list

# Edit platformio.ini sesuai port:
upload_port = /dev/ttyACM1   # Linux
# upload_port = COM4          # Windows
```

### OLED tidak muncul

1. Cek wiring GPIO8 (SDA) dan GPIO9 (SCL)
2. Cek tegangan VCC OLED = 3.3V
3. Buka Serial Monitor — cari baris `[I2C] device at 0x3C`
4. Jika tidak ada: periksa koneksi, coba tambah pull-up 4.7kΩ

### BME280 gagal init

1. Cek wiring I2C (shared bus dengan OLED)
2. Serial Monitor → cek baris `[BME280]`
3. Coba alamat alternatif: ubah `I2cAddr::BME280` dari `0x76` ke `0x77`

### WiFi gagal terhubung

1. Pastikan SSID dan password benar (case-sensitive)
2. Periksa jarak ke router
3. Jika tersimpan di NVS tapi salah, masuk Settings → WiFi Setup → pilih jaringan baru

### MQTT tidak terhubung

1. Pastikan ESP32 sudah terhubung WiFi terlebih dahulu
2. Cek `HOST`, `USER`, `PASS` di `config.h`
3. Pastikan port 8883 tidak diblokir oleh firewall router
4. Serial Monitor → cari baris `[MQTT]` untuk detail error

### Alarm tidak bisa dimatikan

```
Menu → Settings → Alarm: NONAKTIF → [ SIMPAN ]
```

Atau kirim MQTT command:
```
Topic: derabot/command
Payload: silence
```

### Reset pengaturan ke default

Pengaturan alarm tersimpan di NVS (flash). Untuk reset:
1. Masuk Settings
2. Set semua nilai ke default
3. Set Alarm ke NONAKTIF
4. Tekan Center di `[ SIMPAN ]`

---

## Catatan Pengembangan

- Firmware dibangun dengan **Clean Architecture** — domain logic tidak bergantung pada hardware
- Sensor dibaca non-blocking (I2S) untuk menjaga responsivitas UI
- Tombol menggunakan state machine dengan debounce 15ms dan guard `WaitRelease` setelah transisi layar
- Semua pengaturan (WiFi credentials, alarm threshold) disimpan di NVS dan bertahan setelah restart
