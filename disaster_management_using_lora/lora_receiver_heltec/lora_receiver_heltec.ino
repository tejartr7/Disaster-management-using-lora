TaskHandle_t Task1;
TaskHandle_t Task2;
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <SPI.h> // include libraries
#include <LoRa.h>

int button1 = 13;
int button2 = 25;
String outgoing; // outgoing message
String incomings;
byte msgCount = 0;        // count of outgoing messages
byte localAddress = 0xFF; // address of this device
byte destination = 0xBB;  // destination to send to
long lastSendTime = 0;    // last send time
int interval = 50;        // interval between sends
Adafruit_SSD1306 display(128, 64, &Wire, -1);
String incoming = "";
void setup()
{
    Serial.begin(115200);     // begin the serial monitor and intialize the baud rate
    SPI.begin(5, 19, 27, 18); // begin the spi interface
    LoRa.setPins(18, 14, 26); // calling the lora interface
    Wire.begin(4, 15);
    pinMode(16, OUTPUT);
    pinMode(13, INPUT_PULLUP);
    digitalWrite(16, LOW);
    digitalWrite(16, HIGH);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    /*running on dual core because this module doesnt support 
    two way communication along with spi interface*/
    xTaskCreatePinnedToCore(Task1code, "Task1", 10000, NULL, 1, &Task1, 0);
    xTaskCreatePinnedToCore(Task2code, "Task2", 10000, NULL, 1, &Task2, 1);
    while (!Serial)
        ;

    Serial.println("LoRa Duplex");

    if (!LoRa.begin(465E6))
    { // initialize the frequency
        Serial.println("LoRa init failed. Check your connections.");
        while (true)
            ; // if failed, do nothing
    }

    Serial.println("LoRa init succeeded.");
}
void onReceive(int packetSize)
{
    incoming = "";
    if (packetSize == 0)
        return; // if there's no packet, return

    // read packet header bytes:
    int recipient = LoRa.read();       // recipient address
    byte sender = LoRa.read();         // sender address
    byte incomingMsgId = LoRa.read();  // incoming msg ID
    byte incomingLength = LoRa.read(); // incoming msg length

    while (LoRa.available())
    {
        incoming += (char)LoRa.read();
    }
    if (incomingLength != incoming.length())
    { // check length for error
        // Serial.println("error: message length does not match length");
        ;
        return; // skip rest of function
    }

    // if the recipient isn't this device or broadcast,
    if (recipient != localAddress && recipient != 0xFF)
    {
        // Serial.println("This message is not for me.");
        ;
        return; // skip rest of function
    }

    // if message is for this device, or broadcast, print details:
    Serial.println("Received from: 0x" + String(sender, HEX));
    Serial.println("Sent to: 0x" + String(recipient, HEX));
    Serial.println("Message ID: " + String(incomingMsgId));
    Serial.println("Message length: " + String(incomingLength));
    Serial.println("Message: " + incoming);
    incomings = incoming;
    Serial.println("RSSI: " + String(LoRa.packetRssi()));
    Serial.println("Snr: " + String(LoRa.packetSnr()));
    Serial.println();
}
void sendMessage(String outgoing)
{
    LoRa.beginPacket();            // start packet
    LoRa.write(destination);       // add destination address
    LoRa.write(localAddress);      // add sender address
    LoRa.write(msgCount);          // add message ID
    LoRa.write(outgoing.length()); // add payload length
    LoRa.print(outgoing);          // add payload
    LoRa.endPacket();              // finish packet and send it
    msgCount++;                    // increment message ID
}
void Task1code(void *parameter)
{
    Serial.print("Task1 is running on core ");
    Serial.println(xPortGetCoreID());

    for (;;)
    {
        if (millis() - lastSendTime > interval)
        {

            if (digitalRead(13) == LOW)
            {
                String message = "OK";
                sendMessage(message);
            }
            if (digitalRead(13) == HIGH)
            {
                String message = "NOT responded";
                sendMessage(message);
            }

            lastSendTime = millis(); // timestamp the message
            interval = random(50) + 100;
        }
        // parse for a packet, and call onReceive with the result:
        onReceive(LoRa.parsePacket());
        vTaskDelay(50);
        // vTaskDelay(200);
    }
}
void Task2code(void *parameter)
{
    Serial.print("Task2 is running on core ");
    Serial.println(xPortGetCoreID());

    for (;;)
    {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 20);
        Serial.println(incomings);
        display.display();
    }
    vTaskDelay(200);
}

void loop()
{
}
