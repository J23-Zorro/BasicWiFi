#include "WiFiFSManager.h"

static const char* WIFI_CFG_PATH = "/KonfigWiFi.txt";
static const char* AUTH_CFG_PATH = "/KonfigAuth.txt";

static const char* DEFAULT_STYLE_CSS =
"html,body{margin:0;padding:0;font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;background:#f4f9f9;color:#203040}"
"header{background:linear-gradient(90deg,#2e7d32,#0277bd);color:#fff;padding:14px 18px}"
"h1{margin:0;font-size:22px}"
"nav{background:#fff;border-bottom:1px solid #d0e0e0;padding:10px 18px}"
"nav a{margin-right:14px;color:#0277bd;text-decoration:none;font-weight:600}"
"nav a:hover{text-decoration:underline}"
"main{max-width:960px;margin:24px auto;padding:0 16px}"
"section{background:#fff;border:1px solid #d0e0e0;border-radius:10px;padding:16px;margin-bottom:18px}"
"table{border-collapse:collapse;width:100%}"
"th,td{border:1px solid #e0efef;padding:8px;text-align:left}"
"th{background:#e8f5e9}"
"input,select,button{font:inherit;padding:8px;margin:4px 0}"
"button{background:#2e7d32;color:#fff;border:none;border-radius:6px;padding:8px 12px}"
"button:hover{background:#1b5e20}"
".danger{background:#c62828}"
".danger:hover{background:#8e0000}"
"footer{max-width:960px;margin:0 auto 24px;padding:8px 16px;color:#5a7a8a}";

WiFiFSManager::WiFiFSManager(uint16_t port, bool debug)
: _server(port), _debug(debug) {
    _cfg.mode = "AP";
    _cfg.apSsid = "ESP32_AP";
    _cfg.apPass = "12345678";
    _fileAuthUser = "files";
    _fileAuthPass = "files123";
}

bool WiFiFSManager::begin(const char* defaultApSsid, const char* defaultApPass, const char* mdnsHostname, bool formatIfFail) {
    _mdnsHostname = mdnsHostname;
    if (!mountFS(formatIfFail)) return false;
    ensureDefaultWebFiles();
    readWiFiConfig();
    readAuthConfig();
    bool okWiFi = startWiFiFromConfig(defaultApSsid, defaultApPass);

    if (MDNS.begin(_mdnsHostname.c_str())) {
        if (_debug) Serial.printf("[WiFiFS] mDNS: http://%s.local/\n", _mdnsHostname.c_str());
        MDNS.addService("http", "tcp", 80);
    }

    setupRoutes();
    _server.begin();
    if (_debug) Serial.println(F("[WiFiFS] Serwer WWW start"));
    printStatus();
    return okWiFi;
}

void WiFiFSManager::handle() { _server.handleClient(); }

bool WiFiFSManager::mountFS(bool formatIfFail) {
    if (!LittleFS.begin(formatIfFail)) { if (_debug) Serial.println(F("[WiFiFS] LittleFS mount FAILED")); return false; }
    if (_debug) Serial.println(F("[WiFiFS] LittleFS mounted"));
    return true;
}

void WiFiFSManager::ensureDefaultWebFiles() {
    if (!LittleFS.exists("/style.css")) {
        File f = LittleFS.open("/style.css", "w");
        if (f) { f.write((const uint8_t*)DEFAULT_STYLE_CSS, strlen(DEFAULT_STYLE_CSS)); f.close(); }
        if (_debug) Serial.println(F("[WiFiFS] zapisano domyślny /style.css"));
    }
}

