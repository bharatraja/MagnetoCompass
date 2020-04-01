// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 
#include "AZ3166WiFi.h"
#include "LIS2MDLSensor.h"
#include "OledDisplay.h"

#define APP_VERSION     "ver=1.0"
#define LOOP_DELAY      1000
#define EXPECTED_COUNT  5

//#define M_PI            3.14  //M_PI is already defined in system headers
#define  NORTH          "N"
#define  SOUTH          "S"
#define  EAST           "E"
#define  WEST           "W"
#define  COMPASS_SENSITIVITY 5  //Adjustable so we get stable reading around N/S/E/W

//const


// The magnetometer sensor
static DevI2C *i2c;
static LIS2MDLSensor *lis2mdl;

// Data from magnetometer sensor
static int axes[3];
static int base_x;
static int base_y;
static int base_z;

// Indicate whether the magnetometer sensor has been initialized
static bool initialized = false;

// The open / close status of the door
static bool preOpened = false;

// Indicate whether DoorMonitorSucceed event has been logged
static bool telemetrySent = false;

// Indicate whether WiFi is ready
static bool hasWifi = false;

// Indicate whether IoT Hub is ready
static bool hasIoTHub = false;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
static void InitMagnetometer()
{
  Screen.print(2, "Initializing...");
  i2c = new DevI2C(D14, D15);
  lis2mdl = new LIS2MDLSensor(*i2c);
  lis2mdl->init(NULL);
  
  lis2mdl->getMAxes(axes);
  base_x = axes[0];
  base_y = axes[1];
  base_z = axes[2];
  
  int count = 0;
  int delta = 10;
  char buffer[20];
  while (true)
  {
    delay(LOOP_DELAY);
    lis2mdl->getMAxes(axes);
    
    // Waiting for the data from sensor to become stable
    if (abs(base_x - axes[0]) < delta && abs(base_y - axes[1]) < delta && abs(base_z - axes[2]) < delta)
    {
      count++;
      if (count >= EXPECTED_COUNT)
      {
        // Done
        Screen.print(0, "Monitoring...");
        break;
      }
    }
    else
    {
      count = 0;
      base_x = axes[0];
      base_y = axes[1];
      base_z = axes[2];
    }
    sprintf(buffer, "      %d", EXPECTED_COUNT - count);
    Screen.print(3, buffer);
  }
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Arduino sketch
void setup()
{
  Screen.init();
  Screen.print(0, "Initializing...");
  Screen.print(1, " > Magnetometer");
  
  Serial.begin(115200);
  
  InitMagnetometer();
}

void loop()
{
    // Get data from magnetometer sensor
    lis2mdl->getMAxes(axes);
    Serial.printf("Axes: x - %d, y - %d, z - %d\r\n", axes[0], axes[1], axes[2]);

    char buffer[50];

    sprintf(buffer, "x:  %d", axes[0]);
    Screen.print(0, buffer);

    sprintf(buffer, "y:  %d", axes[1]);
    Screen.print(1, buffer);

    sprintf(buffer, "z:  %d", axes[2]);
    //Screen.print(2, buffer);

    //Calculate Heading in Radians
    double heading=0.0;
    if(axes[0] !=0)
      heading=atan2(axes[1],axes[0]);
      

    // CorrecTION #1
    if(heading < 0)
      heading += 2*M_PI;
    
  // CORRECTION #2
    if(heading > 2*M_PI)
      heading -= 2*M_PI;

    //Convert from radians to deg
    heading=heading*180/M_PI;
    int deg=(int) (heading);
    sprintf(buffer, "Deg:  %d", deg);
    Screen.print(2, buffer);

    //Direction Finder N/S/E/Waiting
    char dir[100]="";
    if (270-COMPASS_SENSITIVITY <= deg && deg < 270+COMPASS_SENSITIVITY)
        sprintf(dir, "%s", NORTH);
    else if (90-COMPASS_SENSITIVITY <= deg && deg < 90+COMPASS_SENSITIVITY)  
        sprintf(dir, "%s", SOUTH);
    else if(180-COMPASS_SENSITIVITY <= deg && deg < 180+COMPASS_SENSITIVITY)
        sprintf(dir, "%s", EAST);
    else if(((360-COMPASS_SENSITIVITY <= deg) && (deg <= 360)) || 
        ((0 <=  deg && deg < 0+COMPASS_SENSITIVITY)))
        sprintf(dir, "%s", WEST);
    else if ((0 + COMPASS_SENSITIVITY <= deg) && (deg < 90-COMPASS_SENSITIVITY))
        sprintf(dir, "%s%s", SOUTH, WEST);
    else if ((90+COMPASS_SENSITIVITY <= deg) && (deg < 180-COMPASS_SENSITIVITY))
        sprintf(dir, "%s%s", SOUTH, EAST);
    else if ((180+COMPASS_SENSITIVITY <= deg) && (deg < 270-COMPASS_SENSITIVITY))
        sprintf(dir, "%s%s", NORTH, EAST);
    else
        sprintf(dir, "%s%s", NORTH, WEST);
    
    sprintf(buffer,"Dir: %s",dir);
    Screen.print(3,buffer);
   
 
  delay(LOOP_DELAY);
}
