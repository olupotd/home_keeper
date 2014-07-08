#include <Arduino.h>
#include<SoftwareSerial.h>
#include<string.h>
#include <LiquidCrystal.h>

//NSSF REF NO 16741

/*
*	Contributors: Olupot Douglas, Habugisha David
*
*/


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);


int motor_forward = 10;
int motor_reverse = 9;
//Create a software serial object
SoftwareSerial serial(7,8);

string msg("");
char data;

//Variables for holding data read from Pins.
int temp_output, light_output;
int fixed = 0;
int led = 13; //alarm light 
int temp_sensor = A0;
int light_senor = A1;
int light = 12;
int fan = 2;
int sent_summary = 0;
int alarm = 13;

//Setup all the required Pins once.
void setup(){
	serial.begin(19200);
	serial.println("AT+CPIN=4510"); //Sets the PinCode for Sim
	delay(5000); //Delay for 5 seconds
	pinMode(Temp_sensor,INPUT); 
	pinMode(light_Sensor,INPUT); 
	pinMode(led, OUTPUT);
	pinMode(light, OUTPUT);
	//Attempt to delete all available messages from Sim
	serial.println("AT+CMGDA=\"DEL ALL\"");
	delay(1000);
	pinMode(photPin, OUTPUT);
	lcd.begin(16, 4);
	lcd.println("Welcome, System initializing..");
	//Setup the motors
	setupMotor();
}


void loop(){
	//Light must be read firs
	read_light();
	//Read the Room temp values
	read_temp();
	//Read smoke detector values
	read_smoke();
	//Check if the GSM is still available
	if(serial.available()){
		//Means data is coming from GSM
		while(serial.available()){
			data = serial.read();
			msg+= data;
		}
		//For testing purposes only. 
		Serial.println(msg);	
		
		if(msg.indexOf("+CMTI") > 0){
			//Means the SMS was received.
			Serial.println("Now processing GSM");
			process_gsm();	
		}
	}
	if(sent_summary == 0){
		//Means the system never sent a summary message  so send one.
		send_sms('S');
	}
}


//function to initialize the motor settings.
void setupMotor(){
	pinMode(motor_forward, OUTPUT);
	pinMode(motor_reverse, OUTPUT);
}

void forward_rotation(int motor_f, int motor_r){
	digitalWrite(motor_f, 1);
	digitalWrite(motor_r, 0);
}

void reverse_rotation(int motor_f, int motor_r){
	digitalWrite(motor_f, 0);
	digitalWrite(motor_r, 1);
}

void stop_motor(){
	digitalWrite(motor_forward, 0);
	digitalWrite(motor_reverse, 0);
}
int readTemp(){
	//Read from the temperature sensor
	temp_output = analogRead(temp_sensor); 
	//Take a delay of 1 second
 	delay(1000); 
 	//Perform some Mathematical calcs to Celcius
 	int temp_value = (5.0 * temp_output * 100) / 1024; 
 	//Again, only for purposes of testing
 	Serial.println(temp_value);
	if(temp_value > 45 && temp_value < 60)
	{ 
		//Perform necessary options 
		//Like turn on the Cooling fans
		forward_rotation(motor_forward, motor_reverse);
	} 
	else if(temp_value < 45) {
		//It's getting cold in here
		stop_motor();
	}
	else if(temp_value > 60) {
		//It's now really serious
		forward_rotation();
		sound_alarm();
		send_sms('A');	
	}
	//return the temp value for other purposes.
	return temp_value;
}

//Needs to be wrapped in an interrupt function.
void process_gprs(){
	int pos = msg.indexOf(",");
	//Message starts right after the , sign
	string index = msg.substring(pos + 1);
	Serial.println(index);
	//Acquire the GSM INBOX
	serial.println("AT+CMGR = ");
	serial.println(index);
	
	//Read the incoming sms
	delay(1000);
	if(msg.indexOf("+CMGR") > 0){
		Serial.println("Now inside the CMGR");
		process_gsm();
	}
}


void sound_alarm(){
	digitalWrite(alarm, 1);
}
//Perform decisions on the GSM Inquiries. 
void process_gsm(){
	//Check for the inquire type
	if(msg.indexOf("What is the Temp") > 0){
		Serial.println("Temp Values are");
		Serial.println(analogRead(A1));
		send_sms('T');
	}
	if(msg.indexOf("Turn light off") > 0){
		Serial.println("Turning Off lights");
		Serial.println(anaologWrite(A0, LOW));
		send_sms('O');
	}
}

