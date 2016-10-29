#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6
#define LED	13
int DEBUG = 0;
int LIGHT_COUNT = 60;

int EEPROM_addr = 0;


int customColorNumber = 0;
int colorArray[ ][3] = {    {0, 0, 0}
                          , {0, 0, 0}
                          , {0, 0, 0}
                          , {0, 0, 0}
                          , {0, 0, 0}
                          , {0, 0, 0} };

int customColorCount = 6;                    

//program tracking};
int activeProgram=0;
int previousProgram=0;

//flags for message resevieng
boolean partStartReceived=false;
boolean fullStartReceived=false;
boolean customProgramReceived=false;

char chunk = '-';

//variable for holding when the last step time was for dynamic programs
long lastStep = 0;
int state = 0;
boolean direction = 0;

#define PROGRAM_COUNT (17) //array starts at zero so this should be one plus the highest numbered pname below

char pname0[ ] = "Off";
char pname1[ ] = "All Green";
char pname2[ ] = "All Red";
char pname3[ ] = "Green and Red";
char pname4[ ] = "White and Red";
char pname5[ ] = "White and Violet";
char pname6[ ] = "Sequenced Colors";
char pname7[ ] = "Yellow and Violet";
char pname8[ ] = "Red, White and Blue";
char pname9[ ] = "Green & Red Blinking";
char pname10[ ] = "Orange and Green";
char pname11[ ] = "Orange and White";
char pname12[ ] = "All Orange";
char pname13[ ] = "Red Cylon in Green";
char pname14[ ] = "Red Orange Yellow";
char pname15[ ] = "Orange Blue";
char pname16[ ] = "Custom";

char* prognames[] = {pname0, pname1, pname2, pname3, pname4, pname5, pname6, pname7, pname8, pname9, pname10, pname11, pname12, pname13, pname14, pname15, pname16};

char pcodes[] = "-abcdefghijklmnox";

#define COMMAND_COUNT (1) //array starts at zero so this should be one minus total pnames below
char cname0[ ] = "Send JSON Programs";

char* commnames[] = {cname0};

char ccodes[] = "0";

// print help
// send JSON
// receive custom config



// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LIGHT_COUNT, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  // activeProgram=EEPROM.read(EEPROM_addr); //read last program from EEPROM when power started

  pinMode(LED,OUTPUT);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
    
  // run test sequence
  colorWipe(strip.Color(255, 0, 0), 5); // Red
  colorWipe(strip.Color(0, 255, 0), 5); // Green
  colorWipe(strip.Color(0, 0, 255), 5); // Blue
  colorWipe(strip.Color(0, 0, 0), 0); // Black
  
  //pull last from eeprom

  activeProgram = EEPROM.read(EEPROM_addr); // set the new active program to the index of the pcode matching the command
  updateProgram(activeProgram);


  //start up serial
  Serial.begin(9600);
  Serial.setTimeout(5000);
  // Serial.print("{");
  // sendCommandResponseJSON();
  // Serial.println("}");
}

