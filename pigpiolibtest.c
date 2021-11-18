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
    
    gpioSetMode(17, PI_OUTPUT);
    gpioSetPWMfrequency(27, 8000);
    gpioPWM(17,14);
    int c;
    puts ("Enter q to quit");
    do {
        c=getchar();
        putchar (c);
    } while (c != 'q');
    gpioTerminate();

    return 0;
}