//Function to send an SMS 
void send_sms(char c){
//Switch depending on the values passed
switch c{
	case 'T':
		//Means Send the temp values to user
		serial.println("AT+CMGS=\"+256774435155\"");
		delay(100);
		serial.println(readTemp());
		delay(1000);
		//Now tell the GSM that that's all for the msg sent
		serial.println((char)26); //Crtl+Z
		delay(100);
		serial.println();
		clear_msg();
		break;
	case 'O':
		//Means user requrested to turn off the lights
		//You need to create a separate function to handle 
		digitalWrite(light, LOW);
		//Inform the user about the operation
		serial.println("AT+CMGS=\"+256774435155\"");
		delay(100);
		serial.println("Lights turned Off");
		delay(1000);
		//Now tell the GSM that that's all for the msg sent
		serial.println((char)26); //Crtl+Z
		delay(100);
		serial.println();
		break;
	case 'A':
		//Only when an alarm has been triggered.
		serial.println("AT+CMGS=\"+256774435155\"");
		delay(100);
		serial.println("Temperature Values very high");
		delay(1000);
		serial.println("Looks like your house is on Fire");
		delay(1000);
		//Now tell the GSM that that's all for the msg sent
		serial.println((char)26); //Crtl+Z
		delay(100);
		serial.println();
		break;
	case 'W':
		//Only when an alarm has been triggered.
		writeToLCD("Sending Message to User");
		serial.println("AT+CMGS=\"+256774435155\"");
		delay(100);
		serial.println("Temp values rising but no smoke yet");
		delay(1000);
		//Now tell the GSM that that's all for the msg sent
		serial.println((char)26); //Crtl+Z
		delay(100);
		serial.println();
		break;
	case 'S':
		//Only when an alarm has been triggered.
		serial.println("AT+CMGS=\"+256774435155\"");
		delay(100);
		serial.println("Good morning These are the readings.");
		serial.print("Temp:");
		serial.println(readTemp());
		serial.print("Smoke Levels:");
		serial.println(smoker());
		serial.print("Light Intensity:");
		serial.println(read_light());
		delay(1000);
		//Now tell the GSM that that's all for the msg sent
		serial.println((char)26); //Crtl+Z
		delay(100);
		serial.println();
		sent_summary = 1;
		break;
		
	default:
		//No valid option was passed as an argument.
		Serial.println("Invalid option Selected.");
		break;
}
//Flush out all the data in the GSM Buffer.
serial.flush();
}


//function to clear the GSM message received.
void clear_msg(){
	msg = "";
}

//Not needed by the user as is not well understood, so void
int read_light(){
	//function to handle light reading.
	light_output = analogRead(light_sensor);
	//Try to map the value read
	if(map(light_output, 0, 1024, 0, 500) < 200){
		//Assuming it gets dark at a vlaue of 200
		//turn On the lights then
		digitalWrite(light, HIGH);
		writeToLCD("Turning On lights");
		//Open the curtains
		forward_rotate(motor_forward, motor_reverse);
		delay(5000);
		stop_motor();
	}
	else{
		//Light value is too high so turn off the lights
		digitalWrite(light, LOW);
		writeToLCD("Turning Off lights");
		//Close the curtains
		reverse_rotate(motor_forward, motor_reverse);
		delay(5000);
		stop_motor();
	}
	return light_output;
}

//fixed can only be fixed via SMS or Manually by user
void sound_alarm(){
	//Only called to sound the alarm system
	//Also check to see if the Smoke detector is reading something
	if(smoker() > 50){
	//Looks like things are on fire
		//Sounds the Alarm indefinately
		while(fixed = 0){
			digitalWrite(alarm, HIGH);
			delay(1000);
			digitalWrite(alarm, LOW);
			delay(1000);
			writeToLCD("Fire Outbreak");
			//Open the curtains
			forward_rotate(motor_forward, motor_reverse);
		}
	}else{
	//Probably just some kitchen smoke or the kids
	//Alert the user anyway.
	send_sms('W');
	forward_rotate(motor_forward, motor_reverse);
	
	}
}

void writeToLCD(char* msg){
	lcd.print(msg);
	delay(2000); //delay for 2 seconds
	lcd.setCursor(0, 1);
	//There might be a need to clear the LCD
	
}

