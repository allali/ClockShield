#define TEMP A0
#define LIGHT A1


void setup() {
  Serial.begin(9600);
  pinMode(TEMP, INPUT);
  pinMode(LIGHT, INPUT);
}

float temperature = 0;
int lightValue = 0;

void loop() {
  lightValue = analogRead(LIGHT);


  float resistance;
  int a;
  a = analogRead(TEMP);
  resistance = (float)(1023 - a) * 10000 / a;  //Calculate the resistance of the thermistor
  int B = 3975;
  /*Calculate the temperature according to the following formula.*/
  temperature = 1 / (log(resistance / 10000) / B + 1 / 298.15) - 273.15;

  Serial.print("temp:");
  Serial.print(temperature);

  Serial.print(" | light :");
  Serial.println(lightValue);

  delay(1000);
}