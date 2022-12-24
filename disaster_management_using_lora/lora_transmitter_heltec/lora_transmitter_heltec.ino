TaskHandle_t Task1;
TaskHandle_t Task2;
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <SPI.h> // include libraries
#include <LoRa.h>

int button1 = 13;
int button2 = 25;
int button3 = 32;
int button4 = 33;
String outgoing; // outgoing message
String incomings;
byte msgCount = 0;        // count of outgoing messages
byte localAddress = 0xBB; // address of this device
byte destination = 0xFF;  // destination to send to
long lastSendTime = 0;    // last send time
int interval = 50;        // interval between sends
Adafruit_SSD1306 display(128, 64, &Wire, -1);
String message = "SENT BY LORA AT 4TH STREET ";
void setup()
{
    Serial.begin(115200);//begin the serial monitor
    SPI.begin(5, 19, 27, 18);//begin spi 
    LoRa.setPins(18, 14, 26);//set the pins of lora
    Wire.begin(4, 15);//begin the wifi
    pinMode(16, OUTPUT);
    pinMode(button1, INPUT_PULLUP);
    pinMode(button2, INPUT_PULLUP);
    pinMode(button3, INPUT_PULLUP);
    pinMode(button4, INPUT_PULLUP);
    digitalWrite(16, LOW);
    digitalWrite(16, HIGH);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    display.setRotation(2);//display is set vertical to make it horizontal we use this command
    /*running on dual core because this module doesnt support two way communication along with spi interface*/
    xTaskCreatePinnedToCore(Task1code, "Task1", 10000, NULL, 1, &Task1, 0);

    xTaskCreatePinnedToCore(Task2code, "Task2", 10000, NULL, 1, &Task2, 1);
    while (!Serial)
        ;

    Serial.println("LoRa Duplex");
    if (!LoRa.begin(465E6))
    { // initialize ratio at 915 MHz
        Serial.println("LoRa init failed. Check your connections.");
        while (true)
            ; // if failed, do nothing
    }

    Serial.println("LoRa init succeeded.");
}
void onReceive(int packetSize)
{
    if (packetSize == 0)
        return; // if there's no packet, return

    // read packet header bytes:
    int recipient = LoRa.read();       // recipient address
    byte sender = LoRa.read();         // sender address
    byte incomingMsgId = LoRa.read();  // incoming msg ID
    byte incomingLength = LoRa.read(); // incoming msg length

    String incoming = "";

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
                message += " NEED MEDICINE";
                sendMessage(message);
            }
            else if (digitalRead(25) == LOW)
            {
                message += " NEED DOCTOR";

                sendMessage(message);
            }

            else if (digitalRead(32) == LOW)
            {
                message += " NEED FOOD";
                sendMessage(message);
            }
            else if (digitalRead(33) == LOW)
            {
                message += " NEED SHELTER";
                sendMessage(message);
            }
            Serial.println(message);
            lastSendTime = millis(); // timestamp the message
            interval = random(50) + 100;
        }
        else
            message = "SENT BY LORA AT 4TH STREET ";
        // parse for a packet, and call onReceive with the result:
        onReceive(LoRa.parsePacket());
        vTaskDelay(10);
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

        if (incomings.indexOf("OK") != -1)
        {
            display.clearDisplay();
            display.setCursor(10, 0);
            display.println("RESPONSE NOTED");
            display.display();
            vTaskDelay(2000);
        }
        else
        {
            display.clearDisplay();
            display.setCursor(10, 0);
            display.println("NOT RESPONDED");
            display.display();
        }
        vTaskDelay(200);
    }
}

void loop()
{
}
