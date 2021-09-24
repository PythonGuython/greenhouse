#include <wiringPi.h>
#include <stdio.h>

#define LedPin 0
int main(){

    if (wiringPiSetup() == -1) {
    printf("wiringPi setup failed/n");
    return -1;}
    
    else{
    printf("wiring setup successful");
    }
    
    
    return 0;
}