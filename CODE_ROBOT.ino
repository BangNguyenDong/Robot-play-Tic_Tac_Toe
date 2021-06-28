#include <math.h>
#include <AccelStepper.h>
#include <MultiStepper.h>

//driver for the axis 1 - X
#define PUL1_PIN 2
#define DIR1_PIN 5
//driver for the axis 2 - Y
#define PUL2_PIN 3
#define DIR2_PIN 6
//driver for the axis 3 - Z
#define PUL3_PIN 4
#define DIR3_PIN 7
//enable pin for the axis 1,2,3
#define EN1_PIN 8

AccelStepper stepper1(AccelStepper::DRIVER, PUL1_PIN, DIR1_PIN);
AccelStepper stepper2(AccelStepper::DRIVER, PUL2_PIN, DIR2_PIN);
AccelStepper stepper3(AccelStepper::DRIVER, PUL3_PIN, DIR3_PIN);
MultiStepper steppers;

boolean en = false;
const byte numChars = 100;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false;
float positions[3] = {0, 0, 0};
int microStep[3] = {16, 16, 16};
float angleStep[3] = {1.8, 1.8, 1.8};


float l2 = 200; // CHIỀU DÀI CÁNH TAY Ở TRONG
float l3 = 170; // CHIỀU DÀI CÁNH TAY Ở NGOÀI
float r = 20; // BAN KINH O
float a = 20; // CANH HINH VUONG CHỨA CHỮ Z CÓ CẠNH a*2
float up  =  4; // KHOẢNG DI CHUYỂN TRỤC Z
float dw  = -4; // CŨNG LÀ KHOẢNG DI CHUYỂN TRỤC Z NHƯNG ÂM
long curSpeed = 400; // TỐC ĐỘ CỦA 2 ĐỘNG CƠ XY
long curSpeedZ = 500; // TỐC ĐỘ CỦA ĐỘNG CƠ Z
int gear = 1; // TỈ SỐ CHUYỀN ĐAI
float s1, c1, s2, c2, t1, t2 ;
float x, x_or, y, k1, k2;
bool mode = true;


void setup()
{
  Serial.begin(9600);
  Serial.println("NHAP G1 DE VE O, G2 DE VE X " );
  Serial.println("VI DU G1 X100 Y150 " );
  delay(100);

  pinMode(PUL1_PIN, OUTPUT);
  pinMode(DIR1_PIN, OUTPUT);
  pinMode(PUL2_PIN, OUTPUT);
  pinMode(DIR2_PIN, OUTPUT);
  pinMode(PUL3_PIN, OUTPUT);
  pinMode(DIR3_PIN, OUTPUT);

  pinMode(EN1_PIN, OUTPUT);

  digitalWrite(DIR1_PIN, LOW);
  digitalWrite(DIR2_PIN, LOW);
  digitalWrite(DIR3_PIN, LOW);

  steppers.addStepper(stepper1);
  steppers.addStepper(stepper2);
  //  steppers.addStepper(stepper3);

  enableAll();
}

void loop()
{
  recvWithEndMarker();
  parseData(receivedChars);
}

