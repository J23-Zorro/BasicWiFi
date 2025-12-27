#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ESPmDNS.h>

class WiFiFSManager {
public:
    struct WiFiConfig {
        String mode; // "STA" lub "AP"
        String ssid;
        String pass;
        String apSsid;
        String apPass;
    };

    explicit WiFiFSManager(uint16_t port = 80, bool debug = true);

    bool begin(const char* defaultApSsid = "ESP32_AP",
               const char* defaultApPass = "12345678",
               const char* mdnsHostname = "esp32fs",
               bool formatIfFail = true);

    void handle();

    bool setWiFiSTA(const String& ssid, const String& pass);
    bool setWiFiAP(const String& ssid, const String& pass);

    void setFileAuth(const String& user, const String& pass);

    String ipString() const;
    WiFiConfig getConfig() const;
    void printStatus() const;

private:
    WebServer _server;
    bool _debug;
    WiFiConfig _cfg;

    String _fileAuthUser;
    String _fileAuthPass;

    String _mdnsHostname;

    bool mountFS(bool formatIfFail);
    void ensureDefaultWebFiles();
    bool readWiFiConfig();
    bool writeWiFiConfig();
    bool readAuthConfig();
    bool writeAuthConfig();
    String contentTypeFor(const String& path) const;
    String humanSize(size_t bytes) const;

    bool startWiFiFromConfig(const char* defaultApSsid, const char* defaultApPass);
    bool connectSTA(const String& ssid, const String& pass);
    bool startAP(const String& ssid, const String& pass);

    void setupRoutes();
    void handleRoot();
    void handleFilesPage();
    void handleWiFiPage();
    void handleAuthPage();
    void handleAuthSave();
    void handleFileList();
    void handleFileListJson();
    void handleStatusJson();
    void handleFileView();
    void handleFileDelete();
    void handleFileUpload();
    void handleWiFiSave();
    void handleStaticOr404();

    bool ensureFileAuth();
    bool requireFileAuth();
    size_t calcUsedBytes();
};