bool WiFiFSManager::readWiFiConfig() {
    if (!LittleFS.exists(WIFI_CFG_PATH)) return false;
    File f = LittleFS.open(WIFI_CFG_PATH, "r"); if (!f) return false;
    while (f.available()) {
        String line = f.readStringUntil('\n'); line.trim();
        if (line.length()==0 || line.startsWith("#")) continue;
        int eq=line.indexOf('='); if (eq<=0) continue;
        String key=line.substring(0,eq); key.trim();
        String val=line.substring(eq+1); val.trim();
        if (key=="mode") _cfg.mode=val; else if (key=="ssid") _cfg.ssid=val; else if (key=="pass") _cfg.pass=val; else if (key=="apSsid") _cfg.apSsid=val; else if (key=="apPass") _cfg.apPass=val;
    }
    f.close(); if (_debug) Serial.println(F("[WiFiFS] wczytano KonfigWiFi.txt")); return true;
}

bool WiFiFSManager::writeWiFiConfig() {
    File f = LittleFS.open(WIFI_CFG_PATH, "w"); if (!f) return false;
    f.printf("mode=%s\nssid=%s\npass=%s\napSsid=%s\napPass=%s\n", _cfg.mode.c_str(), _cfg.ssid.c_str(), _cfg.pass.c_str(), _cfg.apSsid.c_str(), _cfg.apPass.c_str());
    f.close(); if (_debug) Serial.println(F("[WiFiFS] zapisano KonfigWiFi.txt")); return true;
}

bool WiFiFSManager::readAuthConfig() {
    if (!LittleFS.exists(AUTH_CFG_PATH)) return false; File f=LittleFS.open(AUTH_CFG_PATH,"r"); if(!f) return false;
    while (f.available()) { String line=f.readStringUntil('\n'); line.trim(); if(line.length()==0||line.startsWith("#")) continue; int eq=line.indexOf('='); if(eq<=0) continue; String k=line.substring(0,eq); String v=line.substring(eq+1); v.trim(); if(k=="user") _fileAuthUser=v; else if(k=="pass") _fileAuthPass=v; }
    f.close(); if (_debug) Serial.println(F("[WiFiFS] wczytano KonfigAuth.txt")); return true;
}

bool WiFiFSManager::writeAuthConfig() {
    File f = LittleFS.open(AUTH_CFG_PATH, "w"); if(!f) return false;
    f.printf("user=%s\npass=%s\n", _fileAuthUser.c_str(), _fileAuthPass.c_str());
    f.close(); if (_debug) Serial.println(F("[WiFiFS] zapisano KonfigAuth.txt")); return true;
}

String WiFiFSManager::contentTypeFor(const String& path) const {
    if (path.endsWith(".htm")||path.endsWith(".html")) return "text/html";
    if (path.endsWith(".css")) return "text/css";
    if (path.endsWith(".js")) return "application/javascript";
    if (path.endsWith(".json")) return "application/json";
    if (path.endsWith(".txt")||path.endsWith(".csv")||path.endsWith(".ini")) return "text/plain";
    if (path.endsWith(".png")) return "image/png";
    if (path.endsWith(".gif")) return "image/gif";
    if (path.endsWith(".jpg")||path.endsWith(".jpeg")) return "image/jpeg";
    if (path.endsWith(".ico")) return "image/x-icon";
    if (path.endsWith(".svg")) return "image/svg+xml";
    return "application/octet-stream";
}

String WiFiFSManager::humanSize(size_t bytes) const { const char* u[]={"B","KB","MB"}; double v=bytes; int i=0; while(v>=1024.0 && i<2){v/=1024.0;i++;} char b[32]; snprintf(b,sizeof(b),"%.2f %s",v,u[i]); return String(b);} 

size_t WiFiFSManager::calcUsedBytes(){ size_t used=0; File root=LittleFS.open("/","r"); File f=root.openNextFile(); while(f){used+=f.size(); f=root.openNextFile();} return used; }

bool WiFiFSManager::startWiFiFromConfig(const char* defaultApSsid, const char* defaultApPass){ if(_cfg.apSsid.length()==0) _cfg.apSsid=defaultApSsid; if(_cfg.apPass.length()==0) _cfg.apPass=defaultApPass; if(_cfg.mode=="STA" && _cfg.ssid.length()>0) return connectSTA(_cfg.ssid,_cfg.pass); else return startAP(_cfg.apSsid,_cfg.apPass);} 

