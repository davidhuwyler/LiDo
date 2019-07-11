readme.txt
----------
Note: using the LC709203Fxx-01 version

Open points
- Note: restore -O3 compiler optimization
- Note: currently runtime statistics are enabled, need to be disabled for low power
- Note: currently Segger systemview is enabled, needs to be disabled for lower power
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
- Reenable watchdog
- supply voltage oszillates with no battery and only USB attached?
- Temperature sensor? Calibration?
- Uhr nach Reset nicht gesetzt?
- reset button does not work if in low power mode?
- need glas/enclosure for light sensor?
- keep communication open if RTT/debugger is connected
- todo: use bat emergency pin to cut off power in PowerMangement.c
- watchdog triggers if using the Shell
- had empty file?
CMD>   < AppData print
00> LIDO_NAME = -
00> LIDO_ID = -
00> LIDO_VERSION = -
00> SAMPLE_INTERVALL = -
00> SAMPLE_ENABLE = 1
00> LIGHTSENS_GAIN = -
00> LIGHTSENS_INTT = -
00> LIGHTSENS_WTIM = -
00> AUTOGAIN_ENABLE = -
00> SAMPLE_AUTO_OFF = -

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


Power Concept:
- With the USB connected (5V), it turns on the FET (T3) and powers the target
- Or: User presses the push button
- uC can power down Vcc with the DO_PWR_ON pin
==> Reset button does not help to wake up processor from low power mode?

Debug without USB cable:
- press button and keep it pressed to keep board powered.
- run code until power pin/FET is pulled high (PIN_POWER_ON, is done during PE_low_level_init())

