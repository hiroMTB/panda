#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include "SPIFFS.h"

class PandaWebServer{

public: 

    PandaWebServer(){}
    void setup();
    void update();
    static String getContentType(String filename);
    static void handleFileDelete();
    static bool handleFileRead(String path);
    static void handleFileUpload();
    static void handleFileList();

    const char* ssid     = "yourssid";
    const char* password = "yourpasswd";
    
    static WebServer server;
    static File fsUploadFile;
};

WebServer PandaWebServer::server;
File PandaWebServer::fsUploadFile;

void PandaWebServer::setup(){
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp32")) {
        Serial.println("MDNS responder started");
    }

    // WebServer
    server.on("/list", HTTP_GET, handleFileList);
    server.on("/delete", HTTP_DELETE, handleFileDelete);
    server.on("/upload", HTTP_POST, []() {
        server.send(200, "text/plain", "{\"success\":1}");
    }, handleFileUpload);
    // called when the url is not defined
    server.onNotFound([]() {
        if (!handleFileRead(server.uri())) {
        server.send(404, "text/plain", "File Not Found!");
        }
    });
    server.begin();
    Serial.println("HTTP server started");

}


void PandaWebServer::update(){
    server.handleClient();
}

String PandaWebServer::getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    return "text/plain";
}

void PandaWebServer::handleFileDelete() {
    // make sure we get a file name as a URL argument
    if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS!");
    String path = server.arg(0);
    // protect root path
    if (path == "/") return server.send(500, "text/plain", "BAD PATH!");
    // check if the file exists
    if (!SPIFFS.exists(path)) return server.send(404, "text/plain", "FILE NOT FOUND!");
    SPIFFS.remove(path);
    Serial.println("DELETE: " + path);
    String msg = "deleted file: " + path;
    server.send(200, "text/plain", msg);
}

bool PandaWebServer::handleFileRead(String path) {
    // Serve index file when top root path is accessed
    if (path.endsWith("/")) path += "index.html";
    // Different file types require different actions
    String contentType = getContentType(path);

    if (SPIFFS.exists(path)) {
        fs::File file = SPIFFS.open(path, "r");
        if (contentType == "image/jpeg"){
        // do nothing
        }
        server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false; // if the file doesn't exist or can't be opened
}

void PandaWebServer::handleFileUpload() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if (!filename.startsWith("/"))
        filename = "/" + filename;
        Serial.print("handleFileUpload Name: "); Serial.println(filename);
        fsUploadFile = SPIFFS.open(filename, "w");
    } else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (fsUploadFile)
        fsUploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END)
    {
        if (fsUploadFile)
        fsUploadFile.close();
        Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
    }
}

void PandaWebServer::handleFileList() {
    String output = "[";
    String path = "/";

    // Assuming there are no subdirectories
    File dir = SPIFFS.open(path);
    if(dir.isDirectory()){
        File file = dir.openNextFile();
        while (file)
        {
        // Separate by comma if there are multiple files
        if (output != "[")
            output += ",";
        output += String(file.name()).substring(1);
        file.close();
        }
    }
    output += "]";
    server.send(200, "text/plain", output);
}
