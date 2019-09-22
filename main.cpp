/*
 * DustCollector.cpp
 *
 * Created: 9/16/2019 8:32:02 AM
 * Author : Lee
 */ 

#include <avr/io.h>
#include <Timer1Uint32.h>

// Pin 13 - PA0 - OUTPUT - Open Main trunk blast gate
// Pin 12 - PA1 - OUTPUT - Open Drill press and router trunk blast gate
// Pin 11 - PA2 - OUTPUT - Open Miter saw and router trunk blast gate
// Pin 10 - PA3 - OUTPUT - Open Miter saw blast gate
// Pin  9 - PA4 - Available
//              - ALSO DEBUGGER ISP - USCK
// Pin  8 - PA5 - 
//              - ALSO DEBUGGER ISP - MISO
// Pin  7 - PA6 - Available
//              - ALSO DEBUGGER ISP- MOSI
// Pin  6 - PA7 - OUTPUT - Turn on the dust collector
// Pin  2 - PB0 - INPUT - Drill press or router blast gate opened
//                Pull-up resister.
//                Pull LOW to indicate gate is open.
// Pin  3 - PB1 - INPUT - Table saw (top or bottom) blast gate opened
// Pin  5 - PB2 - INPUT - Planer blast gate opened
// Pin  4 - PB3 - INPUT - Open miter saw blast gate
//              - ALSO DEBUGGER RESET - ISP and debugWire

#define DRILL_PRESS_OR_ROUTER_BLAST_GATE_OPENED _BV(PB0)
#define TABLE_SAW_BLAST_GATE_OPENED             _BV(PB1)
#define PLANER_BLAST_GATE_OPENED                _BV(PB2)
#define PLEASE_OPEN_MITER_SAW_BLAST_GATE        _BV(PB3)
#define INPUT_MASK (DRILL_PRESS_OR_ROUTER_BLAST_GATE_OPENED | TABLE_SAW_BLAST_GATE_OPENED | PLANER_BLAST_GATE_OPENED | PLEASE_OPEN_MITER_SAW_BLAST_GATE)

#define OPEN_MAIN_TRUNK_BLAST_GATE                   _BV(PA0)
#define OPEN_DRILL_PRESS_AND_ROUTER_TRUNK_BLAST_GATE _BV(PA1)
#define OPEN_MITER_SAW_AND_PLANER_BLAST_GATE         _BV(PA2)
#define OPEN_MITER_SAW_BLAST_GATE                    _BV(PA3)
#define AVAILABLE_PA4                                _BV(PA4)
#define AVAILABLE_PA5                                _BV(PA5)
#define AVAILABLE_PA6                                _BV(PA6)
#define TURN_ON_DUST_COLLECTOR                       _BV(PA7)  // Dust collector should be on highest pin so it is turned on last
#define OUTPUT_MASK (OPEN_MAIN_TRUNK_BLAST_GATE | OPEN_DRILL_PRESS_AND_ROUTER_TRUNK_BLAST_GATE | OPEN_MITER_SAW_AND_PLANER_BLAST_GATE | OPEN_MITER_SAW_BLAST_GATE | TURN_ON_DUST_COLLECTOR)
#define INVERTED_OUTPUT_MASK (OPEN_MAIN_TRUNK_BLAST_GATE | OPEN_DRILL_PRESS_AND_ROUTER_TRUNK_BLAST_GATE | OPEN_MITER_SAW_AND_PLANER_BLAST_GATE | OPEN_MITER_SAW_BLAST_GATE)

int main(void)
{
    uint8_t outputs = 0;  // Close all blast gates and turn off the dust collector
    PORTA &= (outputs ^ INVERTED_OUTPUT_MASK);
    DDRA |= OUTPUT_MASK;
    DDRB &= ~INPUT_MASK;
    PINB |= INPUT_MASK;  // Enable pull-up resistors

    uint8_t targetOutputs = outputs;

    Timer1Uint32::Start(CLOCK_PRESCALE_DIV_8);
    
    uint32_t nextTime = Timer1Uint32::Now();
    
    while (1) 
    {
        uint8_t inputs = PINB & INPUT_MASK;  // Because of the pull-up resister, 1 means no request and 0 means requested.
        inputs ^= INPUT_MASK; // Flip all the inputs so 1 means the trigger is requested.
        
        // Check the inputs in priority order
        if (inputs & DRILL_PRESS_OR_ROUTER_BLAST_GATE_OPENED)
        {
            // Want only the drill press and router trunk open
            targetOutputs = OPEN_DRILL_PRESS_AND_ROUTER_TRUNK_BLAST_GATE | TURN_ON_DUST_COLLECTOR;
        }
        else if (inputs & TABLE_SAW_BLAST_GATE_OPENED)
        {
            // Only want the main trunk open
            targetOutputs = OPEN_MAIN_TRUNK_BLAST_GATE | TURN_ON_DUST_COLLECTOR;
        }
        else if (inputs & PLANER_BLAST_GATE_OPENED)
        {
            // Need the main trunk and the miter saw/ planer trunk
            targetOutputs = OPEN_MAIN_TRUNK_BLAST_GATE | OPEN_MITER_SAW_AND_PLANER_BLAST_GATE | TURN_ON_DUST_COLLECTOR;
        }
        else if (inputs & PLEASE_OPEN_MITER_SAW_BLAST_GATE)
        {
            targetOutputs = OPEN_MAIN_TRUNK_BLAST_GATE | OPEN_MITER_SAW_AND_PLANER_BLAST_GATE | OPEN_MITER_SAW_BLAST_GATE | TURN_ON_DUST_COLLECTOR;
        }
        else if (outputs & TURN_ON_DUST_COLLECTOR)
        {
            // If there are no requests and the dust collector is on....
            // Just turn off the dust collector (after 10 seconds) and leave the blast gates alone.
            if (targetOutputs & TURN_ON_DUST_COLLECTOR)
            {
                targetOutputs = outputs & ~TURN_ON_DUST_COLLECTOR;
                nextTime = Timer1Uint32::Future(10*Timer1Uint32::ticksPerSecond);                
            }
        }            
        
        if (Timer1Uint32::IsPast(nextTime))
        {
            if (outputs != targetOutputs)
            {
                // Flip the next output
                uint8_t different = (outputs ^ targetOutputs);
                
                // Find the lowest bit that's different
                uint8_t pin = 1;
                while ((different & 1) == 0)
                {
                    different >>= 1;
                    pin <<= 1;
                }
                
                outputs ^= pin; // Flip the bit
                PORTA = (outputs ^ INVERTED_OUTPUT_MASK);

                nextTime = Timer1Uint32::Future(Timer1Uint32::ticksPerSecond);
            }
        }            
    }
}