bool WiFiFSManager::connectSTA(const String& ssid, const String& pass){ WiFi.mode(WIFI_STA); WiFi.begin(ssid.c_str(), pass.c_str()); if(_debug) Serial.printf("[WiFiFS] Łączenie z SSID \"%s\"...\n", ssid.c_str()); unsigned long t=millis(); while(WiFi.status()!=WL_CONNECTED && millis()-t<15000){ delay(250); if(_debug) Serial.print("."); } Serial.println(); if(WiFi.status()==WL_CONNECTED){ if(_debug) Serial.printf("[WiFiFS] Połączono. IP: %s\n", WiFi.localIP().toString().c_str()); _cfg.mode="STA"; writeWiFiConfig(); return true; } else { if(_debug) Serial.println(F("[WiFiFS] Nie udało się połączyć. AP.")); return startAP(_cfg.apSsid,_cfg.apPass);} }

bool WiFiFSManager::startAP(const String& ssid, const String& pass){ WiFi.mode(WIFI_AP); bool ok=WiFi.softAP(ssid.c_str(), pass.c_str()); delay(100); if(_debug){ Serial.printf("[WiFiFS] AP %s: %s\n", ssid.c_str(), ok?"OK":"FAILED"); Serial.printf("[WiFiFS] AP IP: %s\n", WiFi.softAPIP().toString().c_str()); } _cfg.mode="AP"; writeWiFiConfig(); return ok; }

void WiFiFSManager::setupRoutes(){
    _server.on("/", HTTP_GET, std::bind(&WiFiFSManager::handleRoot, this));
    _server.on("/files", HTTP_GET, std::bind(&WiFiFSManager::handleFilesPage, this));
    _server.on("/wifi", HTTP_GET, std::bind(&WiFiFSManager::handleWiFiPage, this));
    _server.on("/auth", HTTP_GET, std::bind(&WiFiFSManager::handleAuthPage, this));

    _server.on("/api/files", HTTP_GET, std::bind(&WiFiFSManager::handleFileList, this));
    _server.on("/api/files.json", HTTP_GET, std::bind(&WiFiFSManager::handleFileListJson, this));
    _server.on("/api/status.json", HTTP_GET, std::bind(&WiFiFSManager::handleStatusJson, this));

    _server.on("/view", HTTP_GET, std::bind(&WiFiFSManager::handleFileView, this));
    _server.on("/delete", HTTP_POST, std::bind(&WiFiFSManager::handleFileDelete, this));
    _server.on("/upload", HTTP_POST, [](){}, std::bind(&WiFiFSManager::handleFileUpload, this));

    _server.on("/wifi/save", HTTP_POST, std::bind(&WiFiFSManager::handleWiFiSave, this));
    _server.on("/auth/save", HTTP_POST, std::bind(&WiFiFSManager::handleAuthSave, this));

    _server.onNotFound(std::bind(&WiFiFSManager::handleStaticOr404, this));
}

bool WiFiFSManager::ensureFileAuth(){ if(_fileAuthUser.length()==0) _fileAuthUser="files"; if(_fileAuthPass.length()==0) _fileAuthPass="files123"; return true; }

bool WiFiFSManager::requireFileAuth(){ ensureFileAuth(); if(!_server.authenticate(_fileAuthUser.c_str(), _fileAuthPass.c_str())){ _server.requestAuthentication(BASIC_AUTH, "ESP32WiFiFS"); return false; } return true; }

