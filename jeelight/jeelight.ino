#include <Ports.h>
#include <RF12.h>
#include "../protocol.h"
#include "color.h"
#include "husl.h"

class DimLight {
protected:
     DeviceI2C i2c;
 public:
     byte r;
     byte g;
     byte b;
     byte w;
    DimLight(DeviceI2C i2c) :
        i2c(i2c)
    {};
    void sendColor(const byte r, const byte g, const byte b, const byte w) {
        this->r = r;
        this->g = g;
        this->b = b;
        this->w = w;
        i2c.send();
        i2c.write(0);
        i2c.write(r >> 2);
        i2c.write(g >> 2);
        i2c.write(b >> 2);
        i2c.write(128 >> 2);
        i2c.stop();
    }
};


class LightTask {
protected:
    bool active;
    DimLight &light;
public:
    LightTask(DimLight &light) :
        active(false), light(light)
    {}

    virtual bool step();

    void stop() {
        active = false;
    }

    bool isActive() {
        return active;
    }
};



class BlinkTask : public LightTask {
    byte rOn, gOn, bOn, wOn;
    byte rOff, gOff, bOff, wOff;
    unsigned int durationOn, durationOff;
    byte repeat;
    bool on;
    unsigned long nextSwitchTime;
    LightTask **currentTaskPointer;
    LightTask *previousTask;

public:
    BlinkTask(DimLight &light) :
        LightTask(light)
    {}

    void setOnColor(const byte r, const byte g, const byte b, const byte w, const long duration=1000) {
        rOn = r;
        gOn = g;
        bOn = b;
        wOn = w;
        durationOn = duration;
    };

    void setOffColor(const byte r, const byte g, const byte b, const byte w, const long duration=1000) {
        rOff = r;
        gOff = g;
        bOff = b;
        wOff = w;
        durationOff = duration;
    };

    void setRepeat(byte repeat) {
        this->repeat = repeat;
        active = true;
        on = true;
        light.sendColor(rOn, gOn, bOn, wOn);
        nextSwitchTime = millis() + durationOn;
    }

    void setPreviousTask(LightTask **currentTaskPointer, LightTask *previousTask) {
        this->currentTaskPointer = currentTaskPointer;
        this->previousTask = previousTask;
    }

    bool step() {
        if (nextSwitchTime > millis()) {
            return false;
        }
        if (repeat <= 0) {
            active = false;
            *currentTaskPointer = previousTask;
            return true;
        }

        if (on) {
            on = false;
            light.sendColor(rOff, gOff, bOff, wOff);
            nextSwitchTime = millis() + durationOff;
        } else {
            on = true;
            light.sendColor(rOn, gOn, bOn, wOn);
            nextSwitchTime = millis() + durationOn;
            repeat -= 1;
        }
        return false;
    };
};


class FadeTask: public LightTask {
    byte rTarget, gTarget, bTarget, wTarget;
    float rCurrent, gCurrent, bCurrent, wCurrent;
    int maxStep;
    int currentStep;
    float rStep, gStep, bStep, wStep;
    int stepDelay;
    unsigned long lastStepTime;
public:
    FadeTask(DimLight &light) :
        LightTask(light)
    {}

    void setTargetColor(byte r, byte g, byte b, byte w, const long duration=1000) {

        if (r == light.r && g == light.g && b == light.b && w == light.w) {
            // hit the same color, change w so that we don't get
            // division zero later
            if (w < 128) {
                w += 1;
            } else {
                w -= 1;
            }
        }
        // calculate distances between current and target colors
        float rDistance = r - light.r;
        float gDistance = g - light.g;
        float bDistance = b - light.b;
        float wDistance = w - light.w;

        this->rCurrent = light.r;
        this->gCurrent = light.g;
        this->bCurrent = light.b;
        this->wCurrent = light.w;

        this->rTarget = r;
        this->gTarget = g;
        this->bTarget = b;
        this->wTarget = w;

        maxStep = max(max(max(abs(rDistance), abs(gDistance)), abs(bDistance)), abs(wDistance));

        currentStep = 0;
        rStep = rDistance / maxStep;
        gStep = gDistance / maxStep;
        bStep = bDistance / maxStep;
        wStep = wDistance / maxStep;

        stepDelay = duration / maxStep;
        lastStepTime = millis();
        active = true;
    }

public:
    bool step() {
        while (millis() - lastStepTime > stepDelay) {
            if (currentStep >= maxStep) {
                light.sendColor(rTarget, gTarget, bTarget, wTarget);
                active = false;
                return false;
            }
            currentStep += 1;
            rCurrent += rStep;
            gCurrent += gStep;
            bCurrent += bStep;
            wCurrent += wStep;
            light.sendColor(rCurrent, gCurrent, bCurrent, wCurrent);
            lastStepTime += stepDelay;
        }
        return true;
    }
};

class RandomHUSLFadeTask: public FadeTask {
    byte lightness, saturation;
    static const byte defaultLightness = 50;
    static const byte defaultSaturation = 50;
    int duration;
public:
    RandomHUSLFadeTask(DimLight &light) :
        FadeTask(light), lightness(0), saturation(0),
        duration(15000)
    {
        setRandomTargetColor();
    }

