/******************************************************************************
 *   Header Includes
 *******************************************************************************/
#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>

#include "eecs_388_lib.h"

/******************************************************************************
 *   Constant definitions
 *******************************************************************************/

/*ServoMotor constants*/
#define SERVO_PULSE_MAX (2400)    /* 2400 us */
#define SERVO_PULSE_MIN (544)     /* 544 us */
#define SERVO_PERIOD    (20000)   /* 20000 us (20ms) */
#define MIN_ANGLE       (0)       /* degrees */
#define MAX_ANGLE       (180)     /* degrees */

/*Auto-brake LED pins*/
#define LED_RED         GPIO_13
#define LED_GREEN       GPIO_12
#define LED_BLUE        GPIO_11

/*Auto-brake distance thresholds (cm)*/
#define SAFE_DIST       (200)     /* d > 200        -> safe        */
#define LIGHT_DIST      (100)     /* 100 < d <= 200 -> brake light */
#define HARD_DIST       (60)      /* 60 < d <= 100  -> brake hard  */
                                  /* d <= 60        -> must stop   */

/*LED timer: Timer1 in CTC mode, prescaler 256 -> 62500 ticks/s (16us/tick)
  50ms tick -> red LED toggles every 50ms -> flashing period = 100ms*/
#define LED_TICK_MS     (50)
#define TIMER1_OCR      ((F_CPU / 256UL / 1000UL) * LED_TICK_MS - 1)

/*Test mode: cycle through fake distances until Task 3 (receiving from
  arduino1) is done. Set to 0 once real distances are coming in.*/
#define USE_TEST_DISTANCES (1)
#define TEST_HOLD_MS       (3000)

typedef enum
{
    SAFE_STATE,       /* Green           */
    BRAKE_LIGHTLY,    /* Yellow (R + G)  */
    BRAKE_HARD,       /* Red             */
    UNSAFE_STATE      /* Flashing red    */
} brake_state_t;

/******************************************************************************
 *   Global variables
 *******************************************************************************/
/*Shared with the timer ISR, so it must be volatile*/
static volatile brake_state_t brake_state = SAFE_STATE;

/******************************************************************************
 *   Local function prototypes
 *******************************************************************************/
static void auto_brake(uint16_t distance);
static void steering(int pos);
static void led_timer_init();

/******************************************************************************
 *   Function: ISR(TIMER1_COMPA_vect) - LED update timer
 *      Pre condition:
 *          led_timer_init() has been called
 *      Post condition:
 *          Fires every LED_TICK_MS and drives the LEDs from brake_state.
 *          In UNSAFE_STATE the red LED is toggled to make it flash.
 *******************************************************************************/
ISR(TIMER1_COMPA_vect)
{
    static uint8_t flash_on = 0;

    switch (brake_state)
    {
        case SAFE_STATE:
            gpio_write(LED_RED,   OFF);
            gpio_write(LED_GREEN, ON);
            break;

        case BRAKE_LIGHTLY:
            gpio_write(LED_RED,   ON);
            gpio_write(LED_GREEN, ON);
            break;

        case BRAKE_HARD:
            gpio_write(LED_RED,   ON);
            gpio_write(LED_GREEN, OFF);
            break;

        case UNSAFE_STATE:
        default:
            flash_on = !flash_on;
            gpio_write(LED_RED,   flash_on ? ON : OFF);
            gpio_write(LED_GREEN, OFF);
            break;
    }

    gpio_write(LED_BLUE, OFF);
}

/******************************************************************************
 *   Function: led_timer_init() - Configure Timer1 for the LED ISR
 *      Pre condition:
 *          None
 *      Post condition:
 *          Timer1 generates a compare-match interrupt every LED_TICK_MS
 *******************************************************************************/
static void led_timer_init()
{
    cli();
    TCCR1A = 0;                             /* normal port operation       */
    TCCR1B = (1 << WGM12) | (1 << CS12);    /* CTC mode, prescaler 256     */
    TCNT1  = 0;
    OCR1A  = TIMER1_OCR;                    /* 3124 -> 50ms                */
    TIMSK1 = (1 << OCIE1A);                 /* enable compare A interrupt  */
    sei();
}

