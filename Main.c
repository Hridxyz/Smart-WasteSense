#include <LPC17xx.h>
#include <system_LPC17xx.h>

#define PRESCALE 29999999            // Prescale value for timer
#define TRIG (1 << 15)               // Trigger pin P0.15
#define ECHO (1 << 16)               // Echo pin P0.16
#define LED_PIN (1 << 8)             // LED pin P0.8
#define STEPPER_CTRL_MASK (0xF << 4) // P0.4 to P0.7 for stepper motor controls

unsigned long int var1;
unsigned long int i, j, k;
int echoTime = 5000;
float distance = 0;

enum State
{
    IDLE,
    ROTATE_CLOCKWISE,
    WAIT,
    ROTATE_ANTICLOCKWISE
};

enum State currentState = IDLE;
int stepCounter = 0;
int waitCounter = 0;

void initTimer0(void);
void startTimer0(void);
float stopTimer0(void);
void delayUS(unsigned int microseconds);
void delay(unsigned int r1);
void controlStepperMotor(int enable);
void rotateOnceClockwise(void);
void rotateOnceAnticlockwise(void);

int main()
{
    SystemInit();
    SystemCoreClockUpdate();
    initTimer0();

    // Configuring trigger, echo, LED, and stepper motor controls
    LPC_PINCON->PINSEL0 &= ~(0xFF << 4);                     // Set P0.4 to P0.7 as GPIO for stepper control
    LPC_GPIO0->FIODIR |= TRIG | STEPPER_CTRL_MASK | LED_PIN; // Set pins as output
    LPC_GPIO1->FIODIR &= ~ECHO;                              // Set ECHO pin as input

    LPC_GPIO0->FIOCLR = TRIG; // Ensure TRIG is initially low

    while (1)
    {
        // Trigger ultrasonic sensor
        LPC_GPIO0->FIOSET = TRIG;
        delayUS(10); // 10us high pulse
        LPC_GPIO0->FIOCLR = TRIG;

        // Wait for echo
        startTimer0();
        while (!(LPC_GPIO0->FIOPIN & ECHO))
            ; // Wait for echo high
        while (LPC_GPIO0->FIOPIN & ECHO)
            ; // Wait for echo low
        echoTime = stopTimer0();
        distance = (0.0343 * echoTime) / 2;

        // Act based on distance
        if (distance < 40)
        {
            LPC_GPIO0->FIOSET = LED_PIN; // Turn on the LED
            if (currentState == IDLE)
            {
                currentState = ROTATE_CLOCKWISE;
                stepCounter = 0;
            }
        }
        else
        {
            LPC_GPIO0->FIOCLR = LED_PIN; // Turn off the LED
            if (currentState == ROTATE_CLOCKWISE)
            {
                currentState = WAIT;
                waitCounter = 0;
            }
        }

        // State machine for stepper motor control
        switch (currentState)
        {
        case ROTATE_CLOCKWISE:
            if (stepCounter < 50)
            {
                controlStepperMotor(1); // Clockwise rotation
                stepCounter++;
            }
            else
            {
                currentState = WAIT;
                stepCounter = 0;
            }
            break;
        case WAIT:
            delay(7000000); // Delay for responsiveness
            waitCounter++;
            if (waitCounter >= 4)
            {
                currentState = ROTATE_ANTICLOCKWISE;
                stepCounter = 0;
            }
            break;
        case ROTATE_ANTICLOCKWISE:
            if (stepCounter < 50)
            {
                controlStepperMotor(0); // Anticlockwise rotation
                stepCounter++;
            }
            else
            {
                currentState = IDLE;
                stepCounter = 0;
            }
            break;
        default:
            break;
        }
        delay(88000); // Delay between sensor checks
    }
}

void initTimer0(void)
{
    LPC_TIM0->CTCR = 0;   // Use timer mode
    LPC_TIM0->PR = 11999; // Prescale for 1ms @ 12MHz
    LPC_TIM0->TCR = 2;    // Reset timer
}

void startTimer0(void)
{
    LPC_TIM0->TCR = 2; // Reset timer
    LPC_TIM0->TCR = 1; // Start timer
}

float stopTimer0(void)
{
    LPC_TIM0->TCR = 0; // Stop timer
    return LPC_TIM0->TC;
}

void delayUS(unsigned int microseconds)
{
    LPC_SC->PCLKSEL0 &= ~(0x3 << 2); // Set PCLK_TIMER0 to divide by 1 LPC_TIM0->TCR = 0x02; // Reset timer LPC_TIM0->PR = 0; // Set prescaler to 0 LPC_TIM0->MR0 = microseconds - 1; // Set MR for the specified microseconds
    LPC_TIM0->MCR = 0x01;
    LPC_TIM0->TCR = 0x01;
    while ((LPC_TIM0->IR & 0x01) == 0)
        ;
    LPC_TIM0->TCR = 0x00;
    LPC_TIM0->IR = 0x01;
}

void delay(unsigned int r1)
{
    volatile int r;
    for (r = 0; r < r1; r++)
        ;
}

void controlStepperMotor(int action)
{
    if (action == 1)
    {
        // Clockwise rotation
        LPC_GPIO0->FIOPIN |= STEPPER_CTRL_MASK;
        for (j = 0; j < 150; j++)
        {
            rotateOnceClockwise();
        }
    }
    else if (action == 0)
    {
        // Anticlockwise rotation
        LPC_GPIO0->FIOPIN |= STEPPER_CTRL_MASK;
        for (j = 0; j < 3; j++)
        {
            rotateOnceAnticlockwise();
        }
    }
    else
    {
        LPC_GPIO0->FIOPIN &= ~STEPPER_CTRL_MASK;
    }
}

void rotateOnceClockwise()
{
    var1 = 0x8;
    for (i = 0; i < 4; i++)
    {
        var1 = var1 << 1;
        LPC_GPIO0->FIOPIN = ~var1;
        for (k = 0; k < 25000; k++)
            ;
    }
}

void rotateOnceAnticlockwise()
{
    var1 = 0x100;
    for (i = 0; i < 4; i++)
    {
        var1 = var1 >> 1;
        LPC_GPIO0->FIOPIN = ~var1;
        for (k = 0; k < 25000; k++)
            ;
    }
}
