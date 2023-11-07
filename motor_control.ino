
const int driverPUL = 7; //PUL- pin
const int driverDIR = 6; //DIR- pin
// const int spd = A0; // Potentiometer (Analog Read)
const int limitSwitchMotorSide = 3; // motor side limit switch pin
const int limitSwtichFarSide = 2; // far side limit switch pin

// const int calibrateSwtich;
// const int centerSwitch;

// steps per rev -> 16 microsteps per step (really 8 because of the way I'm stepping), 1.8 degrees per true step -> 800 microsteps/revolution
const int stepsPerRev = 800; // depends on microstepping mode -> ONLY BECAUSE OF THE WAY I AM COUNTING STEPS (One of my programmed steps is really 2 steps)
const long InchesPerRev = 0.0787402; // from screw pitch // ALSO CHECKED THIS
const long halfPlatformWidth = 3.0625; // from gantry plate
const long inchesTravel = 32.125; // after considering gantry plate width, limit switch buffer

const int pd = 65; // pulse 65 microseconds
bool setdir = LOW; // set direction, LOW = Backwards, HIGH = Forwards

// ALL POSITIONS WILL REFERENCE THE CENTER OF THE MOUNT.  THIS MEANS THAT THE ZERO POSITION IS NOT WHERE THE LIMIT SWITCH IS, 
// BUT RATHER HALF THE MOUNT LENGTH AWAY FROM THE MOTORSIDE LIMIT SWICH
long currentStepPosition;
long targetStepPosition;

// to track 
static long interruptLastTime = 0;

// to check whether motor is in calibrating mode
bool calibrating = false;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); // open the serial port at 9600 bps:
  pinMode(driverPUL, OUTPUT);
  pinMode(driverDIR, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(limitSwitchFarSide), limitFarSideTrigger, FALLING); // when we push the push button, we pull pin 2 low, triggering this fall -> only works on 2 pins
  attachInterrupt(digitalPinToInterrupt(limitSwitchMotorSide), limitMotorSideTrigger, FALLING); // when we push the push button, we pull pin 2 low, triggering this fall -> only works on 2 pins
  setZeroPosition();
  setTargetStepPosition(10000);
  // calibrate();
  // center();
  interruptLastTime = -75; // trick to make sure that if we motor starts close to limit switch that it still triggers
}

void loop() {
  runToTarget();
}

void step() {
    digitalWrite(driverDIR, setdir);
    digitalWrite(driverPUL,HIGH);
    delayMicroseconds(pd);
    digitalWrite(driverPUL,LOW);
    delayMicroseconds(pd);
    delay(.125); // can control speed really well here
    if (setdir == HIGH) {
      currentStepPosition++;
    } else {
      currentStepPosition--;
    }

    if (currentStepPosition == targetStepPosition) {
      Serial.println("Reached Target Position!");
      Serial.println((String)"Position is: " + currentStepPosition);
    }
}

void limitMotorSideTrigger() {
  Serial.println("Motor Side Limit Switch Triggered!");
  // Debounce ...
  if (millis() - interruptLastTime < 75) {
    return;
  } else {
    setZeroPosition(); // set this as zero position 
    setdir = HIGH;
    for (int i = 0; i < 10000; ++i) { // move 10000 steps off of limit switch
      step();
    }
    setTargetStepPosition(currentStepPosition); // reset target position
    interruptLastTime = millis();
    calibrating = false;
  }
}

void limitFarSideTrigger() {
  Serial.println("Far Side Limit Switch Triggered!");
  // Debounce ...
  if (millis() - interruptLastTime < 75) {
    return;
  } else {
    // setZeroPosition(); // set this as zero position -> Can set it to theoretical step position if I calculate, don't need it for now
    setdir = LOW;
    for (int i = 0; i < 10000; ++i) { // move 10000 steps off of limit switch
      step();
    }
    setTargetStepPosition(currentStepPosition); // reset target position
    interruptLastTime = millis();
  }
}

void revMotor() {
  delay(150);
  setdir = !setdir;
  Serial.println("Reversing!");
  delay(30);
  setTargetStepPosition(currentStepPosition - 30000);
}

// run forwards/backwards, without considering position
// bool direction: True runs Forwards, False runs back, DEFAULT = True
void run (bool direction = true) { 
  if (direction) {
    setdir = HIGH;
  } else {
    setdir = LOW;
  }
  step();
}

// run until we hit stepTarget
bool runToTarget() {
  if (currentStepPosition != targetStepPosition) {
    // Serial.println((String)"Distance to Position: " + (targetStepPosition - currentStepPosition));
    if (currentStepPosition < targetStepPosition ) {
      setdir = HIGH;
    } else {
      setdir = LOW;
    }
    step();
    return false;
  } else {
    return true;
  }

}

// sets target step position relative to 0 point
void setTargetStepPosition(long target) {
  targetStepPosition = target;
  Serial.println((String)"Set step target to: " + target + " | Current step position is " + currentStepPosition);
  Serial.println((String)"Set position target to: " + target + " | Current step position is " + currentStepPosition);
}
// Sets the current position as the zero point
void setZeroPosition() {
  currentStepPosition = 0;
  Serial.println("Set Zero Position!");
}

// Sets current position as some value we input
void setCurrentPosition(long position) {
  currentStepPosition = position;
  Serial.println((String)"Set Current Step Position To: " + position);
}

// stops all movement until either center/calibrate button is clicked.
void stop() {
  while (1 == 1) {
    // check for button presses to exit loops 
  }
}

// motor moves from current position, whatever it may be, to the center position
// calculate theoretical position tomrrow
void center() {
  setTargetStepPosition(163195); // this is what it should be theoretically, actually test it . . . 
  bool centered = runToTarget();
  while (!centered) {
    runToTarget();
  }
}

// motor moves all the way to motor side limit switch and resets its' zero positon, then moves 5 revolutions off the limit switch
void calibrate() {
  calibrating = true;
  while (calibrating) {
    run(false);
  }
}

// bounce between workpiece boundary limit switches (for now can extend to travel later)
void polish() {

}

void getCurrentPosition() {
  long tmp = currentStepPosition / stepsPerRev // gives the number of revolutions relative to zero point
  tmp = tmp * inchesPerRev // gives inches relative to zero point -> KEEP IN MIND THAT ZERO POINT IS HALF PLATFORM WIDTH AWAY FROM LIMIT SWITCH
  return tmp;
}

void getTargetPosition() {
  long tmp = targetStepPosition / stepsPerRev // gives the number of revolutions of target position relative to zero pt, can be negative/positive
  tmp = tmp * inchesPerRev // gives inches to target position -> KEEP IN MIND THAT ZERO POINT IS HALF PLATFORM WIDTH AWAY FROM LIMIT SWITCH
  return tmp;

}


    // digitalWrite(driverDIR,setdir);
    // digitalWrite(driverPUL,HIGH);
    // delayMicroseconds(pd);
    // digitalWrite(driverPUL,LOW);
    // delayMicroseconds(pd);
    // int sensorVal = digitalRead(8);
    // // Serial.println(sensorVal);
