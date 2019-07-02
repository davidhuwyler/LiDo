readme.txt
----------
Note: using the LC709203Fxx-01 version

Open points
- restore -O3 compiler optimization
- The LC needs to be initialized for the correct battery
- need reset button, ability to reset (disconnect from debugger halts the target), or need to disconnect battery (==> V2.2)
- V2.2 will have FETs for sensors and SPI removed
- small heartbeat indicator
- re-add getting battery voltage with new IC
- provide complete interface to gauge IC
- add I2C temperature mode
- check extra (alarm) pins
- check waing for SPI in FS driver
- V2.1 Board: do not get from charing IC the charging state (always low?)

User Interface:
- 1x short press: set marker (1x green blink)
- 2x short press: toggle sampling (2x green blink)
- 4x short press: disable communication, go into low power mode
- 9sec press after power-on: format device (red on after power-on, goes out after 3 secs, on at 6s and off at 9s)

- short press while logging: setting marker (confirmed with green LED blink)
- logging: indication with short purple LED
- red blinky LED: failure (e.g. I2C)
- blue LED blinking: connection with host

Re-enable communication:
- turn on device (5x button) when connected to USB
- device implements SEGGER RTT

Links:
- https://github.com/kirananto/RaZorReborn/blob/master/drivers/power/yl_lc709203_fuelgauge.c
