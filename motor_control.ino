
const int driverPUL = 7; //PUL- pin
const int driverDIR = 6; //DIR- pin
// const int spd = A0; // Potentiometer (Analog Read)
const int limitSwitchMotorSide = 3; // motor side limit switch pin
const int limitSwitchFarSide = 2; // far side limit switch pin

// const int calibrateSwtich; -> will need these three switches
// const int centerSwitch;
const int polishLimitSwitch = 8;


// steps per rev -> 16 microsteps per step (really 8 because of the way I'm stepping), 1.8 degrees per true step -> 800 microsteps/revolution
const int stepsPerRev = 800; // depends on microstepping mode ->  (One of my programmed steps is really 2 steps)
const long InchesPerRev = 0.0787402; // from screw pitch
const long halfPlatformWidth = 3.0625; // from gantry plate
const long inchesTravel = 32.125; // after considering gantry plate width, limit switch buffer
const long centerPoint = 163195; // center of actuator in steps

const int pd = 65; // pulse 65 microseconds
boolean setdir = HIGH; // set direction, LOW = Backwards, HIGH = Forwards


long currentStepPosition; // even though long can hold decimals, these should only ever store ints, otherwise it will cause problems with functions
long targetStepPosition;

// to track 
static long interruptLastTime = 0;

// to check whether motor is in calibrating mode
bool calibrating = false;

// to check whether polishing function is finding workpiece boundary
bool findingPolishBoundary = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); // open the serial port at 9600 bps:
  pinMode(driverPUL, OUTPUT);
  pinMode(driverDIR, OUTPUT);
  pinMode(limitSwitchFarSide, INPUT_PULLUP);
  pinMode(limitSwitchMotorSide, INPUT_PULLUP);
  pinMode(polishLimitSwitch, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(limitSwitchFarSide), limitFarSideTrigger, FALLING); // when we push the push button, we pull pin 2 low, triggering this fall -> only works on 2 pins
  attachInterrupt(digitalPinToInterrupt(limitSwitchMotorSide), limitMotorSideTrigger, FALLING); // when we push the push button, we pull pin 3 low, triggering this fall -> only works on 2 pins
  delay(500);
  setZeroPosition();
  // setTargetStepPosition(10000);
  interruptLastTime = -75; // trick to make sure that if we motor starts close to limit switch that it still triggers
  calibrate();
  delay(5000); // wait 5 seconds
  center();
  polish();
}

void loop() {
  // runToTarget();
  // setdir = HIGH;
  // step();
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
      return;
    }
}

void limitMotorSideTrigger() {
  // Debounce ...
  if (millis() - interruptLastTime < 75) {
    return;
  } else {
    Serial.println("Motor Side Limit Switch Triggered!");
    setZeroPosition(); // set this as zero position 
    setdir = HIGH;
    for (int i = 0; i < 10000; ++i) { // move 10000 steps off of limit switch
      step();
    }
    setTargetStepPosition(currentStepPosition); // reset target position -> this will stop a polishing job ... 
    interruptLastTime = millis();
    calibrating = false;
  }
}

void limitFarSideTrigger() {
  if (millis() - interruptLastTime < 75) {
    return;
  } else {
    // setZeroPosition(); // set this as zero position -> Can set it to theoretical step position if I calculate, don't need it for now
    Serial.println("Far Side Limit Switch Triggered!");
    setdir = LOW;
    for (int i = 0; i < 10000; ++i) { // move 10000 steps off of limit switch
      step();
    }
    setTargetStepPosition(currentStepPosition); // reset target position -> this will stop a polishing job
    interruptLastTime = millis();
  }
}

// void revMotor() {
//   delay(150);
//   setdir = !setdir;
//   Serial.println("Reversing!");
//   delay(30);
//   setTargetStepPosition(currentStepPosition - 30000);
// }

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
  // Serial.println((String)"Set position target to: " + getTargetPosition() + " | Current position is " + getCurrentPosition());
}
// Sets the current position as the zero point
void setZeroPosition() {
  currentStepPosition = 0;
  Serial.println("Set Zero Position!");
}

// Sets current position as some value we input
// input should be an integer b/c we can't do decimal steps, so we will oscillate permanently
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
  setTargetStepPosition(centerPoint); // this is what it should be theoretically ((32.125/2) / inchesPerRev / stepsperrev)
  Serial.println("Centering!");
  bool centered = runToTarget();
  while (!centered) {
    // Serial.println("Trapped?");
    centered = runToTarget();
  }
  Serial.println("CENTERED");
}

// motor moves all the way to motor side limit switch and resets its' zero positon, then moves 5 revolutions off the limit switch
void calibrate() {
  Serial.println("Calibrating!");
  calibrating = true;
  while (calibrating) {
    run(false);
  }
}

// bounce between workpiece boundary limit switches (for now can extend to travel later)
// assume always starting in center, 
void polish() {
    Serial.println("Starting Polishing Job");
    bool findingPolishBoundary = true;
    long boundary_close;
    while (findingPolishBoundary) {
      bool pushed = digitalRead(polishLimitSwitch);
      if (pushed == LOW) {
        findingPolishBoundary = false;
        boundary_close = currentStepPosition;
      }
      run(false); // run motor backwards until we hit workpiece limit switch
    }

    Serial.println((String)"boundary_close is: " + boundary_close);
    long workpiece_length = 2*(centerPoint - boundary_close);
    long boundary_far = currentStepPosition + workpiece_length;
    Serial.println((String)"Workpiece length: " + workpiece_length);
    const int numberOfBounces = 3;

    for (int i = 0; i < numberOfBounces; ++i) { // i here determines number of bounces
      setTargetStepPosition(boundary_far); // run to far boundary
      bool arrived = runToTarget();
      while (!arrived) {
        arrived = runToTarget();
      }
      setTargetStepPosition(boundary_close);
      arrived = runToTarget();
      while (!arrived) {
        arrived = runToTarget();
      }
      Serial.println((String) "Finished Round " + (i + 1) + " of " +  numberOfBounces );
    }
    
    Serial.print("DONE WITH POLISHING JOB!");
}

void getCurrentPosition() {
  long tmp = currentStepPosition / stepsPerRev; // gives the number of revolutions relative to zero point
  tmp = tmp * InchesPerRev; // gives inches relative to zero point -> KEEP IN MIND THAT ZERO POINT IS HALF PLATFORM WIDTH AWAY FROM LIMIT SWITCH
  return tmp;
}

void getTargetPosition() {
  long tmp = targetStepPosition / stepsPerRev; // gives the number of revolutions of target position relative to zero pt, can be negative/positive
  tmp = tmp * InchesPerRev; // gives inches to target position -> KEEP IN MIND THAT ZERO POINT IS HALF PLATFORM WIDTH AWAY FROM LIMIT SWITCH
  return tmp;
}

