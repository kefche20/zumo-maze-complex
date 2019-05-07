#include <Wire.h>
#include <ZumoShield.h>
#define SENSOR_THRESHOLD 300
# define ABOVE_LINE(sensor)((sensor) > SENSOR_THRESHOLD)
# define TURN_SPEED 400
# define SPEED 360
# define LINE_THICKNESS .80
# define INCHES_TO_ZUNITS 17142.0
# define OVERSHOOT(line_thickness)(((INCHES_TO_ZUNITS * (line_thickness)) / SPEED))
ZumoBuzzer buzzer;
ZumoReflectanceSensorArray reflectanceSensors;
ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);

char path[1000] = "";
unsigned char path_length = 0;

void setup() 
{

  unsigned int sensors[6];
  unsigned short count = 0;
  unsigned short last_status = 0;
  int turn_direction = 1;

  reflectanceSensors.init();

  delay(500);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  button.waitForButton();

  for (int i = 0; i < 4; i++) 
  {

    turn_direction *= -1;

    motors.setSpeeds(turn_direction * (TURN_SPEED-100), -1 * turn_direction * (TURN_SPEED-100));

    while (count < 2) 
  {
      reflectanceSensors.calibrate();
      reflectanceSensors.readLine(sensors);
    
      if (turn_direction < 0) 
    {
        count += ABOVE_LINE(sensors[5]) ^ last_status;
        last_status = ABOVE_LINE(sensors[5]);
      } 
    else      
    {
        count += ABOVE_LINE(sensors[0]) ^ last_status;
        last_status = ABOVE_LINE(sensors[0]);
      }
    }

    count = 0;
    last_status = 0;
  }

  turn('L');

  motors.setSpeeds(0, 0);

  digitalWrite(13, LOW);
}

void turn(char dir) 
{

  unsigned short count = 0;
  unsigned short last_status = 0;
  unsigned int sensors[6];

  switch (dir) 
  {

   case 'L':
    case 'B':

    motors.setSpeeds(-TURN_SPEED, TURN_SPEED); //TURN_SPEED

    while (count < 2) 
    {
      reflectanceSensors.readLine(sensors);

      count += ABOVE_LINE(sensors[1]) ^ last_status;
      last_status = ABOVE_LINE(sensors[1]);
    }
    break;

   case 'R':

    motors.setSpeeds(TURN_SPEED, -TURN_SPEED); //TURN_SPEED

    while (count < 2) 
    {
      reflectanceSensors.readLine(sensors);
      count += ABOVE_LINE(sensors[4]) ^ last_status;
      last_status = ABOVE_LINE(sensors[4]);
    }
    break;

   case 'S':
    break;
  }
}

char selectTurn(unsigned char found_left, unsigned char found_straight, unsigned char found_right) 
{

  if (found_left)
    return 'L';
  else if (found_right)
    return 'R';
  else if (found_straight)
    return 'S';
  else
    return 'B';
}

void followSegment() 
{
  unsigned int position;
  unsigned int sensors[6];
  int offset_from_center;
  int power_difference;

  while (1) 
  {
    position = reflectanceSensors.readLine(sensors);

    offset_from_center = ((int) position) - 2500;

    power_difference = offset_from_center / 3;

    if (power_difference > SPEED)
      power_difference = SPEED;
  
    if (power_difference < -SPEED)
      power_difference = -SPEED;

    if (power_difference < 0)
      motors.setSpeeds(SPEED + power_difference, SPEED);
    else
      motors.setSpeeds(SPEED, SPEED - power_difference);

    if (!ABOVE_LINE(sensors[0]) && !ABOVE_LINE(sensors[1]) && !ABOVE_LINE(sensors[2]) && !ABOVE_LINE(sensors[3]) && !ABOVE_LINE(sensors[4]) && !ABOVE_LINE(sensors[5])) 
  {

      return;
    
    }
// && ili || ????????????????????????????????????????????????????????????????????????????????
  else if (ABOVE_LINE(sensors[0]) && ABOVE_LINE(sensors[5])) 
  {

      return;
    }
  }
}

