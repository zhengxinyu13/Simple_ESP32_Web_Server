#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid     = "Grayson";
const char* password = "Zhengxinyu13@";

bool led_state = 0;
const int led_pin = 2;


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
        <title>ESP32 WebSocket Server</title>
        <style>
            html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; }
            body { margin-top: 50px; }
            h1 { color: #444444; margin: 50px auto; }
            p { font-size: 19px; color: #888; }
            #state { font-weight: bold; color: #444; }
            .switch { margin: 25px auto; width: 80px; }
            .toggle { display: none; }
            .toggle + label { display: block; position: relative; cursor: pointer; outline: 0; user-select: none; padding: 2px; width: 80px; height: 40px; background-color: #ddd; border-radius: 40px; }
            .toggle + label:before, .toggle + label:after { display: block; position: absolute; top: 1px; left: 1px; bottom: 1px; content: ""; }
            .toggle + label:before { right: 1px; background-color: #f1f1f1; border-radius: 40px; transition: background .4s; }
            .toggle + label:after { width: 40px; background-color: #fff; border-radius: 20px; box-shadow: 0 2px 5px rgba(0, 0, 0, .3); transition: margin .4s; }
            .toggle:checked + label:before { background-color: #4285f4; }
            .toggle:checked + label:after { margin-left: 42px; }
            .footer-text { font-size: 30px; }
            .bold-text { font-weight: bold !important; }
        </style>
    </head>
    <body>
        <h1>ESP32 WebSocket Server</h1>
        <div class="switch">
            <input id="toggle-btn" class="toggle" type="checkbox" %CHECK%>
            <label for="toggle-btn"></label>
        </div>
        <p>On-board LED: <span id="state">%STATE%</span></p>

        <script>
            window.addEventListener('load', function() {
                var websocket = new WebSocket(`ws://${window.location.hostname}/ws`);
                websocket.onopen = function(event) {
                    console.log('Connection established');
                }
                websocket.onclose = function(event) {
                    console.log('Connection died');
                }
                websocket.onerror = function(error) {
                    console.log('error');
                };
                websocket.onmessage = function(event) {
                    if (event.data == "1") {
                        document.getElementById('state').innerHTML = "ON";
                        document.getElementById('toggle-btn').checked = true;
                    } else {
                        document.getElementById('state').innerHTML = "OFF";
                        document.getElementById('toggle-btn').checked = false;
                    }
                };

                document.getElementById('toggle-btn').addEventListener('change', function() { websocket.send('toggle'); });
            });
        </script>

        <div class="footer">
            <p class="footer-text">This software was written by <span class="bold-text">Grayson Zheng</span>.</p>
        </div>
    </body>
    </html>
)rawliteral";

void handle_websocket_message(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        if (strcmp((char*)data, "toggle") == 0) {
        led_state = !led_state;
        ws.textAll(String(led_state));
        }
    }
}

void event_handler(AsyncWebSocket *server, AsyncWebSocketClient *client,
                  AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type) {
        case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
        case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
        case WS_EVT_DATA:
        handle_websocket_message(arg, data, len);
        digitalWrite(led_pin, led_state);
        break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
        break;
    }
}

String processor(const String& var)
{
    if(var == "STATE")
        return led_state ? "ON" : "OFF";

    if(var == "CHECK")
        return led_state ? "checked" : "";

    return String();
}

void setup()
{
    Serial.begin(115200);
    pinMode(led_pin, OUTPUT);
    digitalWrite(led_pin, LOW);

    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.println("Connected..!");
    Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

    ws.onEvent(event_handler);
    server.addHandler(&ws);

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html, processor);
    });

    // Start server
    server.begin();
}

void loop()
{
    ws.cleanupClients();
}