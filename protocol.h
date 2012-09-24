#include <stdint.h>


#define ON 0
#define OFF 1
#define SHOW 2
#define FADETO 3
#define RANDOM 4
#define BLINK 5


typedef struct _Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;
} Color;

typedef struct _FadeCommand {
    uint8_t type;
    Color color;
    uint16_t duration;
} FadeCommand;

typedef struct _RandomCommand {
    uint8_t type;
    uint8_t s;
    uint8_t l;
    uint16_t duration;
} RandomCommand;

typedef struct _BlinkCommand {
    uint8_t type;
    Color on_color;
    Color off_color;
    uint16_t on_duration;
    uint16_t off_duration;
    uint8_t repeat;
} BlinkCommand;

typedef union _DimCommand {
    uint8_t type;
    FadeCommand fade;
    RandomCommand random;
    BlinkCommand blink;
} DimCommand;

void printColor(Color color) {
        Serial.print(color.r); Serial.print(' ');
        Serial.print(color.g); Serial.print(' ');
        Serial.print(color.b); Serial.print(' ');
        Serial.print(color.w);
}

void printCommand(DimCommand cmd) {
    if (cmd.type == OFF) {
        Serial.println("OFF");
    } else if (cmd.type == ON) {
        Serial.println("ON");
    } else if (cmd.type == FADETO) {
        Serial.print("FADETO ");
        printColor(cmd.fade.color); Serial.print(' ');
        Serial.print(cmd.fade.duration); Serial.println();
    } else if (cmd.type == RANDOM) {
        Serial.println("RANDOM ");
        Serial.print(cmd.random.s); Serial.print(' ');
        Serial.print(cmd.random.l); Serial.print(' ');
        Serial.print(cmd.random.duration); Serial.println();
    } else if (cmd.type == BLINK) {
        Serial.print("BLINK ");
        printColor(cmd.blink.on_color); Serial.print(" for ");
        Serial.print(cmd.blink.on_duration); Serial.print(' ');
        printColor(cmd.blink.off_color); Serial.print(" for ");
        Serial.print(cmd.blink.off_duration); Serial.print(" repeat ");
        Serial.print(cmd.blink.repeat); Serial.println();
    } else {
        Serial.println("Error: unknown cmd");
    }
}
