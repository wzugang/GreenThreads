#include <stdio.h>
#include <stdlib.h>
#include "channel.h"
#include "scheduler.h"

// Example function. Hailstone sequence
void example_func(uint8_t start)
{
    printf("Start is: [%d]\n", start);
    while (start > 1)
    {
        if (start % 2 == 0)
        {
            start = start / 2;
        }
        else
        {
            start = start * 3 + 1;
        }
        printf("  Val: %u\n", start);
    }
}

void average_novararg(uint8_t one, uint8_t two, uint8_t three)
{
    printf("One    : %3u, address: %X\n", one, &one);
    printf("Two    : %3u, address: %X\n", two, &two);
    printf("Three  : %3u, address: %X\n", three, &three);
    yield(1);
    printf("Average: %f\n", (one + two + three) / 3.0);
}

void hailstone(uint32_t start)
{
    uint32_t iter = start;
    uint32_t count = 1;
    printf("Start at %u\n", start);
    while (iter > 1)
    {
        if (iter % 2 == 0)
        {
            iter /= 2;
            count++;
        }
        else
        {
            iter = iter * 3 + 1;
            count++;
        }
        yield(1);
    }
    printf("%5u takes %5u (%X) steps.\n", start, count, &count);
}

int fib(int x)
{
    if (x == 0)
    {
        return 0;
    }
    if (x == 1)
    {
        return 1;
    }
    yield(1);
    return fib(x-1)+fib(x-2);
}

void printFib(int x)
{
    int result = fib(x);
    printf("The %uth fibonacci number is %u\n", x, result);
}

void numProducer(uint32_t num, uint32_t willProduce, Channel* chan)
{
    uint32_t multiplier = 1;
    uint32_t i = 0;
    while (i < willProduce)
    {
        uint32_t written = write(chan, 1, num * multiplier);
        if (written == 1)
        {
            printf("Produced value %5u on channel %X\n", num * multiplier, chan);
            multiplier++;
            i++;
        }
        else
        {
            printf("Value not yet read on channel %X\n", chan);
        }
        yield(1);
    }
}

void numConsumer(uint32_t expects, Channel* chan)
{
    uint32_t numRead = 0;
    uint32_t value;
    uint32_t success = 0;
    while (numRead < expects)
    {
        yield(1);
        success = read_1(chan, &value);
        if (success)
        {
            numRead++;
            printf("Got value %5u from channel %X\n", value, chan);
        }
        else
        {
            printf("No value available on channel %X\n", chan);
        }
    }
}

void spawnInner(int count, int endVal)
{
    printf("Entered spawnInner with count: %u :: endVal: %u\n", count, endVal);
    if (count < endVal)
    {
        int i;
        for (i = 0; i <= count; i++)
        {
            newProc(sizeof(uint32_t) * 2, &spawnInner, count + 1, endVal);
            printf("  Created new thread. count: %u :: endVal: %u\n", count + 1, endVal);
            printf("    Yielding...\n");
            yield(1);
        }
    }
}

void spawnMany(int endVal)
{
    printf("Exec-ing spawnInner with endVal: %u\n", endVal);
    spawnInner(0, endVal);
}

// newProc(uint32_t argBytes, void* funcAddr, uint8_t* args);

int main(int argc, char** argv)
{
    initThreadManager();

    newProc(sizeof(uint32_t) * 3, &average_novararg, 10, 20, 30);
    newProc(sizeof(uint32_t) * 3, &average_novararg, 20, 30, 40);
    newProc(sizeof(uint32_t) * 3, &average_novararg, 30, 40, 50);

    newProc(sizeof(uint32_t) * 1, &hailstone, 10);
    newProc(sizeof(uint32_t) * 1, &hailstone, 15);
    newProc(sizeof(uint32_t) * 1, &hailstone, 20);
    newProc(sizeof(uint32_t) * 1, &hailstone, 25);
    newProc(sizeof(uint32_t) * 1, &hailstone, 30);
    newProc(sizeof(uint32_t) * 1, &hailstone, 35);

    newProc(sizeof(uint32_t) * 1, &printFib, 10);
    newProc(sizeof(uint32_t) * 1, &printFib, 20);
    newProc(sizeof(uint32_t) * 1, &printFib, 30);

    Channel* chan_1 = createChannel(1);
    Channel* chan_2 = createChannel(1);
    Channel* chan_3 = createChannel(1);
    newProc(sizeof(uint32_t) * 3, &numProducer, 10, 5, chan_1);
    newProc(sizeof(uint32_t) * 3, &numProducer, 11, 5, chan_2);
    newProc(sizeof(uint32_t) * 3, &numProducer, 13, 5, chan_3);
    newProc(sizeof(uint32_t) * 2, &numConsumer, 5, chan_1);
    newProc(sizeof(uint32_t) * 2, &numConsumer, 5, chan_2);
    newProc(sizeof(uint32_t) * 2, &numConsumer, 5, chan_3);

    newProc(sizeof(uint32_t) * 1, &spawnMany, 3);

    execAllManagedFuncs();

    takedownThreadManager();
    destroyChannel(chan_1);
    destroyChannel(chan_2);
    destroyChannel(chan_3);

    return 0;
}