void WiFiFSManager::handleRoot(){ String h="<!doctype html><html lang='pl'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>ESP32 – WiFi & LittleFS</title><link rel='stylesheet' href='/style.css'></head><body>"; h+="<header><h1>ESP32 – WiFi & LittleFS</h1></header>"; h+="<nav><a href='/'>Strona główna</a><a href='/files'>Pliki</a><a href='/wifi'>Ustawienia WiFi</a><a href='/auth'>Uwierzytelnianie</a></nav>"; h+="<main><section><h2>Status</h2>"; h+="<p>Tryb: "+_cfg.mode+"</p><p>IP: "+ipString()+"</p>"; if(_cfg.mode=="STA"){ h+="<p>SSID: "+_cfg.ssid+"</p>"; h+=String("<p>RSSI: ")+String(WiFi.RSSI())+String(" dBm</p>"); } else { h+="<p>AP SSID: "+_cfg.apSsid+"</p>"; } h+=String("<p>mDNS: ")+_mdnsHostname+String(".local</p>"); h+="<p>API: <a href='/api/status.json'>/api/status.json</a></p>"; h+="</section></main><footer><small>&copy; 2025 ESP32WiFiFS</small></footer></body></html>"; _server.send(200, "text/html", h);} 

void WiFiFSManager::handleFilesPage(){ if(!requireFileAuth()) return; String h="<!doctype html><html lang='pl'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Pliki – ESP32</title><link rel='stylesheet' href='/style.css'></head><body><header><h1>Pliki w LittleFS</h1></header><nav><a href='/'>Strona główna</a><a href='/wifi'>Ustawienia WiFi</a><a href='/auth'>Uwierzytelnianie</a></nav><main><section><h2>Lista plików</h2><table><thead><tr><th>Ścieżka</th><th>Rozmiar</th><th>Akcje</th></tr></thead><tbody>"; File root=LittleFS.open("/","r"); File f=root.openNextFile(); while(f){ String path=f.name(); size_t size=f.size(); String safePath=path; safePath.replace("+", "%2B"); h+="<tr><td><a href='/view?path="+path+"'>"+path+"</a></td><td>"+humanSize(size)+"</td><td><form method='POST' action='/delete' style='display:inline'><input type='hidden' name='path' value='"+safePath+"'><button type='submit' class='danger'>Usuń</button></form> <a href='"+path+"' download>Pobierz</a></td></tr>"; f=root.openNextFile(); } h+="</tbody></table></section><section><h2>Wgraj plik</h2><form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='upload'><br><button type='submit'>Wyślij</button></form><p>Uwaga: pliki zapisywane są w katalogu głównym LittleFS.</p></section></main><footer><small>&copy; 2025 ESP32WiFiFS</small></footer></body></html>"; _server.send(200,"text/html",h);} 

void WiFiFSManager::handleWiFiPage(){ String h="<!doctype html><html lang='pl'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Ustawienia WiFi – ESP32</title><link rel='stylesheet' href='/style.css'></head><body><header><h1>Ustawienia WiFi</h1></header><nav><a href='/'>Strona główna</a><a href='/files'>Pliki</a><a href='/auth'>Uwierzytelnianie</a></nav><main><section><h2>Bieżący status</h2>"; h+="<p>Tryb: "+_cfg.mode+"</p><p>IP: "+ipString()+"</p>"; if(_cfg.mode=="STA") h+="<p>SSID: "+_cfg.ssid+"</p>"; h+="</section><section><h2>Konfiguracja</h2><form method='POST' action='/wifi/save'><label>Tryb: <select name='mode'>"; h+=String("<option value='STA' ")+String(_cfg.mode=="STA"?"selected":"")+String(">STA</option>"); h+=String("<option value='AP' ")+String(_cfg.mode=="AP"?"selected":"")+String(">AP</option>"); h+="</select></label><br><label>SSID (STA): <input type='text' name='ssid' value='"+_cfg.ssid+"'></label><br><label>Hasło (STA): <input type='password' name='pass' value='"+_cfg.pass+"'></label><br><label>AP SSID: <input type='text' name='apSsid' value='"+_cfg.apSsid+"'></label><br><label>AP Hasło: <input type='password' name='apPass' value='"+_cfg.apPass+"'></label><br><button type='submit'>Zapisz i zastosuj</button></form></section></main><footer><small>&copy; 2025 ESP32WiFiFS</small></footer></body></html>"; _server.send(200, "text/html", h);} 