void loop() {
//listen on serial
	while (Serial.available() > 0) {
//		if(DEBUG) Serial.println("serial available");
		chunk = Serial.read();
//		Serial.write(chunk); // echo the chunk messes everything up if really parsing JSON
		if(chunk == '|' && !partStartReceived) { 
		  // this will run twice and catch the first two pipes.
		  partStartReceived = true;
//			if(DEBUG) {
//				Serial.print("Start Message");
//				delay(500);
//				Serial.print("partStartReceived = ");
//				Serial.print(partStartReceived);
//				Serial.print(", char was: ");
//				Serial.println(chunk);
//			}
		} else if (chunk == '|' && partStartReceived  && !fullStartReceived) { 
		  // once inital pipes caught, this might catch the third pipe and
		  fullStartReceived = true;
//			  if(DEBUG) {
//				Serial.print("Confirm Message");
//				delay(500);
//				Serial.print("fullStartReceived = true, char was: ");
//				Serial.println(chunk);
//			  }
    } else if (fullStartReceived && chunk == '&') {
      // we've got a custom character so lets start reading serial until 
      customProgramReceived = true;
      if(DEBUG) Serial.println("ok custom coming");

    } else if (customProgramReceived && customColorNumber == 0 ) { 
        // ok, we need to set a custom number
        if(DEBUG) Serial.println("read next chunk: then chunkInt");
        //chunk = Serial.read();
        int chunkInt = chunk - 48; // convert from ascii to int
        if(DEBUG) Serial.println(chunkInt);

        if(chunkInt >= 0 && chunkInt < 6) { // 48= 0 and 55 = 7 in
          customColorNumber = chunkInt;
          if(DEBUG) Serial.print("custom number: ");
          if(DEBUG) Serial.println (customColorNumber);

          if(DEBUG) Serial.println("ok ready for R,G,B");

          int r = Serial.parseInt();
          int g = Serial.parseInt();
          int b = Serial.parseInt();
          

          colorArray[customColorNumber][0] = constrain(r, 0, 255);
          colorArray[customColorNumber][1] = constrain(g, 0, 255);
          colorArray[customColorNumber][2] = constrain(b, 0, 255);
          

          sendCustomColorsJSON(customColorNumber);
          Serial.println();

          resetInputCommand();
 



        } else {         //end chunkInt if

          // if we didn't get a number, reset us back to inital entry
          if(DEBUG) Serial.println("command fail, not 0-5 number");
          resetInputCommand();
        }

		} else if (fullStartReceived && chunk != '|') { 
		  // we got thru the start, so now our chunk could be a valid program
//		  if(DEBUG) {
//			Serial.print("chunk: ");
//			Serial.print(chunk);
//			delay(500);      
//		  }
	     
	     for(int i=0; i<COMMAND_COUNT; i++) {
	     	if(ccodes[i] == chunk) {

     		  executeCommand(i);
          
	     	}
	     
	     }
	   

		 for(int i=0; i<PROGRAM_COUNT; i++) {
			if(pcodes[i] == chunk) {
			  // ok we found a matching pcode to change the program
//			  if(DEBUG) {
//				  Serial.print("Remote Command: ");
//				  Serial.print(i);
//				  delay(1000);
//			  }
        previousProgram=activeProgram;
			  activeProgram = i; // set the new active program to the index of the pcode matching the command
        updateProgram(activeProgram);
        Serial.print("{");
        sendCommandResponseJSON();
        Serial.println("}");

			  break;
			}
		  }
      resetInputCommand();
		} // end elseif
	  } //end while of listen on serial section

    stepProgram(activeProgram);

// get time every 5 minutes, decide if sun up or down
 // if sun is up, go to sleep for 5 minutes
 // if sun is down, is it before midnight?
 	// if before midnight then run program
 	// if later than midnight
//start previous program
//if serial read:
 // get letter, find program
 // if found, 
 	//run program,
 	// write program to previous program in eeprom, 
 	// send program name back up serial
 // if not found, don't change program



} // end of loop()

void sendCustomColorsJSON(int theLED) {

    Serial.print("{\"ledNumber\":");
    Serial.print(theLED);
    Serial.print(", \"r\":");
    Serial.print(colorArray[theLED][0]);
    Serial.print(", \"g\":");
    Serial.print(colorArray[theLED][1]);
    Serial.print(", \"b\":");
    Serial.print(colorArray[theLED][2]);
    Serial.print("}");

}

void resetInputCommand() {
      fullStartReceived = false;
      partStartReceived = false;
      customProgramReceived = false;
      customColorNumber = 0;

}