void solveMaze() 
{
  while (1) 
  {

    followSegment();

    unsigned char found_left = 0;
    unsigned char found_straight = 0;
    unsigned char found_right = 0;

    unsigned int sensors[6];
    reflectanceSensors.readLine(sensors);

    /*if (ABOVE_LINE(sensors[0]))
      found_left = 1;
    if (ABOVE_LINE(sensors[5]))
      found_right = 1;

    motors.setSpeeds(150, 150);
    delay(OVERSHOOT(LINE_THICKNESS) / 2);

    reflectanceSensors.readLine(sensors);*/

    motors.setSpeeds(SPEED, SPEED);
    delay(OVERSHOOT(LINE_THICKNESS) / 2);
    reflectanceSensors.readLine(sensors);
    if(!ABOVE_LINE(sensors[0]) && !ABOVE_LINE(sensors[1]) && !ABOVE_LINE(sensors[2]) && !ABOVE_LINE(sensors[3]) && !ABOVE_LINE(sensors[4]) && !ABOVE_LINE(sensors[5]))
    {
      found_straight = 1;
    }

    if (ABOVE_LINE(sensors[1]) || ABOVE_LINE(sensors[2]) || ABOVE_LINE(sensors[3]) || ABOVE_LINE(sensors[4]))
  {
      found_straight = 1;
  }

    if (ABOVE_LINE(sensors[1]) && ABOVE_LINE(sensors[2]) && ABOVE_LINE(sensors[3]) && ABOVE_LINE(sensors[4])) 
  {
     /* motors.setSpeeds(0, 0);
      break;*/found_straight = 1;
    }
  
    if (ABOVE_LINE(sensors[0]))
      found_left = 1;
  
    if (ABOVE_LINE(sensors[5]))
      found_right = 1;   
  
    if(ABOVE_LINE(sensors[0]) && ABOVE_LINE(sensors[5]))
    {
      found_straight = 1;
      found_left = 0;
      found_right = 0;
    }

  //motors.setSpeeds(SPEED, SPEED);
    //delay(OVERSHOOT(LINE_THICKNESS) / 2);

    unsigned char dir = selectTurn(found_left, found_straight, found_right);

    turn(dir);

    path[path_length] = dir;
    path_length++;

    simplifyPath();
  }
}

void goToFinishLine() 
{
  unsigned int sensors[6];
  int i = 0;

  if (path[0] == 'B') 
  {
    turn('B');
    i++;
  }

  for (; i < path_length; i++) 
  {
    followSegment();

    motors.setSpeeds(SPEED, SPEED);
    delay(OVERSHOOT(LINE_THICKNESS));

    turn(path[i]);
  }

  followSegment();

  reflectanceSensors.readLine(sensors);
  motors.setSpeeds(0, 0);

  return;
}

void simplifyPath() 
{
  if (path_length < 3 || path[path_length - 2] != 'B')
    return;

  int total_angle = 0;
  int i;

  for (i = 1; i <= 3; i++) 
  {
    switch (path[path_length - i]) 
  {
    case 'R':
      total_angle += 90;
      break;
    case 'L':
      total_angle += 270;
      break;
    case 'B':
      total_angle += 180;
      break;
    }
  }

  total_angle = total_angle % 360;

  switch (total_angle) 
  {
    case 0:
    path[path_length - 3] = 'S';
    break;
    case 90:
    path[path_length - 3] = 'R';
    break;
    case 180:
    path[path_length - 3] = 'B';
    break;
    case 270:
    path[path_length - 3] = 'L';
    break;
  }

  path_length -= 2;
}

void loop() 
{
  buzzer.play(">g32>>c32");
  solveMaze();

  while (1) 
  {
    button.waitForButton();
    goToFinishLine();
  }
}