void WiFiFSManager::handleAuthPage(){ if(!requireFileAuth()) return; String h="<!doctype html><html lang='pl'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Uwierzytelnianie – ESP32</title><link rel='stylesheet' href='/style.css'></head><body><header><h1>Uwierzytelnianie operacji na plikach</h1></header><nav><a href='/'>Strona główna</a><a href='/files'>Pliki</a><a href='/wifi'>Ustawienia WiFi</a></nav><main><section><h2>Bieżące dane</h2><p>Użytkownik: <strong>"+_fileAuthUser+"</strong></p></section><section><h2>Zmień login/hasło</h2><form method='POST' action='/auth/save'><label>Nowy użytkownik: <input type='text' name='user' value='"+_fileAuthUser+"'></label><br><label>Nowe hasło: <input type='password' name='pass' placeholder='min. 4 znaki'></label><br><button type='submit'>Zapisz</button></form><p>Po zmianie przeglądarka może ponownie poprosić o hasło.</p></section></main><footer><small>&copy; 2025 ESP32WiFiFS</small></footer></body></html>"; _server.send(200,"text/html",h);} 

void WiFiFSManager::handleAuthSave(){ if(!requireFileAuth()) return; String user=_server.arg("user"); user.trim(); String pass=_server.arg("pass"); pass.trim(); if(user.length()==0){ _server.send(400,"text/plain","Użytkownik nie może być pusty"); return;} if(pass.length()<4){ _server.send(400,"text/plain","Hasło musi mieć min. 4 znaki"); return;} _fileAuthUser=user; _fileAuthPass=pass; writeAuthConfig(); _server.sendHeader("Location","/files",true); _server.send(302,"text/plain",""); }

void WiFiFSManager::handleFileList(){ if(!requireFileAuth()) return; handleFilesPage(); }

void WiFiFSManager::handleFileListJson(){ if(!requireFileAuth()) return; String j="{\"files\":["; bool first = true; File root=LittleFS.open("/","r"); File f=root.openNextFile();
    while(f){ if(!first) j+=","; first = false; j+="{\"path\":\""+String(f.name())+"\",\"size\":"+String((unsigned long)f.size())+"}"; f=root.openNextFile(); }
    j+="],\"usedBytes\":"+String((unsigned long)calcUsedBytes())+"}"; _server.send(200,"application/json",j);} 

void WiFiFSManager::handleStatusJson(){ String j="{"; j+="\"mode\":\""+_cfg.mode+"\","; j+="\"ip\":\""+ipString()+"\","; if(_cfg.mode=="STA"){ j+="\"ssid\":\""+_cfg.ssid+"\","; j+="\"rssi\":"+String(WiFi.RSSI())+","; } else { j+="\"apSsid\":\""+_cfg.apSsid+"\","; } j+="\"mdns\":\""+_mdnsHostname+".local\"}"; _server.send(200,"application/json",j);} 

void WiFiFSManager::handleFileView(){ String path=_server.arg("path"); if(path.length()==0 || !LittleFS.exists(path)){ _server.send(404,"text/html","<html><body><h3>Plik nie istnieje</h3></body></html>"); return;} String ct=contentTypeFor(path); File f=LittleFS.open(path,"r"); _server.streamFile(f,ct); f.close(); }

void WiFiFSManager::handleFileDelete(){ if(!requireFileAuth()) return; String path=_server.arg("path"); path.trim(); if(path.length() && path[0]!='/') path = "/" + path; bool exists = LittleFS.exists(path); if(!exists && path.indexOf(' ')!=-1){ String alt = path; alt.replace(" ", "+"); if(LittleFS.exists(alt)){ path = alt; exists = true; } }
    if(!exists){ _server.send(404,"text/plain","Nie znaleziono pliku"); if(_debug) Serial.printf("[WiFiFS] delete miss: '%s'\n", path.c_str()); return; } bool ok=LittleFS.remove(path); if(_debug) Serial.printf("[WiFiFS] usuwanie %s: %s\n", path.c_str(), ok?"OK":"FAILED"); _server.sendHeader("Location","/files",true); _server.send(302,"text/plain",""); }