    void setColorParams(byte lightness, byte saturation) {
        this->lightness = lightness;
        this->saturation = saturation;
    }
    void setFadeDuration(int duration) {
        this->duration = duration;
    }

    void setRandomTargetColor() {
        float r, g, b;
        byte l = lightness;
        byte s = saturation;

        float h = random(360);
        if (lightness ==  0) {
            l = defaultLightness;
        }
        if (saturation ==  0) {
            s = defaultSaturation;
        }

        HUSLtoRGB(&r, &g, &b, h, l, s);
        setTargetColor((byte)(r * 255), (byte)(g * 255), (byte)(b * 255), 0, duration);
        Serial.print("FADETO RANDOM ");
        Serial.print((byte)(r * 255)); Serial.print(' ');
        Serial.print((byte)(g * 255)); Serial.print(' ');
        Serial.print((byte)(b * 255)); Serial.print(' ');
        Serial.print(duration); Serial.println(' ');
    }

    bool step() {
        if (!FadeTask::step()) {
            setRandomTargetColor();
        }
        return true;
    }
};

class RandomFadeTask: public FadeTask {
    byte lightness, saturation;
    static const byte defaultLightness = 255 * 0.4;
    static const byte defaultSaturation = 255 * 0.8;
    int duration;
public:
    RandomFadeTask(DimLight &light) :
        FadeTask(light), lightness(0), saturation(0),
        duration(15000)
    {
        setRandomTargetColor();
    }

    void setColorParams(byte lightness, byte saturation) {
        this->lightness = lightness;
        this->saturation = saturation;
    }
    void setFadeDuration(int duration) {
        this->duration = duration;
    }

    void setRandomTargetColor() {
        byte r, g, b;
        byte l = lightness;
        byte s = saturation;

        float h = random(1024)/1024.0;
        if (lightness ==  0) {
            l = defaultLightness;
        }
        if (saturation ==  0) {
            s = defaultSaturation;
        }

        hsl_to_rgb(h, s / 255.0f, l / 255.0f, &r, &g, &b);
        setTargetColor(r, g, b, 0, duration);
        Serial.print("FADETO RANDOM ");
        Serial.print(r); Serial.print(' ');
        Serial.print(g); Serial.print(' ');
        Serial.print(b); Serial.print(' ');
        Serial.print(duration); Serial.println(' ');
    }

    bool step() {
        if (!FadeTask::step()) {
            setRandomTargetColor();
        }
        return true;
    }
};

const byte DAC_PORT = 1;
const byte DAC_ADDRESS = 0x27;

PortI2C dacBus (DAC_PORT);
DeviceI2C dac (dacBus, DAC_ADDRESS);

DimLight light(dac);

DimCommand cmd;

FadeTask fadeTask(light);
RandomHUSLFadeTask randomFadeTask(light);
BlinkTask blinkTask(light);
LightTask *currentTask;

void setup() {
    rf12_initialize(11, RF12_868MHZ, 143);

    pinMode(13, OUTPUT);

    Serial.begin(57600);
    Serial.println("Welcome to Lightdim Node!");
    if (dac.isPresent()) {
        Serial.println("DAC present!");
    } else {
        Serial.println("ERROR: DAC not present!");
    }

    currentTask = &randomFadeTask;
}

void loop() {
    if (rf12_recvDone() && rf12_crc == 0 && rf12_len == sizeof cmd) {
        memcpy(&cmd, (byte*) rf12_data, sizeof cmd);

        if (cmd.type == OFF) {
            light.sendColor(0, 0, 0, 0);
            currentTask = NULL;
        } else if (cmd.type == ON) {
            light.sendColor(255, 255, 255, 255);
            currentTask = NULL;
        } else if (cmd.type == FADETO) {
            fadeTask.setTargetColor(cmd.fade.color.r, cmd.fade.color.g, cmd.fade.color.b, cmd.fade.color.w, cmd.fade.duration);
            currentTask = &fadeTask;
        } else if (cmd.type == RANDOM) {
            randomFadeTask.setColorParams(cmd.random.s, cmd.random.l);
            randomFadeTask.setFadeDuration(cmd.random.duration);
            randomFadeTask.setRandomTargetColor();
            currentTask = &randomFadeTask;
        } else if (cmd.type == BLINK) {
            blinkTask.setOnColor(cmd.blink.on_color.r, cmd.blink.on_color.g, cmd.blink.on_color.b, cmd.blink.on_color.w, cmd.blink.on_duration);
            blinkTask.setOffColor(cmd.blink.off_color.r, cmd.blink.off_color.g, cmd.blink.off_color.b, cmd.blink.off_color.w, cmd.blink.off_duration);
            blinkTask.setRepeat(cmd.blink.repeat);
            blinkTask.setPreviousTask(&currentTask, currentTask);
            currentTask = &blinkTask;
        } else {
            Serial.println("Error: unknown cmd");
        }

        printCommand(cmd);
    }
    if (currentTask && currentTask->isActive()) {
        currentTask->step();
    }

}