void parseData(char * receivedChars_) {
  char g[5] = "";
  String g_ = "";
  char chars[10] = "";
  String stringValue = "";
  const char delim[2] = " ";
  if (newData == false) {
    return;
  }
  Serial.write(receivedChars_);
  Serial.write("\n");
  newData = false;
  // split the data into its parts

  char * token; // this is used by strtok() as an index

  token = strtok(receivedChars_, delim);     // get the first part - the string
  strcpy(g, token); //
  g_ = String(g);

  while (token != NULL && g_.startsWith("G1") ) {
    token  = strtok(NULL, delim); // this continues where the previous call left off
    strcpy(chars, token);
    stringValue = String(chars);
    if (stringValue.startsWith("X")) {
      positions[0] = toPosition(stringValue);
    } else if (stringValue.startsWith("Y")) {
      positions[1] = toPosition(stringValue);
    } else if (stringValue.startsWith("Z")) {
      positions[2] = toPosition(stringValue);
    }
    mode = true;
  }

  while (token != NULL && g_.startsWith("G2") ) {
    token  = strtok(NULL, delim); // this continues where the previous call left off
    strcpy(chars, token);
    stringValue = String(chars);
    if (stringValue.startsWith("X")) {
      positions[0] = toPosition(stringValue);
    } else if (stringValue.startsWith("Y")) {
      positions[1] = toPosition(stringValue);
    } else if (stringValue.startsWith("Z")) {
      positions[2] = toPosition(stringValue);
    }
    mode = false;
  }

  //set speed, en, disable
  while (token != NULL && g_.startsWith("G30")) {
    token  = strtok(NULL, delim); // this continues where the previous call left off
    strcpy(chars, token);
    stringValue = String(chars);
    if (stringValue.startsWith("F")) {
      curSpeed = microStep[0] * toPosition(stringValue);
    } else if (stringValue.startsWith("E0")) {
      disableAll();
    } else if (stringValue.startsWith("E1")) {
      enableAll();
    }

  }

  if (g_.startsWith("G1")) {
    runSteppers(positions);

  }

  if (g_.startsWith("G2")) {
    runSteppers(positions);

  }
}

