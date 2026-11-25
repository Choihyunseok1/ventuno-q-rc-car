#include <Arduino.h>
#include <Arduino_HardwareServo.h>
#include <Arduino_RouterBridge.h>

// MDD3A channel A input uses PWM. Channel B input selects the direction.
constexpr pin_size_t kLeftPwm = D3;
constexpr pin_size_t kLeftDirection = D4;
constexpr pin_size_t kRightPwm = D6;
constexpr pin_size_t kRightDirection = D7;
constexpr pin_size_t kServo = D9;
constexpr pin_size_t kLeftA = D2, kLeftB = D8;
constexpr pin_size_t kRightA = D11, kRightB = D12;

// Verify polarity and steering center with the chassis lifted.
constexpr int kLeftDriveSign = 1, kRightDriveSign = -1;
constexpr int kLeftEncoderSign = 1, kRightEncoderSign = -1;
constexpr int kSteerCenterUs = 1500, kSteerTravelUs = 120, kSteerSign = 1;
constexpr int kDriveDuty = 77;                 // 30% of 255
constexpr int kRampStep = 10;
constexpr uint32_t kRampIntervalMs = 20;
constexpr uint32_t kCommandTimeoutMs = 300;
constexpr uint32_t kReverseDelayMs = 100;

struct Encoder {
  pin_size_t a, b;
  int sign;
  volatile uint8_t previous;
  volatile int32_t count;
};

Encoder leftEncoder{kLeftA, kLeftB, kLeftEncoderSign, 0, 0};
Encoder rightEncoder{kRightA, kRightB, kRightEncoderSign, 0, 0};
HardwareServo steering;
bool steeringReady = false;
int throttle = 0, steer = 0, driveDirection = 1, duty = 0;
bool commandActive = false;
uint32_t lastCommandMs = 0, lastRampMs = 0, stoppedMs = 0;

uint8_t encoderState(const Encoder &encoder) {
  return (digitalRead(encoder.a) << 1) | digitalRead(encoder.b);
}

void updateEncoder(Encoder &encoder) {
  static constexpr int8_t transitions[] = {
       0,  1, -1,  0,
      -1,  0,  0,  1,
       1,  0,  0, -1,
       0, -1,  1,  0,
  };
  const uint8_t state = encoderState(encoder);
  const int step = transitions[(encoder.previous << 2) | state] * encoder.sign;
  if ((step > 0 && encoder.count < INT32_MAX) || (step < 0 && encoder.count > INT32_MIN)) {
    encoder.count += step;
  }
  encoder.previous = state;
}

void onLeftEdge() { updateEncoder(leftEncoder); }
void onRightEdge() { updateEncoder(rightEncoder); }

void writeMotor(pin_size_t pwmPin, pin_size_t directionPin, int direction, int magnitude) {
  const bool reverse = direction < 0;
  // Equal input levels brake. In reverse, low time on A produces drive.
  analogWrite(pwmPin, reverse ? 255 - magnitude : magnitude);
  digitalWrite(directionPin, reverse ? HIGH : LOW);
}

void writeDrive(int magnitude) {
  writeMotor(kLeftPwm, kLeftDirection, driveDirection * kLeftDriveSign, magnitude);
  writeMotor(kRightPwm, kRightDirection, driveDirection * kRightDriveSign, magnitude);
}

void brake(uint32_t now) {
  if (duty > 0) {
    duty = 0;
    stoppedMs = now;
    writeDrive(0);
  }
}

void setSteering(int next) {
  if (next == steer) return;
  steer = next;
  if (steeringReady) {
    steering.writeMicroseconds(kSteerCenterUs + steer * kSteerTravelUs * kSteerSign);
  }
}

String setDrive(int nextThrottle, int nextSteer) {
  const uint32_t now = millis();
  if (!steeringReady || nextThrottle < -1 || nextThrottle > 1 || nextSteer < -1 || nextSteer > 1) {
    commandActive = false;
    throttle = 0;
    brake(now);
    setSteering(0);
    return String("ERR");
  }
  commandActive = true;
  lastCommandMs = now;
  throttle = nextThrottle;
  if (throttle == 0 || throttle != driveDirection) brake(now);
  setSteering(nextSteer);

  noInterrupts();
  const int32_t left = leftEncoder.count, right = rightEncoder.count;
  interrupts();
  return String(left) + "," + String(right);
}

void configureEncoder(Encoder &encoder, void (*handler)()) {
  pinMode(encoder.a, INPUT_PULLUP);
  pinMode(encoder.b, INPUT_PULLUP);
  encoder.previous = encoderState(encoder);
  attachInterrupt(digitalPinToInterrupt(encoder.a), handler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoder.b), handler, CHANGE);
}

void setup() {
  pinMode(kLeftPwm, OUTPUT);
  pinMode(kLeftDirection, OUTPUT);
  pinMode(kRightPwm, OUTPUT);
  pinMode(kRightDirection, OUTPUT);
  analogWriteResolution(8);
  writeDrive(0);
  configureEncoder(leftEncoder, onLeftEdge);
  configureEncoder(rightEncoder, onRightEdge);
  steering.attach(kServo, 1000, 2000);
  steeringReady = steering.attached();
  if (steeringReady) steering.writeMicroseconds(kSteerCenterUs);

  if (Bridge.begin()) {
    Bridge.provide_safe("rc_set_drive", setDrive);
  }
}

void loop() {
  const uint32_t now = millis();
  if (commandActive && now - lastCommandMs >= kCommandTimeoutMs) {
    commandActive = false;
    throttle = 0;
    brake(now);
    setSteering(0);
  }
  if (commandActive && throttle != 0 && now - lastRampMs >= kRampIntervalMs) {
    if (throttle != driveDirection && now - stoppedMs >= kReverseDelayMs) {
      driveDirection = throttle;
      writeDrive(0);
    }
    if (throttle == driveDirection && duty < kDriveDuty) {
      duty = min(duty + kRampStep, kDriveDuty);
      writeDrive(duty);
    }
    lastRampMs = now;
  }
  delay(2);
}
