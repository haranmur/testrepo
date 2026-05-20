const int relayPin = 4; // Connect your relay signal pin to D4
const int pulseRelayPin = 2; // Connect your pulse relay signal pin to D2

void setup() {
  pinMode(relayPin, OUTPUT);
  pinMode(pulseRelayPin, OUTPUT);
  
  digitalWrite(relayPin, LOW); // Start with relay ON (common for active-low)
  digitalWrite(pulseRelayPin, HIGH); // Start with pulse relay OFF (assuming active-low)
  
  Serial.begin(9600); // Standard baud rate for Serial Monitor
  Serial.println("Relay Control Ready.");
  Serial.println("Type '1' for ON, '0' for OFF (Pin 4)");
  Serial.println("Type 'p' or 'P' for momentary pulse (Pin 2)");
}

void loop() {
  if (Serial.available() > 0) {
    char userInput = Serial.read(); // Read the first character available
    
    if (userInput == '1') {
      digitalWrite(relayPin, LOW); // Turn relay ON
      Serial.println("Status: Relay Pin 4 is ON (flash mode)");
    } 
    else if (userInput == '0') {
      digitalWrite(relayPin, HIGH); // Turn relay OFF
      Serial.println("Status: Relay Pin 4 is OFF (boot mode)");
    }
    else if (userInput == 'p' || userInput == 'P') {
      Serial.println("Pulsing relay on Pin 2...");
      digitalWrite(pulseRelayPin, LOW); // Turn pulse relay ON
      delay(1500); // Keep it on for 1.5 seconds
      digitalWrite(pulseRelayPin, HIGH); // Turn pulse relay OFF
      Serial.println("Status: Pulse relay completed");
    }
  }
}