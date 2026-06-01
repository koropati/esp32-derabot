#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// TEMPLATE CREDENTIALS — salin file ini menjadi config_secrets.h
// lalu isi dengan nilai asli Anda.
//
//   cp src/config/config_secrets.example.h src/config/config_secrets.h
//
// File config_secrets.h sudah masuk .gitignore dan TIDAK akan ter-commit.
// ─────────────────────────────────────────────────────────────────────────────

namespace Secrets {

namespace Mqtt {
    constexpr const char* HOST   = "xxxx.s1.eu.hivemq.cloud"; // broker URL
    constexpr int         PORT   = 8883;                       // TLS port
    constexpr const char* USER   = "your-username";
    constexpr const char* PASS   = "your-password";
    constexpr const char* CLIENT = "esp32-derabot";            // harus unik per device
}

} // namespace Secrets
