#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WebServer.h>
//#include "ESPAsyncWebServer.h"
#include <ESPmDNS.h>
#include <FS.h>
#include "SPIFFS.h"
#include <string>
#include "Helper.h"

typedef void (*gif_play_callback)(String filename);

class PandaWebServer{

public: 

    PandaWebServer(){}
    void setup();
    void update();
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

    int mode = 0;

    switch(mode){
      case 0:
        // Station Mode
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
        break;
        
      case 1:
      {
        // Soft AP Mode
        WiFi.softAP(ssid, password);
        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(myIP);
        break;
      }
      
      case 2:
        // SmartConfig Mode
        // http://www.iotsharing.com/2017/05/how-to-use-smartconfig-on-esp32.html        
        WiFi.mode(WIFI_AP_STA);
        WiFi.beginSmartConfig();
    
        //Wait for SmartConfig packet from mobile
        Serial.println("Waiting for SmartConfig.");
        while (!WiFi.smartConfigDone()) {
            delay(500);
            Serial.print(".");
        }
        break;

      default: 
        break;
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
    server.send(200, "text/plain", "Try to change gif");
}

void PandaWebServer::handleGifDelete() {

    Serial.println("handleGifDelete()");
    
    // make sure we get a file name as a URL argument
    if (server.args() == 0) {
        Serial.println("bad args");
        server.send(500, "text/plain", "BAD ARGS!"); 
        return;
    }
    
    String filename = server.arg(0);
    Serial.print("Try to delete " + filename + "...");
    
    // protect root path
    if (filename == "/") {
        Serial.println("Not allowed to delete root path");  
        server.send(500, "text/plain", "BAD PATH!"); 
        return;
    }

    // deny filename contains /
//    if(filename.indexOf("/") != -1){
//        Serial.println("filename can not contain /");
//        server.send(500, "text/plain", "BAD PATH!"); 
//        return;
//    }

    // check if the file exists
    String path = gifRoot + "/" + filename;
    if (!SPIFFS.exists(path)){
        Serial.println("file not found");
        server.send(404, "text/plain", "FILE NOT FOUND!"); 
        return;
    } 
    SPIFFS.remove(path);
    Serial.println("deleted");
    String msg = "deleted file: " + path;
    server.send(200, "text/plain", msg);

    // TODO
    // we have to notify to Gifplayer that this file is no longer exist
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

        // whitespace causes trouble so we replace it into underline '_'
        std::string str = std::string(upload.filename.c_str());
        replaceWhitespace(str);
        String filename(str.c_str());
        Serial.println(filename);
        
        String contentType = getContentType(filename);
        if(contentType != "image/gif"){
            server.send(500, "text/plain", "Prohibited to upload non-gif file!");
            Serial.println("Prohibited to upload non-gif file");
            return;
        }
        String path = gifRoot + "/" + filename;
        Serial.println("handleFileUpload Name: " + path);
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

    // TODO
    // we have to notify to Gifplayer about this new file
    
}

void PandaWebServer::handleGifList() {
    String output = "";
    String path = gifRoot;

    // Assuming there are no subdirectories
    File dir = SPIFFS.open(path);
    if(dir.isDirectory()){
        File file = dir.openNextFile();
        while (file)
        {
            // Separate by comma if there are multiple files
            if (output != ""){
                output += ",";
            }

            std::string filepath = file.name();
            std::string filename = getFilename(filepath);
                        
            output += String(filename.c_str());
            file.close();
            file = dir.openNextFile();
        }
    }
    Serial.println(output);        
    server.send(200, "text/plain", output);
}
