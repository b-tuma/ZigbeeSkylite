/*
Zigbee Skylite
Made by Bruno Tuma
www.brunotuma.com/blog/skylite-mod
*/

#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "OneButton.h"
#include "Zigbee.h"

#define BUTTON_2_PIN 14
#define BUTTON_1_PIN 13
#define BUTTON_3_PIN 4
#define LED_B_PIN 11
#define LED_G_PIN 12
#define LED_R_PIN 0
#define MOTOR_PIN 1
#define LASER_PIN 3

OneButton ButtonPower = OneButton(BUTTON_1_PIN, true);
OneButton ButtonModes = OneButton(BUTTON_2_PIN, true);
OneButton ButtonBrightness = OneButton(BUTTON_3_PIN, true);

ZigbeeColorDimmableLight zbLight = ZigbeeColorDimmableLight(10);
ZigbeeDimmableLight zbLaser = ZigbeeDimmableLight(11);
ZigbeeAnalog zbMotor = ZigbeeAnalog(12);
ZigbeeAnalog zbEffect = ZigbeeAnalog(13);

const int PWM_FREQ = 5000;

uint16_t led_r_target = 0;
uint16_t led_g_target = 0;
uint16_t led_b_target = 0;
uint16_t laser_target = 0;
uint16_t led_r_current = 0;
uint16_t led_g_current = 0;
uint16_t led_b_current = 0;
uint16_t laser_current = 0;

uint32_t lastUpdateTime = 0;
bool zigbeeConnected = false;

float animationSpeed = 0;

float angleR = 0.0f;
float angleG = 0.0f;
float angleB = 0.0f;
// Spread out phases to look more natural
float phaseR = 0.0f;
float phaseG = 1.7f;
float phaseB = 3.9f;

const float MIN_FREQ = 0.00005236f; // (2.0f * PI) / 12000.0f -> 2 Min
const float MAX_FREQ = 0.00125664f; // (2.0f * PI) / 5000.0f -> 5 Secs
const float FREQ_RATIO = MAX_FREQ / MIN_FREQ;

void setLight(bool state, uint8_t red, uint8_t green, uint8_t blue, uint8_t level)
{
  if (!state || (red == 0 && green == 0 && blue == 0))
  {
    led_r_target = 0;
    led_g_target = 0;
    led_b_target = 0;
    return;
  }

  uint8_t biggest = red;
  if (blue > biggest)
    biggest = blue;
  if (green > biggest)
    biggest = green;

  led_r_target = map(red, 0, biggest, 0, 1023) * level / 255;
  led_g_target = map(green, 0, biggest, 0, 1023) * level / 255;
  led_b_target = map(blue, 0, biggest, 0, 1023) * level / 255;
}

void setLaser(bool state, uint8_t level)
{
  if (!state)
  {
    laser_target = 0;
    return;
  }
  laser_target = map(level, 0, 255, 0, 1023);
  return;
}

void setMotor(float level)
{
  if (level < 1.0)
  {
    ledcWrite(MOTOR_PIN, 0);
    return;
  }
  uint16_t scaledLevel = (uint16_t)(level * 500.0 / 100.0 + 0.5);

  ledcWrite(MOTOR_PIN, 523 + scaledLevel);
}

void setAnimation(float level)
{
  animationSpeed = constrain(level, 0.0f, 100.0f);
}

// Create a task on identify call to handle the identify function
void identify(uint16_t time)
{
  static uint8_t blink = 1;
  log_d("Identify called for %d seconds", time);
  if (time == 0)
  {
    // If identify time is 0, stop blinking and restore light as it was used for identify
    zbLight.restoreLight();
    return;
  }
  ledcWrite(LED_B_PIN, 255 * blink);
  blink = !blink;
}

