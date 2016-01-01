#include "application.h"


#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))


#define TIME_ZONE -5
#define BOOT_STEP_DELAY 60


#include "InternetButton.h"
InternetButton b = InternetButton();


#include "clickButton.h"
ClickButton button1(D4, LOW, CLICKBTN_PULLUP);
ClickButton button2(D5, LOW, CLICKBTN_PULLUP);
ClickButton button3(D6, LOW, CLICKBTN_PULLUP);
ClickButton button4(D7, LOW, CLICKBTN_PULLUP);


#define RF_TX_PIN 2
static const bool rfDeviceCodePreamble[14] = {1,0,0,1,1,0,1,1,1,0,0,1,1,1};

static bool rfDeviceCodes[10][24] = {
	{0,1,1,1,1,1,1,1,0,1,0,1, 0,1,1,1,0,0,1,1,1,0,0,1},	// 1 on
	{1,1,1,1,1,0,1,1,1,0,0,1, 0,1,1,1,0,0,1,1,1,0,0,1},	// 1 off

	{1,1,1,0,1,1,1,1,1,1,0,0, 1,1,1,0,0,0,1,1,0,0,0,0},	// 2 on
	{0,1,1,0,1,0,1,1,0,0,0,0, 1,1,1,0,0,0,1,1,0,0,0,0},	// 2 off

	{1,1,0,1,1,1,1,1,1,0,1,1, 1,1,0,1,0,0,1,1,1,1,1,1},	// 3 on
	{0,1,0,1,1,0,1,1,1,1,1,1, 1,1,0,1,0,0,1,1,1,1,1,1},	// 3 off

	{0,1,0,0,1,1,1,1,0,0,1,0, 0,1,0,0,0,0,1,1,0,1,1,0},	// 4 on
	{1,1,0,0,1,0,1,1,0,1,1,0, 0,1,0,0,0,0,1,1,0,1,1,0},	// 4 off

	{0,0,1,0,1,1,1,1,0,0,0,0, 0,0,1,0,0,0,1,1,0,1,0,0},	// All on
	{1,0,1,0,1,0,1,1,0,1,0,0, 0,0,1,0,0,0,1,1,0,1,0,0}	// All off
};

// Button pixels
static uint8_t rfButtonPixels[4][3] = {
	{11,  0,  1},
	{ 2,  3,  4},
	{ 5,  6,  7},
	{ 8,  9, 10}
};

// Button colors
static uint8_t rfButtonColors[4][3] = {
	{255, 255,   0},
	{  0,   0, 255},
	{  0, 255,   0},
	{255,   0,   0}
};

// Button brightness
static uint8_t rfButtonBrightness[2] = {1, 4}; // 0 = on, 1 = off; Brightness is a divisor


// 434 MHz "states"
byte rfStates = 0;

// 434 MHz timings
uint16_t rfPulseShortuS = 571;
uint16_t rfPulseLonguS = 1331;
uint16_t rfDelayShortuS = 3154;
long rfDelayLonguS = 24140L;


// UDP port to listen on
static const uint16_t udpBroadcastPort = 9877;
UDP udp;


void setup() {
	// Make sure all LEDs are off first
	b.begin();
	b.allLedsOff();

	// Light the ring
	for(uint8_t i=0; i<12; i++) {
		// It's like a little progress loader!
		switch(i) {
			case 0:
				Serial.begin(9600);
				break;

			case 1:
				Time.zone(TIME_ZONE);
				break;

			case 2:
				Particle.function("function", fnRouter);
				break;

			case 3:
				pinMode(RF_TX_PIN, OUTPUT);
				break;

			case 4:
				udp.begin(udpBroadcastPort);
				break;

			case 5:
				pinMode(D4, INPUT_PULLUP);
				break;

			case 6:
				pinMode(D5, INPUT_PULLUP);
				break;

			case 7:
				pinMode(D6, INPUT_PULLUP);
				break;

			case 8:
				pinMode(D7, INPUT_PULLUP);
				break;

			default:
				; // Do nothing
		}

		// Helper variables
		volatile uint8_t thisPixel = rfButtonPixels[i/3][i%3];
		volatile uint8_t thisColor[3] = {
			rfButtonColors[i/3][0]/rfButtonBrightness[0],
			rfButtonColors[i/3][1]/rfButtonBrightness[0],
			rfButtonColors[i/3][2]/rfButtonBrightness[0]
		};

		// Start at pixel 11
		if(i==0)
			thisPixel = 11;
		else
			thisPixel = i-1;

		b.ledOn(thisPixel, thisColor[0], thisColor[1], thisColor[2]);
		delay(BOOT_STEP_DELAY);
	}


	// Hold it
	delay(BOOT_STEP_DELAY);

	// Dim it
	for(uint8_t i=0; i<12; i++) {
		// Helper variables
		volatile uint8_t thisPixel = rfButtonPixels[i/3][i%3];
		volatile uint8_t thisColor[3] = {
			rfButtonColors[i/3][0]/rfButtonBrightness[1],
			rfButtonColors[i/3][1]/rfButtonBrightness[1],
			rfButtonColors[i/3][2]/rfButtonBrightness[1]
		};

		// Start at pixel 11
		if(i==0)
			thisPixel = 11;
		else
			thisPixel = i-1;

		b.ledOn(thisPixel, thisColor[0], thisColor[1], thisColor[2]);
		delay(BOOT_STEP_DELAY);
	}
}


