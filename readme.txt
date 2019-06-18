readme.txt
----------

Open points
- restore -O3 compiler optimization
- check LC CRC and generate it
- The LC needs to be initialized for the correct battery
- need reset button, ability to reset (disconnect from debugger halts the target), or need to disconnect battery
- small heartbeat indicator
- re-add getting battery voltage with new IC

User Interface:
- 1x short press: set marker (1x green blink)
- 2x short press: toggle sampling (2x green blink)
- 4x short press: disable communication, go into low power mode
- 9sec press after power-on: format device (red on after power-on, goes out after 3 secs, on at 6s and off at 9s)

- short press while logging: setting marker (confirmed with green LED blink)
- logging: indication with short purple LED
- red blinky LED: failure (e.g. I2C)
- blue LED blinking: connection with host
