#include <wiringPi.h>
#include <stdio.h>

#define LedPin 0
int main(){

    if (wiringPiSetup() == -1) {
        printf("wiringPi setup failed/n");
        return -1;
    }
    
    else{
    printf("wiring setup successful");
    }

    digitalWrite(LedPin, HIGH);
    delay(10000);
    digitalWrite(LedPin, LOW);
    
    
    return 0;
}