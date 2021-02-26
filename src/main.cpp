#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseESP32.h>
#include <ESP32Tone.h>
#include <LineMessage.h>
#include <pt.h>
#include <MQ7.h>

#define Pin_Siren 25
#define MonoxideTriggerValue 25

static struct pt pt_main, pt_siren;

static WiFiClientSecure client;
static FirebaseData fbdo;
static LineMessage line("notify-api.line.me", "mRkNwPQxLDaP4sQQJEehvwAhRehS8qcKOfzfLMC9pWn");
static MQ7 monoxide_sensor(GPIO_NUM_35, 5.0);

void setup() 
{
	Serial.begin(9600);

	WiFi.begin("Syaro", "istheorderarabbit"); 																	//	Connect WiFi
	while (WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(500); } 									//	Waiting Connect
	Serial.println("WiFi connected");	Serial.println("IP address: ");	Serial.println(WiFi.localIP());			//	Display local IP address

	Firebase.begin("https://iot-monoxide-siren-default-rtdb.firebaseio.com/", "U72zNjh1mW4LhRj1Na7NKmlpg3B1cQlwko9yvxYz");	//Connect Firebase Realtime Database

	PT_INIT(&pt_main);		//Protothreads Main
    PT_INIT(&pt_siren);		//Protothreads Siren
}


static int System_Main(struct pt *pt)
{
	PT_BEGIN(pt);
	static decltype(millis()) lastTime = 0;
	while (true)
	{
		monoxide_sensor.getPPM();
		Serial.print("Now ppm"); Serial.println(monoxide_sensor.value);
		if (monoxide_sensor.value >= MonoxideTriggerValue)
		{
			line.setMessage("一氧化碳濃度超標目前濃度為：" + String(monoxide_sensor.value) + "\n" +
							"");
			line.sendMessage();
			
		}
		lastTime = millis();
		PT_WAIT_UNTIL(pt, millis() >= lastTime + 1000);
	}
	PT_END(pt);
}


static int WarningSiren(struct pt *pt)
{
	PT_BEGIN(pt);
	while (true)
	{
		PT_WAIT_UNTIL(pt, monoxide_sensor.value >= MonoxideTriggerValue);
		while (monoxide_sensor.value >= MonoxideTriggerValue)
		{
			tone(Pin_Siren, 853, 10);
			tone(Pin_Siren, 960, 10);
			PT_YIELD(pt);
		}
		noTone(Pin_Siren);
	}
	PT_END(pt); 
}




void loop() 
{
	System_Main(&pt_main);
	WarningSiren(&pt_siren);



}

// https://github.com/swatish17/MQ7-Library.git