void WiFiFSManager::handleFileUpload(){ if(!requireFileAuth()) return; HTTPUpload& upload=_server.upload(); static File up; if(upload.status==UPLOAD_FILE_START){ String filename=upload.filename; if(!filename.startsWith("/")) filename="/"+filename; up=LittleFS.open(filename,"w"); if(_debug) Serial.printf("[WiFiFS] upload start: %s\n", filename.c_str()); } else if(upload.status==UPLOAD_FILE_WRITE){ if(up) up.write(upload.buf, upload.currentSize); } else if(upload.status==UPLOAD_FILE_END){ if(up) up.close(); if(_debug) Serial.printf("[WiFiFS] upload end (%u bytes)\n", upload.totalSize); _server.sendHeader("Location","/files",true); _server.send(302,"text/plain",""); } else if(upload.status==UPLOAD_FILE_ABORTED){ if(up) up.close(); if(_debug) Serial.println(F("[WiFiFS] upload aborted")); _server.send(500,"text/plain","Upload przerwany"); } }

void WiFiFSManager::handleWiFiSave(){ String mode=_server.arg("mode"); mode.trim(); String ssid=_server.arg("ssid"); ssid.trim(); String pass=_server.arg("pass"); pass.trim(); String apSsid=_server.arg("apSsid"); apSsid.trim(); String apPass=_server.arg("apPass"); apPass.trim(); if(mode!="STA" && mode!="AP") mode="AP"; _cfg.mode=mode; if(ssid.length()) _cfg.ssid=ssid; _cfg.pass=pass; if(apSsid.length()) _cfg.apSsid=apSsid; if(apPass.length()) _cfg.apPass=apPass; writeWiFiConfig(); if(mode=="STA") connectSTA(_cfg.ssid,_cfg.pass); else startAP(_cfg.apSsid,_cfg.apPass); _server.sendHeader("Location","/wifi",true); _server.send(302,"text/plain",""); }

void WiFiFSManager::handleStaticOr404(){ String path=_server.uri(); if(LittleFS.exists(path)){ File f=LittleFS.open(path,"r"); _server.streamFile(f,contentTypeFor(path)); f.close(); } else { _server.send(404,"text/html","<!doctype html><html><body><h3>404 – Nie znaleziono</h3><p>"+path+"</p></body></html>"); } }

bool WiFiFSManager::setWiFiSTA(const String& ssid, const String& pass){ _cfg.mode="STA"; _cfg.ssid=ssid; _cfg.pass=pass; writeWiFiConfig(); return connectSTA(ssid,pass);} 

bool WiFiFSManager::setWiFiAP(const String& ssid, const String& pass){ _cfg.mode="AP"; _cfg.apSsid=ssid; _cfg.apPass=pass; writeWiFiConfig(); return startAP(ssid,pass);} 

void WiFiFSManager::setFileAuth(const String& user, const String& pass){ _fileAuthUser=user; _fileAuthPass=pass; writeAuthConfig(); }

String WiFiFSManager::ipString() const{ if(_cfg.mode=="STA" && WiFi.status()==WL_CONNECTED) return WiFi.localIP().toString(); return WiFi.softAPIP().toString(); }

WiFiFSManager::WiFiConfig WiFiFSManager::getConfig() const { return _cfg; }

void WiFiFSManager::printStatus() const{ Serial.printf("[WiFiFS] Tryb: %s\n", _cfg.mode.c_str()); if(_cfg.mode=="STA"){ Serial.printf("[WiFiFS] SSID: %s, status: %d, RSSI: %d dBm\n", _cfg.ssid.c_str(), WiFi.status(), WiFi.RSSI()); } else { Serial.printf("[WiFiFS] AP SSID: %s\n", _cfg.apSsid.c_str()); } Serial.printf("[WiFiFS] IP: %s\n", ipString().c_str()); Serial.printf("[WiFiFS] mDNS: %s.local\n", _mdnsHostname.c_str()); }