// to update the program and send serial confirmation of the program name
void updateProgram(int theProgram) {
    
    switch (theProgram) {
      case 0: colorWipe(strip.Color(0, 0, 0), 20); break; // all off
      case 1: colorWipe(strip.Color(0, 255, 0), 20); break; // all green
      case 2: colorWipe(strip.Color(255, 0, 0), 20); break; // all red
      case 3: altTwoColor(strip.Color(0, 255, 0), strip.Color(255, 0, 0), 20);  break; // alt green/red
      case 4: altTwoColor(strip.Color(255, 255, 255), strip.Color(255, 0, 0), 20); break; //alt white/red
      case 5: altTwoColor(strip.Color(255, 255, 255), strip.Color(102, 0, 204), 20); break; //alt white_violet
      case 6: altSixColor(strip.Color(0, 255, 0), strip.Color(0, 0, 255), strip.Color(102, 0, 204) , strip.Color(255, 255, 0), strip.Color(255, 153, 0), strip.Color(255, 0, 0), 20); break;// color
      case 7: altTwoColor(strip.Color(255, 255, 0), strip.Color(102, 0, 204), 20); break; //alt yellow_violet
      case 8: altThreeColor(strip.Color(255, 0, 0), strip.Color(255, 255, 255), strip.Color(0, 0, 255), 20); break;//three color
     // case 9: alt_green_red_alternating(lights, false); break;
      case 10: altTwoColor(strip.Color(255, 153, 0), strip.Color(0, 255, 255), 20); break; //alt orange_green
      case 11: altTwoColor(strip.Color(255, 153, 0), strip.Color(255, 255, 255), 20); break; //alt orange_white
      case 12: colorWipe(strip.Color(255, 153, 0), 20); break; // all orange
      case 13: two_color_cylon(strip.Color(0, 255, 0), strip.Color(255, 0, 0), false); break; 
      case 14: altThreeColor(strip.Color(255, 0, 0), strip.Color(255, 153, 0), strip.Color(255, 255, 0), 20); break;// orange red yellow three color
      case 15: altTwoColor(strip.Color(255, 153, 0), strip.Color(0, 0, 255), 20); break; //alt orange_blue
      case 16: loadCustomProgram(); break;
      
      default: colorWipe(strip.Color(0, 0, 0), 20); // set to off
    }
    
    Serial.println();
    EEPROM.write(EEPROM_addr,theProgram);



	delay(200);
  	if(DEBUG) blinkLED(theProgram, 200);
}

void stepProgram(int theProgram) {
      switch (theProgram) {
      // case 9: alt_green_red_alternating(lights, true); break;
      case 13: two_color_cylon(strip.Color(0, 255, 0), strip.Color(255, 0, 0), true); break;
      default: ;

    }

}  

void sendCommandResponseJSON() {

    Serial.print("\"commandResponse\": \"1\", \"currentProgram\": \"");
    Serial.print(prognames[activeProgram]);
    Serial.print("\" ");

}

void executeCommand(int theCommand) {
    
    switch (theCommand) {
      case 0: sendProgramsAsJSON(); break; // all off
      
      default: Serial.println('No Command'); // set to off
    }
}

// blink for debugging purposes
void blinkLED(int count, uint8_t wait) {
	for(uint8_t i=0; i<count; i++) {
	  digitalWrite(LED, HIGH);
	  delay(wait);
	  digitalWrite(LED, LOW);
	  delay(wait);
  }
}


// set each pixel to alternating colors supplied as parameters
void altTwoColor(uint32_t c1, uint32_t c2, uint8_t wait) {
	for(uint16_t i=0; i<strip.numPixels(); i++) {
		if(i%2==0) {
			strip.setPixelColor(i,c1);
		} else {
			strip.setPixelColor(i,c2);
		}
		strip.show();
		delay(wait);
	}

}


// set each pixel to alternating colors supplied as parameters
void altThreeColor(uint32_t c1, uint32_t c2, uint32_t c3, uint8_t wait) {
	for(uint16_t i=0; i<strip.numPixels(); i++) {
		switch (i%3) {
			case 0: strip.setPixelColor(i,c1); break;
			case 1: strip.setPixelColor(i,c2); break;
			case 2: strip.setPixelColor(i,c3); break;
		}
		strip.show();
		delay(wait);
	
	}
}

// set each pixel to alternating colors supplied as parameters
void altFourColor(uint32_t c1, uint32_t c2, uint32_t c3, uint32_t c4, uint8_t wait) {
	for(uint16_t i=0; i<strip.numPixels(); i++) {
		switch (i%4) {
			case 0: strip.setPixelColor(i,c1); break;
			case 1: strip.setPixelColor(i,c2); break;
			case 2: strip.setPixelColor(i,c3); break;
			case 3: strip.setPixelColor(i,c4); break;
		}
		strip.show();
		delay(wait);
	}
}

// set each pixel to alternating colors supplied as parameters
void altFiveColor(uint32_t c1, uint32_t c2, uint32_t c3, uint32_t c4, uint32_t c5, uint8_t wait) {
	for(uint16_t i=0; i<strip.numPixels(); i++) {
		switch (i%5) {
			case 0: strip.setPixelColor(i,c1); break;
			case 1: strip.setPixelColor(i,c2); break;
			case 2: strip.setPixelColor(i,c3); break;
			case 3: strip.setPixelColor(i,c4); break;
			case 4: strip.setPixelColor(i,c5); break;
		}
		strip.show();
		delay(wait);
	}
}

