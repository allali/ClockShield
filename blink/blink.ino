#define LED_BLUE 2
#define LED_GREEN 3
#define LED_RED1 4
#define LED_RED2 5


void setup() {
  pinMode(LED_BLUE,OUTPUT);
  pinMode(LED_GREEN,OUTPUT);
  pinMode(LED_RED1,OUTPUT);
  pinMode(LED_RED2,OUTPUT);
}

void loop(){
  digitalWrite(LED_BLUE,HIGH);
  delay(500);
  digitalWrite(LED_GREEN,HIGH);
  delay(500);
  digitalWrite(LED_RED1,HIGH);
  delay(500);
  digitalWrite(LED_RED2,HIGH);
  delay(500);

  digitalWrite(LED_BLUE,LOW);
  delay(500);
  digitalWrite(LED_GREEN,LOW);
  delay(500);
  digitalWrite(LED_RED1,LOW);
  delay(500);
  digitalWrite(LED_RED2,LOW);
  delay(500);
}