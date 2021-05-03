#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WebServer.h>
//#include "ESPAsyncWebServer.h"
#include <ESPmDNS.h>
#include <FS.h>
#include "SPIFFS.h"

typedef void (*gif_play_callback)(String filename);

class PandaWebServer{

public: 

    PandaWebServer(){}
    void setup();
    void update();
    static String getContentType(String filename);
    static void handleGifPlay();
    static void handleGifDelete();
    static bool handleFileRead(String path);
    static void handleGifUpload();
    static void handleGifList();

    void setGifPlayCallback(gif_play_callback cb);
    static gif_play_callback gifPlayCallback;

    const char* ssid     = "yourssid";
    const char* password = "yourpasswd";
    
    static WebServer server;
    //static AsyncWebServer server;
    static File fsUploadFile;

    static String gifRoot;
};

WebServer PandaWebServer::server;
//AsyncWebServer PandaWebServer::server;
File PandaWebServer::fsUploadFile;
String PandaWebServer::gifRoot = "/gifs";

gif_play_callback PandaWebServer::gifPlayCallback;


void PandaWebServer::setup(){

    // Station Mode
    //WiFi.mode(WIFI_STA);
    //WiFi.begin(ssid, password);

    // SmartConfig Mode
    // http://www.iotsharing.com/2017/05/how-to-use-smartconfig-on-esp32.html
    /*
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();

    //Wait for SmartConfig packet from mobile
    Serial.println("Waiting for SmartConfig.");
    while (!WiFi.smartConfigDone()) {
        delay(500);
        Serial.print(".");
    }
    */

    // Soft AP Mode
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

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

    server.on("/play", HTTP_POST, handleGifPlay);

    server.on("/list", HTTP_GET, handleGifList);
    server.on("/delete", HTTP_DELETE, handleGifDelete);
    server.on("/upload", HTTP_POST, []() {
        server.send(200, "text/plain", "{\"success\":1}");
    }, handleGifUpload);
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
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

void PandaWebServer::setGifPlayCallback(gif_play_callback cb){
    gifPlayCallback = cb;
}

void PandaWebServer::handleGifPlay(){
    if (server.args() == 0) {
        server.send(500, "text/plain", "BAD ARGS!");
        return;
    }
    String filename = server.arg(0);

    // check if the file exists
    String path = gifRoot + "/" + filename;
    if (!SPIFFS.exists(path)){
        server.send(404, "text/plain", "FILE NOT FOUND!"); 
        return;
    }

    // set gif filename to play
    gifPlayCallback(filename);
}

void PandaWebServer::handleGifDelete() {
    // make sure we get a file name as a URL argument
    if (server.args() == 0) {
        server.send(500, "text/plain", "BAD ARGS!"); 
        return;
    }
    
    String filename = server.arg(0);
    
    // protect root path
    if (filename == "/") {
        server.send(500, "text/plain", "BAD PATH!"); 
        return;
    }

    // deny filename contains /
    if(filename.indexOf("/") != -1){
        server.send(500, "text/plain", "BAD PATH!"); 
        return;
    }

    // check if the file exists
    String path = gifRoot + "/" + filename;
    if (!SPIFFS.exists(path)){
        server.send(404, "text/plain", "FILE NOT FOUND!"); 
        return;
    } 
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
        server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false; // if the file doesn't exist or can't be opened
}

void PandaWebServer::handleGifUpload() {
    
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;        
        String contentType = getContentType(filename);
        if(contentType != "image/gif"){
            server.send(500, "text/plain", "Prohibited to upload non-gif file!");
            Serial.println("Prohibited to upload non-gif file");
            return;
        }
        String path = gifRoot + "/" + filename;
        Serial.printf("handleFileUpload Name: %s\n", path); 
        fsUploadFile = SPIFFS.open(path, "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (fsUploadFile){
            fsUploadFile.write(upload.buf, upload.currentSize);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (fsUploadFile){
            fsUploadFile.close();
        }
        Serial.print("handleFileUpload Size: "); 
        Serial.println(upload.totalSize);
    }
}

void PandaWebServer::handleGifList() {
    String output = "[";
    String path = gifRoot;

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
            file = dir.openNextFile();
        }
    }
    output += "]";
    server.send(200, "text/plain", output);
}
