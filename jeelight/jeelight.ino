#include <Ports.h>
#include <RF12.h>
#include "../protocol.h"
#include "color.h"

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
        i2c.write(w >> 2);
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


class FadeTask: public LightTask {
    byte r, g, b, w;
    float rTmp, gTmp, bTmp, wTmp;
    float rDist, gDist, bDist, wDist;
    int maxStep;
    int currentStep;
    float rStep, gStep, bStep, wStep;
    int stepDelay;
    unsigned long lastStepTime;
public:
    FadeTask(DimLight &light) :
        LightTask(light)
    {}

    void setTargetColor(const byte r, const byte g, const byte b, const byte w, const long duration=1000) {
        rDist = r - light.r;
        gDist = g - light.g;
        bDist = b - light.b;
        wDist = w - light.w;

        this->rTmp = light.r;
        this->gTmp = light.g;
        this->bTmp = light.b;
        this->wTmp = light.w;

        this->r = r;
        this->g = g;
        this->b = b;
        this->w = w;

        maxStep = max(max(max(abs(rDist), abs(gDist)), abs(bDist)), abs(wDist));
        if (maxStep == 0) {
            return;
        }

        currentStep = 0;
        rStep = rDist / maxStep;
        gStep = gDist / maxStep;
        bStep = bDist / maxStep;
        wStep = wDist / maxStep;

        stepDelay = duration / maxStep;
        lastStepTime = millis();
        active = true;
    }

public:
    bool step() {
        if (millis() - lastStepTime > stepDelay) {
            if (currentStep >= maxStep) {
                light.sendColor(r, g, b, w);
                active = false;
                return false;
            }
            currentStep += 1;
            rTmp += rStep;
            gTmp += gStep;
            bTmp += bStep;
            wTmp += wStep;
            light.sendColor(rTmp, gTmp, bTmp, wTmp);
            lastStepTime = millis();
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

        hls_to_rgb(h, (byte)(l * 255), (byte)(s * 255), &r, &g, &b);
        setTargetColor(r, g, b, 0, duration);
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
RandomFadeTask randomFadeTask(light);
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
            fadeTask.setTargetColor(cmd.r, cmd.g, cmd.b, cmd.w, cmd.duration);
            currentTask = &fadeTask;
        } else if (cmd.type == RANDOM) {
            randomFadeTask.setColorParams(cmd.r, cmd.g);
            randomFadeTask.setFadeDuration(cmd.duration);
            randomFadeTask.setRandomTargetColor();
            currentTask = &randomFadeTask;
        } else {
            Serial.println("Error: unknown cmd");
        }

        printCommand(cmd);
    }
    if (currentTask && currentTask->isActive()) {
        currentTask->step();
    }

}