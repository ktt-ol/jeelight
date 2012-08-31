#include <stdint.h>


#define ON 0
#define OFF 1
#define SHOW 2
#define FADETO 3
#define RANDOM 4

struct DimCommand {
    uint8_t type;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;
    uint16_t duration;
};

void printCommand(DimCommand cmd) {
    if (cmd.type == OFF) {
        Serial.println("OFF");
    } else if (cmd.type == ON) {
        Serial.println("ON");
    } else if (cmd.type == FADETO) {
        Serial.print("FADETO ");
        Serial.print(cmd.r); Serial.print(' ');
        Serial.print(cmd.g); Serial.print(' ');
        Serial.print(cmd.b); Serial.print(' ');
        Serial.print(cmd.w); Serial.print(' ');
        Serial.print(cmd.duration); Serial.println(' ');
    } else if (cmd.type == RANDOM) {
        Serial.println("RANDOM");
        Serial.print(cmd.r); Serial.print(' ');
        Serial.print(cmd.g); Serial.print(' ');
        Serial.print(cmd.duration); Serial.println(' ');
    } else {
        Serial.println("Error: unknown cmd");
    }
}