/********************* Arduino functions **************************/
void setup()
{
  rgbLedWrite(RGB_BUILTIN, 80, 40, 0);

  ButtonPower.attachClick(togglePower);
  ButtonPower.setLongPressIntervalMs(3000);
  ButtonPower.attachLongPressStart(factoryReset);
  ButtonBrightness.setLongPressIntervalMs(1000);
  ButtonBrightness.attachClick(changeBrightness);
  ButtonBrightness.attachLongPressStart(toggleLaser);
  ButtonModes.setLongPressIntervalMs(1000);
  ButtonModes.attachLongPressStart(toggleMotor);
  ButtonModes.attachClick(toggleMode);

  ledcAttach(MOTOR_PIN, PWM_FREQ, 10);
  ledcAttach(LASER_PIN, PWM_FREQ, 10);
  ledcAttach(LED_B_PIN, PWM_FREQ, 10);
  ledcAttach(LED_G_PIN, PWM_FREQ, 10);
  ledcAttach(LED_R_PIN, PWM_FREQ, 10);

  // Set callback function for light change
  zbLight.onLightChange(setLight);
  zbLaser.onLightChange(setLaser);
  zbMotor.addAnalogOutput();
  zbMotor.setAnalogOutputApplication(ESP_ZB_ZCL_AI_RPM_OTHER);
  zbMotor.setAnalogOutputDescription("Rotation Speed");
  zbMotor.setAnalogOutputResolution(1);
  zbMotor.setAnalogOutputMinMax(0, 100);
  zbMotor.onAnalogOutputChange(setMotor);

  zbEffect.addAnalogOutput();
  zbEffect.setAnalogOutputApplication(ESP_ZB_ZCL_AI_PERCENTAGE_OTHER);
  zbEffect.setAnalogOutputDescription("Animation Speed");
  zbEffect.setAnalogOutputResolution(1);
  zbEffect.setAnalogOutputMinMax(0, 100);
  zbEffect.onAnalogOutputChange(setAnimation);

  // Optional: Set callback function for device identify
  zbLight.onIdentify(identify);

  // Optional: Set Zigbee device name and model
  zbLight.setManufacturerAndModel("BrunoTuma", "SkyLite");
  zbLight.setPowerSource(ZB_POWER_SOURCE_MAINS);

  // Add endpoint to Zigbee Core
  Zigbee.addEndpoint(&zbLaser);
  Zigbee.addEndpoint(&zbMotor);
  Zigbee.addEndpoint(&zbLight);
  Zigbee.addEndpoint(&zbEffect);

  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin())
  {
    rgbLedWrite(RGB_BUILTIN, 200, 0, 0);
    delay(300);
    ESP.restart();
  }
  rgbLedWrite(RGB_BUILTIN, 50, 50, 50);
}

void togglePower()
{
  bool isLightOn = zbLight.getLightState();
  bool isLaserOn = zbLaser.getLightState();
  if (isLightOn || isLaserOn)
  {
    zbLight.setLightState(false);
    zbLaser.setLightState(false);
    zbMotor.setAnalogOutput(0);
  }
  else
  {
    zbLight.setLightState(true);
    zbLaser.setLightState(true);
  }
}

void factoryReset()
{
  Zigbee.factoryReset();
}

void toggleMotor()
{
  bool isEmitting = zbLight.getLightState() || zbLaser.getLightState();
  if (!isEmitting)
  {
    return;
  }
  zbMotor.setAnalogOutput(zbMotor.getAnalogOutput() > 0 ? 0 : 100);
}

void changeBrightness()
{
  if (!zbLight.getLightState())
  {
    zbLight.setLightState(true);
  }
  zbLight.setLightLevel(zbLight.getLightLevel() + 50);
  zbLaser.setLightLevel(zbLaser.getLightLevel() + 50);
}

void toggleLaser()
{
  if (zbLaser.getLightState())
  {
    zbLaser.setLightState(false);
  }
  else
  {
    zbLaser.setLightState(true);
    zbLaser.setLightLevel(zbLight.getLightLevel());
  }
}

