#include <stdio.h>
#include <pigpio.h>
#include <time.h>

void delay(int ms)
{
    time_t startTime = clock();
    printf("starting while/n");
    while (clock() < startTime + ms);
    printf("ending while/n");
}

int main()
{
    if (gpioInitialise() < 0)
    {
        printf("pigpio initialise failed/n");
        return -1;
    }
    
    gpioSetMode(27, PI_OUTPUT);
    gpioSetPWMfrequency(27, 8000);
    gpioPWM(27,14);
    while(1){
        
    }
    
    gpioTerminate();

    return 0;
}