void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }
    else {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void runSteppers(float pos[3])
{

  long steps[3] = {};
  long acc = 0.7 * curSpeed;
  stepper1.setMaxSpeed(curSpeed * 2.5);
  stepper2.setMaxSpeed(curSpeed * 2.5);
  //  stepper3.setMaxSpeed(curSpeed);

  x = pos[0] ;
  x_or = pos[0] ;
  pos[1] = -pos[1];
  y = pos[1];


  Serial.print("X: " );
  Serial.println (x);
  Serial.print ("Y: " );
  Serial.println ( y);

  if (mode == true) {

    draw_O(x, y);
  }
  if (mode == false) {

    draw_X(x, y);
  }

  homing();
}


float toPosition(String s) {
  String dataInS = s.substring(1, s.length());
  char buff[dataInS.length() + 1];
  dataInS.toCharArray(buff, dataInS.length() + 1);
  return atof(buff);
}

void step(boolean dir, byte dirPin, byte stepperPin, int steps)
{
  digitalWrite(dirPin, dir);
  delay(50);
  for (int i = 0; i < steps; i++) {
    digitalWrite(stepperPin, HIGH);
    delayMicroseconds(1000);
    digitalWrite(stepperPin, LOW);
    delayMicroseconds(1000);
  }
}

void truc_z_mm(float z) {

  float pos[3];
  long  steps[3] = {};
  long acc = 0.7 * curSpeed;

  pos[2] = (z / 8) * 360;

  steps[2] = pos[2] * microStep[2] / angleStep[2];


  if (en) {
    //    steppers.moveTo(steps);
    //    steppers.runSpeedToPosition();
    Serial.println(" Z IS MOVING");
    Serial.print(" Z STEP: ");
    Serial.println(steps[2] );

    stepper3.setSpeed(curSpeedZ);
    stepper3.setMaxSpeed(curSpeedZ);
    stepper3.setAcceleration(acc);
    stepper3.runToNewPosition(steps[2]);
    //    stepper3.runSpeedToPosition(steps[2]);
  }

}
void go_to(float x, float y) {

  float pos[3];
  long steps[3] = {};
  float n;

  if (x_or < -r) {
    n = x;
    x = abs(x);
  }

  // -----------------------ĐỘNG HỌC NGHỊCH------------------------------

  c2 = (pow(x , 2) + pow(y , 2) - pow(l2, 2) - pow(l3, 2)) / (2 * l2 * l3);
  s2 = sqrt(1 - pow(c2, 2));

  s1 = (x * (l2 + l3 * c2) + y  * l3 * s2);
  c1 = (-y  * (l2 + l3 * c2) + x  * l3 * s2);

  t2 = (atan2(s2, c2) * 180) / 3.14;
  t1 = (atan2(s1, c1) * 180) / 3.14;

  t2 = - t2;
  t1 = - t1;
  t1 = t1 + 90;


  if (x_or < -r) {
    if (n < 0) {
      t2 = -t2;
      t1 = t1 - 2 * (t1 - 90);
    }
  }


  Serial.print(" GOING TO t1:");
  Serial.println(t1);
  Serial.print(" GOING TO t2:");
  Serial.println(t2);


  pos[0] = t1;
  pos[1] = t2 + t1 ; //
  pos[2] =  0 ;

  steps[0] = pos[0] * microStep[0] * gear / angleStep[0];
  steps[1] = pos[1] * microStep[1] * gear / angleStep[1];
  steps[2] = pos[2] * microStep[2] / angleStep[2];



  if (en) {
    steppers.moveTo(steps);
    steppers.runSpeedToPosition();

  }
}



void homing ()
{

  float pos[3];
  long steps[3] = {};
  stepper1.setMaxSpeed(curSpeed * 2.5);
  stepper2.setMaxSpeed(curSpeed * 2.5);

  x = l2 + l3 ;
  y = 0;
  Serial.println(" ----------------------HOMING-----------------------");
  go_to(x, y);
  Serial.println(" ----------------------END HOMING-----------------------");



}

void line(float xa, float ya, float xb, float yb ) {
  float k;

  for (int t = 0; t < 10  ; t++ ) {

    //PHƯƠNG TRÌNH ĐƯỜNG THẲNG
    k = t;
    x = xa + (xb - xa) * (k / 10);
    y = ya + (yb - ya) * (k / 10);

    go_to( x, y );

  }
}

void circle(float xa, float ya, float r ) {

  float angle = 0;

  for (int t = 0; t < 660  ; t = t + 10 ) {

    //PHƯƠNG TRÌNH TRÒN
    angle = t;
    x = xa + r * cos(angle / 100);
    y = ya + r * sin(angle / 100);

    go_to( x, y );

  }
}

void draw_O(float x, float y) {

  Serial.println(" -----------------------DRAW O---------------------");

  float xa = x + 20 ;
  float ya = y   ;

  go_to(xa , ya);

  stepper1.setMaxSpeed(curSpeed );
  stepper2.setMaxSpeed(curSpeed );

  for (int i = 2; i > 0; i-- ) {
    delay(1000);
    Serial.println(i);
  }

  truc_z_mm( dw);
  circle(x, y, r);
  truc_z_mm( up);

  Serial.println(" -----------------------END DRAW O---------------------");


  for (int i = 2; i > 0; i-- ) {
    delay(1000);
    Serial.println(i);
  }

}

void draw_X(float x, float y) {


  Serial.println(" -----------------------DRAW X---------------------");

  float posx[4] = {x - a, x + a, x - a, x + a};
  float posy[4] = {y + a, y - a, y - a, y + a};

  float xa = x - a ;
  float ya = y  + a ;

  go_to(xa , ya);
  stepper1.setMaxSpeed(curSpeed );
  stepper2.setMaxSpeed(curSpeed );

  for (int i = 2; i > 0; i-- ) {
    delay(1000);
    Serial.println(i);
  }

  truc_z_mm( dw );
  line( posx[0],  posy[0], posx[1],  posy[1] );
  truc_z_mm( up);
  line( posx[1],  posy[1], posx[2],  posy[2] );
  truc_z_mm( dw);
  line( posx[2],  posy[2], posx[3],  posy[3] );
  truc_z_mm( up);

  for (int i = 2; i > 0; i-- ) {
    delay(1000);
    Serial.println(i);
  }

  Serial.println(" -----------------------END DRAW O---------------------");

}

void enableAll() {
  digitalWrite(EN1_PIN, LOW);
  Serial.write("Enable all");
  Serial.write("\n");
  en = true;
}

void disableAll() {
  digitalWrite(EN1_PIN, HIGH);
  Serial.write("Disable all");
  Serial.write("\n");
  en = false;
}
