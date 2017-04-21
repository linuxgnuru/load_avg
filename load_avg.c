#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <wiringPi.h>
#include <sr595.h>

static const int dpPin = 2;

static const int leds[10][7] = {
    //     A   B   C   D   E   F   G
    {  4,  1,  3,  5,  6,  0, -1 }, // 0
    { -1,  1,  3, -1, -1, -1, -1 }, // 1
    {  4,  1, -1,  5,  6, -1,  7 }, // 2
    {  4,  1,  3,  5, -1, -1,  7 }, // 3
    { -1,  1,  3, -1, -1,  0,  7 }, // 4
    {  4, -1,  3,  5, -1,  0,  7 }, // 5
    {  4, -1,  3,  5,  6,  0,  7 }, // 6
    {  4,  1,  3, -1, -1, -1, -1 }, // 7
    {  4,  1,  3,  5,  6,  0,  7 }, // 8
    {  4,  1,  3,  5, -1,  0,  7 }  // 9
};

const int dataPin  = 3; // blue (pin 14)
const int latchPin = 2; // green (pin 12)
const int clockPin = 5; // yellow (pin 11)
const int addr = 100;
int bits = 32;

static void die(int sig)
{
    int i;
    for (i = 0; i < bits; i++)
        digitalWrite(addr + i, 0);
    if (sig != 0 && sig != 2)
        (void)fprintf(stderr, "caught signal %d\n", sig);
    if (sig == 2)
        (void)fprintf(stderr, "Exiting due to Ctrl + C\n");
    exit(0);
}

void printDigit(int digit, int pos, int dp)
{
    int i;
    int pos_ = addr + pos * 8;

    for (i = 0; i < 8; i++)
    {
        digitalWrite(pos_ + i, 0);
    }
    digitalWrite(pos_ + dpPin, dp);
    // if anything is invalid; just return
    if ((digit < 0 || digit > 9) || (pos < 0 || pos > 3) || (dp < 0 || dp > 1))
    {
        printf("ERROR: %d %d %d\n", digit, pos, dp);
        return;
    }
    for (i = 0; i < 7; i++)
    {
        if (leds[digit][i] != -1)
        {
            digitalWrite(pos_ + leds[digit][i], 1);
        }
    }
}

int main(int argc, char **argv)
{
    unsigned long currentMillis, lastMillis;
    char filename[1000];
    sprintf(filename, "/proc/loadavg");
    FILE *f;
    int dp_pos = -1;
    int thou, hund, tens, ones;
    double dig = 0.0;
    float tmp;
    float one_, five_, ten_;
    char unused_s[40];
    int unused_d;

    lastMillis = 0;
    // note: we're assuming BSD-style reliable signals here
    (void)signal(SIGINT, die);
    (void)signal(SIGHUP, die);
    (void)signal(SIGTERM, die);
    if (wiringPiSetup () == -1)
    {
        fprintf(stdout, "oops: %s\n", strerror(errno));
        return 1;
    }
    sr595Setup(addr, bits, dataPin, clockPin, latchPin);
    while (1)
    {
        currentMillis = millis();
        if (currentMillis - lastMillis >= 1000)
        {
            lastMillis = currentMillis;
            f = fopen(filename, "r");
            fscanf(f, "%f %f %f %s %d", &one_, &five_, &ten_, unused_s, &unused_d);
            fclose(f);
            dig = one_;
            if (dig == (int)dig || dig > 1000) // no decimal point
            {
                thou = dig / 1000;
                hund = (int)(dig / 100) % 10;
                tens = (int)(dig / 10) % 10;
                ones = (int)dig % 10;
                dp_pos = 0;
            }
            else // decimal point
            {
                if (dig < 100 && dig > 9)
                {
                    dp_pos = 2;
                    thou = (int)dig / 10;
                    hund = (int)dig % 10;
                    tmp = (dig - hund) * 10;
                    tens = (int)tmp % 10;
                    tmp = (dig * 10 - (thou * 100) - (hund * 10) - tens) * 10;
                    ones = (int)tmp;
                }
                else if (dig < 10)
                {
                    dp_pos = 3;
                    thou = (int)dig % 10;
                    tmp = (dig - thou) * 1000;
                    hund = tmp / 100;
                    tmp = (dig - thou) * 1000 - (hund * 100);
                    tens = (int)tmp / 10;
                    tmp = (dig - thou) * 1000 - (hund * 100) - (tens * 10);
                    ones = (int)tmp % 10;
                }
                else // dig > 99 && dig < 999
                {
                    dp_pos = 1;
                    thou = dig / 100;
                    hund = (int)(dig / 10) % 10;
                    tens = (int)dig % 10;
                    tmp = dig * 10;
                    ones = (int)tmp % 10;
                }
            }
            printDigit(thou, 3, (dp_pos == 3));
            printDigit(hund, 2, (dp_pos == 2));
            printDigit(tens, 1, (dp_pos == 1));
            printDigit(ones, 0, (dp_pos == 0));
        }
    }
    return EXIT_SUCCESS;
}