void toggleMode()
{
  uint8_t redLevel = random(0, 256);
  uint8_t blueLevel = random(0, 256);
  uint8_t greenLevel = random(0, 256);

  zbLight.setLightColor(redLevel, greenLevel, blueLevel);
}

// Fade between positions using deltatime, also clamps to 10-bit.
uint16_t moveTo(uint16_t current, uint16_t target, uint32_t delta)
{
  if (target > 1023)
    target = 1023;
  if (target > current)
  {
    uint32_t sum = current + delta;
    if (sum > 1023)
      sum = 1023;
    if (sum > target)
    {
      return target;
    }
    return (uint16_t)sum;
  }
  if (target < current)
  {
    int32_t sub = current - delta;
    if (sub < 0)
      sub = 0;
    if (sub < target)
    {
      return target;
    }
    return (uint16_t)sub;
  }
  return current;
}

float clampedIncrement(float value, float increment){
  float result = value + increment;
  if(result >= TWO_PI){
    return result - TWO_PI;
  }
  return result;
}

void ledTick()
{
  uint32_t currentTime = millis();
  uint32_t deltaTime = currentTime - lastUpdateTime;

  uint16_t adjusted_r = led_r_target;
  uint16_t adjusted_g = led_g_target;
  uint16_t adjusted_b = led_b_target;

  if (animationSpeed > 0.0f)
  {
    float logScale = log10f(animationSpeed);
    float scaleFactor = logScale / 2.0f;
    float frequency = MIN_FREQ * pow(FREQ_RATIO, scaleFactor);

    float freqR = frequency * 0.8f;
    float freqG = frequency;
    float freqB = frequency * 1.3f;

    angleR = clampedIncrement(angleR, deltaTime * freqR);
    angleG = clampedIncrement(angleG, deltaTime * freqG);
    angleB = clampedIncrement(angleB, deltaTime * freqB);
    phaseR = clampedIncrement(phaseR, deltaTime * 0.000005f);
    phaseG = clampedIncrement(phaseG, deltaTime * 0.00002f);
    phaseB = clampedIncrement(phaseB, deltaTime * 0.00006f);
    
    // Sine modulation from 20% to 100% of target.
    float rWave = (sinf(angleR + phaseR) + 1.0f) * 0.4f + 0.2f;
    float gWave = (sinf(angleG + phaseG) + 1.0f) * 0.4f + 0.2f;
    float bWave = (sinf(angleB + phaseB) + 1.0f) * 0.4f + 0.2f;

    adjusted_r = constrain((uint16_t)(led_r_target * rWave + 0.5f), 0, 1023);
    adjusted_g = constrain((uint16_t)(led_g_target * gWave + 0.5f), 0, 1023);
    adjusted_b = constrain((uint16_t)(led_b_target * bWave + 0.5f), 0, 1023);
  }

  // Always use transitions between colors
  led_r_current = moveTo(led_r_current, adjusted_r, deltaTime);
  led_g_current = moveTo(led_g_current, adjusted_g, deltaTime);
  led_b_current = moveTo(led_b_current, adjusted_b, deltaTime);
  laser_current = moveTo(laser_current, laser_target, deltaTime);

  ledcWrite(LED_R_PIN, led_r_current);
  ledcWrite(LED_G_PIN, led_g_current);
  ledcWrite(LED_B_PIN, led_b_current);
  ledcWrite(LASER_PIN, laser_current);

  // Update time
  lastUpdateTime = currentTime;
}

void loop()
{
  if (zigbeeConnected && !Zigbee.connected())
  {
    zigbeeConnected = false;
    rgbLedWrite(RGB_BUILTIN, 0, 0, 50);
    delay(10);
  }
  else if (!zigbeeConnected && Zigbee.connected())
  {
    zigbeeConnected = true;
    rgbLedWrite(RGB_BUILTIN, 0, 0, 0);
    delay(10);
  }
  ButtonPower.tick();
  ButtonModes.tick();
  ButtonBrightness.tick();
  ledTick();
}