/******************************************************************************
 *   Function: auto_brake() - Auto Brake
 *      Pre condition:
 *          None
 *      Post condition:
 *          Checks the LiDAR distance and sets the braking state.
 *          The LEDs themselves are driven by the Timer1 ISR.
 *******************************************************************************/
static void auto_brake(uint16_t distance)
{
    brake_state_t new_state;

    if (distance > SAFE_DIST)
    {
        new_state = SAFE_STATE;
    }
    else if (distance > LIGHT_DIST)
    {
        new_state = BRAKE_LIGHTLY;
    }
    else if (distance > HARD_DIST)
    {
        new_state = BRAKE_HARD;
    }
    else
    {
        new_state = UNSAFE_STATE;
    }

    if (new_state != brake_state)
    {
        brake_state = new_state;    /* single byte write -> atomic on AVR */
        ser_printf("Distance: %u cm -> state %d", distance, (int)new_state);
    }
    // Task-2: 
    // Your code goes here (Use Lab 2 & 4 for reference)
    // Check the project document to understand the task
}


/******************************************************************************
 *   Function: steering() - Steering
 *      Pre condition: 
 *          None
 *      Post condition: 
 *          Control the servomotor with GPIO_6
 *******************************************************************************/
static void steering(int pos)
{
    // Task-4: 
    // Your code goes here (Use Lab 05 for reference)
    // Check the project document to understand the task
}

/******************************************************************************
 *   Function: setup() - Initializes the Arduino System
 *      Pre condition: 
 *          Hardware must be properly connected (BMP180 sensors, etc.)
 *      Post condition: 
 *          Runs initialization calls one time on power up
 *          UART is initialized for ser_printf()
 *          I2C is initialized for communication with BMP180
 *          calib_data is filled with calibration data from BMP180
 *******************************************************************************/
void setup() 
{
uart_init();     // Initialize UART for serial output

//Setup Auto-break LEDs for Distance
gpio_mode(LED_RED,   GPIO_OUTPUT); //RED
gpio_mode(LED_GREEN, GPIO_OUTPUT); //GREEN
gpio_mode(LED_BLUE,  GPIO_OUTPUT); //BLUE

//Timer1 ISR updates the brake LEDs every 50ms
led_timer_init();

//Setup GPIO_6 for PWM output
gpio_mode(GPIO_6, GPIO_OUTPUT);

ser_printf("System Initialized");
}

/******************************************************************************
 *   Function: loop() - Main execution loop
 *      Pre condition: 
 *          setup() has been executed and system is initialized
 *      Post condition: 
 *          Performs a single iteration of the system's function
 *          Repeates indefinetely unless the board is reset or powered off
 *******************************************************************************/
void loop() 
{
// Task-4: 
// Setup simulated code for the angles from the lab sheet
int angle;

// Task-1&2:
// Receive data from arudino1 for the distance and setup the LEDs
#if USE_TEST_DISTANCES
// Until Task 3 is done, step through one distance from each braking range
static const uint16_t test_distances[] = {250, 150, 80, 40};
static uint8_t test_idx = 0;
static unsigned long last_change = 0;

if (millis() - last_change >= TEST_HOLD_MS)
  {
  last_change = millis();
  test_idx = (test_idx + 1) % (sizeof(test_distances) / sizeof(test_distances[0]));
  }
auto_brake(test_distances[test_idx]);
#else
// Task-3: replace with the distance received from arduino1
uint16_t distance = 0;
auto_brake(distance);
#endif

//  Task-4:
for (int i = 0; i < 10; i++)
  {
  // Here, we set the angle to 180 if the prediction from the DNN is a positive angle
  // and 0 if the prediction is a negative angle.
  // This is so that it is easier to see the movement of the servo.
  // You are welcome to pass the angle values directly to the steering function.
  // If the servo function is written correctly, it should still work,
  // only the movements of the servo will be more subtle
  if( angle > 0 )
    {
    steering(180);
    }
  else 
    {
    steering(0);
    }
           
  // Uncomment the line below to see the actual angles on the servo.
  // Remember to comment out the if-else statement above!
  // steering(angle);
  }
}
