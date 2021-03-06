#include <Ports.h>
#include <RF12.h>
#include <inttypes.h>
#include "../protocol.h"

class SerialBuf {
public:
    char buf[128];
    static const byte size = 128;
    byte currentByte;
    byte bytesRead;

    SerialBuf() :
        currentByte(0), bytesRead(0)
    {}

    bool recvDone() {
        while (Serial.available()) {
            buf[currentByte] = Serial.read();
            if ((currentByte + 2) >= sizeof buf || buf[currentByte] == '\n' || buf[currentByte] == '\r') {
                buf[currentByte + 1] = '\0';
                bytesRead = currentByte;
                currentByte = 0;
                return true;
            }
            currentByte++;
        }
        return false;
    }
};


DimCommand cmd;
MilliTimer sendTimer;
bool pendingOutput = false;
byte onOff = 0;
SerialBuf buf;

void setup() {
    rf12_initialize(1, RF12_868MHZ, 143);

    Serial.begin(57600);
    Serial.println("Welcome to Lightdim!");
}


void loop() {
    rf12_recvDone();
    if (buf.recvDone()) {
        bool success = false;
        switch (buf.buf[0]) {
            case 'f':
                success = (
                    sscanf(buf.buf, "f %hhu %hhu %hhu %hhu %u",
                        &(cmd.fade.color.r), &(cmd.fade.color.g), &(cmd.fade.color.b), &(cmd.fade.color.w), &(cmd.fade.duration))
                    == 5
                );
                cmd.type = FADETO;
                break;
            case 'o':
                cmd.type = OFF;
                success = true;
                break;
            case 'i':
                cmd.type = ON;
                success = true;
                break;
            case 'r':
                cmd.type = RANDOM;
                success = (
                    sscanf(buf.buf, "r %hhu %hhu %u",
                        &(cmd.random.s), &(cmd.random.l), &(cmd.random.duration))
                    == 3
                );
                success = true;
                break;
            case 'b':
                success = (
                    sscanf(buf.buf, "b %hhu %hhu %hhu %hhu %u %hhu %hhu %hhu %hhu %u %u",
                        &(cmd.blink.on_color.r), &(cmd.blink.on_color.g), &(cmd.blink.on_color.b),
                        &(cmd.blink.on_color.w), &(cmd.blink.on_duration),
                        &(cmd.blink.off_color.r), &(cmd.blink.off_color.g), &(cmd.blink.off_color.b),
                        &(cmd.blink.off_color.w), &(cmd.blink.off_duration),
                        &(cmd.blink.repeat))
                    == 11
                );
                cmd.type = BLINK;
                break;
        }
        if (success) {
            printCommand(cmd);
            pendingOutput = true;
        } else {
            Serial.println("?");
        }
    }

    if (pendingOutput && rf12_canSend()) {
        rf12_sendStart(0, &cmd, sizeof cmd);
        Serial.println("sent!");
        pendingOutput = false;
    }
}