// set each pixel to alternating colors supplied as parameters
void altSixColor(uint32_t c1, uint32_t c2, uint32_t c3, uint32_t c4, uint32_t c5, uint32_t c6, uint8_t wait) {
	for(uint16_t i=0; i<strip.numPixels(); i++) {
		switch (i%6) {
			case 0: strip.setPixelColor(i,c1); break;
			case 1: strip.setPixelColor(i,c2); break;
			case 2: strip.setPixelColor(i,c3); break;
			case 3: strip.setPixelColor(i,c4); break;
			case 4: strip.setPixelColor(i,c5); break;
			case 5: strip.setPixelColor(i,c6); break;
		}
		strip.show();
		delay(wait);
	}
}


//
//
// below are strand test programs to implement later
//
//
//



// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

void two_color_cylon(uint32_t c1, uint32_t c2, boolean runner) {
  long stepDelay = 250;// number of millisec between state switches.
  int maxState = LIGHT_COUNT; // number of states.

  if (!runner) {// if we come here runner=false, then we are starting fresh;
    state=0;
  }

  if ((millis() - lastStep) > stepDelay) { // if longer than the last time we were here
    if (direction) {
      state--; //decrement the state
    } else {
      state++; //increment the state.
    }

    lastStep = millis(); // set the current time as the lastStep
    
    if(state == maxState) { // reverse direction if gets to the ends.
      direction=1;
      state--;
    } else if(state == -1) {
      direction=0;
      state++;
    }
  }

  // set all c1 color
  for (int i=0; i < LIGHT_COUNT; i=i+1) {
    if(i != state ){
        strip.setPixelColor(i,c1);
    }
  } 
  //set one c2 color
  strip.setPixelColor(state,c2);
  strip.show();
  delay(stepDelay);


}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void sendProgramsAsJSON() {
	Serial.println();
	Serial.println("{\"programs\": [");
	for(uint8_t i = 0; i < PROGRAM_COUNT; i++) { //iterate through programs
		Serial.print("\t");
		Serial.print("{\"programName\" : \"");
		Serial.print(prognames[i]);
		Serial.print("\", \"programCode\" : \"");
		Serial.print(pcodes[i]);
		Serial.print("\"}");
		if(i < PROGRAM_COUNT-1) { //only insert comma if second to last
			Serial.print(",");
		}
		Serial.println();
	}
	Serial.println("],");

  Serial.println("\"customLED\": [");
  for(uint8_t i = 0; i < customColorCount; i++) { //iterate through colors
    Serial.print("\t");
    sendCustomColorsJSON(i);
    if(i < customColorCount-1) { //only insert comma if second to last
      Serial.print(",");
    }
    Serial.println();
  }
  Serial.println("],");


	Serial.print("\"commandPrefix\" : \"||\", ");
  sendCommandResponseJSON();
  Serial.println("}");
}

void sendFreeTextIntro() {
Serial.println("------------------------------------------------------");
  Serial.println("       >>> Little Free Library Light Runner <<<       ");
  Serial.println("Ready for Commands as below. || precedes program code.");
  Serial.println("------------------------------------------------------");
  Serial.print("Current program is: ");
  Serial.println(prognames[activeProgram]);
  Serial.println();

  Serial.println("----------------------Programs------------------------");
  for(uint8_t i=0; i < PROGRAM_COUNT; i++) {
  Serial.print("Program Code: '"); 
  Serial.print(pcodes[i]);
  Serial.print("' is program ");
  Serial.println(prognames[i]);
  }
  Serial.println("----------------------Commands------------------------");
  
  for(uint8_t i=0; i < COMMAND_COUNT; i++) {
  Serial.print("Command Code: '"); 
  Serial.print(ccodes[i]);
  Serial.print("' is command ");
  Serial.println(commnames[i]);
  }
  Serial.println("------------------------------------------------------");

  Serial.print("Ready for Command: ");


}

void loadCustomProgram() {

  altSixColor(strip.Color(colorArray[0][0], colorArray[0][1], colorArray[0][2])
            , strip.Color(colorArray[1][0], colorArray[1][1], colorArray[1][2])
            , strip.Color(colorArray[2][0], colorArray[2][1], colorArray[2][2])
            , strip.Color(colorArray[3][0], colorArray[3][1], colorArray[3][2])
            , strip.Color(colorArray[4][0], colorArray[4][1], colorArray[4][2])
            , strip.Color(colorArray[5][0], colorArray[5][1], colorArray[5][2])
            , 20); 

      
}