void loop() {
	checkUDP();

	button1.Update();
	button2.Update();
	button3.Update();
	button4.Update();

	if (button1.clicks != 0)
		doRfButtonToggle(0);

	else if(button2.clicks != 0)
		doRfButtonToggle(1);

	else if(button3.clicks != 0)
		doRfButtonToggle(2);

	else if(button4.clicks != 0)
		doRfButtonToggle(3);
}


int fnRouter(String command) {
    command.trim();
    command.toUpperCase();

    if(command.substring(0, 7)=="BUTTON,") {
        // 1st set = button number
		uint8_t buttonNumber = command.substring(7).toInt();

        // 2nd set = button state
        uint8_t buttonState = command.substring(9).toInt();

		if(buttonState==1)
			doRfButtonToggle(buttonNumber, 0);
		else
			doRfButtonToggle(buttonNumber, 1);

		return buttonNumber + buttonState;
    }

    return -1;
}


void doRfButtonToggle(uint8_t i) {
	bool rfState = bitRead(rfStates, i);

	doRfButtonToggle(i, rfState);
}


void doRfButtonToggle(uint8_t i, bool rfState) {
	// If it's off, turn it on
	// If it's on, turn it off
	if(rfState==0)
		bitSet(rfStates, i);
	else
		bitClear(rfStates, i);

	Particle.publish("home-button", "Button: "+String(i+1)+", On/Off: "+String(rfState)+", States: "+String(rfStates));

	doRfBroadcast(i, rfState);
	doRfButtonColor(i, rfButtonBrightness[rfState]);
}


void checkUDP() {
	if (udp.parsePacket() > 0) {
		b.ledOn(6, 127, 0, 0);

		char packetBuffer[64] = "";
	    udp.read(packetBuffer, 64);

	    parsePacket(String(packetBuffer));

		delay(100);
	    b.allLedsOff();
	}
}


void doRfButtonColor(uint8_t button, uint8_t brightness) {
	for(uint8_t i=0; i<3; i++) {
		volatile uint8_t thisPixel = rfButtonPixels[button][i];
		volatile uint8_t thisColor[3] = {
			rfButtonColors[button][0]/brightness,
			rfButtonColors[button][1]/brightness,
			rfButtonColors[button][2]/brightness
		};

		b.ledOn(thisPixel, thisColor[0], thisColor[1], thisColor[2]);
	}
}


void parsePacket(String packet) {
	uint32_t checksum = 0;
	packet.trim();

	Serial.print(timestamp());
	Serial.print("<<< ");
	Serial.println(packet);

	Particle.publish("debug", packet, 60, PRIVATE);

	// Calculate checksum up to ";"
	for(uint8_t i=0; i<packet.lastIndexOf(";"); i++) {
		checksum += ord(packet.charAt(i));
	}

	// Get the checksum sent with the packet
	String str_checksum = packet.substring(packet.lastIndexOf(";")+1);

	if(String(checksum, HEX)==str_checksum)
		sendPacket(packet.substring(0, packet.lastIndexOf(";")));
	else {
		String pub = packet+" :: checksum "+str_checksum+" != "+String(checksum, HEX);
		Serial.print(timestamp());
		Serial.print("!!! checksum does not match: ");
		Serial.print(str_checksum);
		Serial.print(" != ");
		Serial.println(String(checksum, HEX));

		Particle.publish("error", pub, 60, PRIVATE);
	}
}


void sendPacket(String packet) {
	Serial.print(timestamp());
	Serial.print(">>> ");
	Serial.println(packet);
	Particle.publish("statsd", packet);
}


uint8_t ord(char b) {
	uint8_t a;
	a = b-'0';
	return a;
}


String timestamp() {
	return "["+Time.format(Time.now(), "%F %T")+"] ";
}


void doRfPreamble() {
  for(uint8_t i=0; i<14; i++) {
    if(rfDeviceCodePreamble[i]==1)
      doRfPulseLong();
    else
      doRfPulseShort();

    doRfDelayShort();
  }
}


void doRfBroadcast(uint8_t device, uint8_t state) {
	bool * Device = rfDeviceCodes[device+state];

  for(uint8_t i=0; i<3; i++) {
    doRfPreamble();

    for(uint8_t j=0; j<12; j++) {
      if(Device[j]==1)
        doRfPulseLong();
      else
        doRfPulseShort();

      doRfDelayShort();
    }

    doRfDelayLong();
  }

  for(uint8_t i=0; i<3; i++) {
    doRfPreamble();

    for(uint8_t j=12; j<24; j++) {
      if(Device[j]==1)
        doRfPulseLong();
      else
        doRfPulseShort();

      doRfDelayShort();
    }

    doRfDelayLong();
  }
}


void doRfPulseLong() {
  digitalWrite(2, HIGH);

  delayMicroseconds(rfPulseLonguS);

  digitalWrite(2, LOW);
}


void doRfPulseShort() {
  digitalWrite(2, HIGH);

  delayMicroseconds(rfPulseShortuS);

  digitalWrite(2, LOW);
}


void doRfDelayLong() {
  digitalWrite(2, LOW);

	// delayMicroseconds doesn't like really big numbers
	// so this is a workaround to make the numbers passed
	// to delayMicroseconds smaller
  for(uint8_t i=0; i<rfDelayLonguS/10000; i++) {
    delayMicroseconds(10000);
  }

  delayMicroseconds(rfDelayLonguS%10000);
}


void doRfDelayShort() {
  digitalWrite(2, LOW);

  delayMicroseconds(rfDelayShortuS);
}
