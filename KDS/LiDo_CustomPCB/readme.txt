readme.txt
----------
Note: using the LC709203Fxx-01 version

Open points
- Note: restore -O3 compiler optimization
- Note: currently runtime statistics are enabled, need to be disabled for low power
- Note: currently Segger systemview is enabled, needs to be disabled for lower power
- Note: Reenable watchdog
- The LC needs to be initialized for the correct battery. Need to calibrate Accel temperature sensor
- Need to measure temperature for I2C gauge IC mode
- Use temperature sensor of AS sensor
- need reset button, ability to reset (disconnect from debugger halts the target), or need to disconnect battery (==> V2.2)
- small heartbeat indicator
- provide complete interface to gauge IC
- check extra (alarm) pins
- check waiting for SPI in FS driver
- verify with supply voltage oszillates with no battery and only USB attached?
- RTC clock settings after Reset not set?
- reset button does not work if in low power mode?
- need glas/enclosure for light sensor?
- keep communication open if RTT/debugger is connected
- todo: use bat emergency pin to cut off power in PowerMangement.c (update it for new gauge IC)
- V2.2: disabled reset pin in CPU?
- after changing the appData.ini file, I have to do a KIN1 reset to start the sampling task again? ==> issue is that the appdata file was empty! see below.
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
- device is powered down:
  - 1 sec press: wake up => active mode, blinks white LED

- active device:
  - 1x short press: set marker (1x green blink) if sampling
  - 2x short press: toggle sampling (2x green blink)
  - 3x short press: no functionality
  - 4x short press: disable communication/shell, go into low power mode
  - 5x short press: software reset
  - 6x short press: power down
  - (9sec press after power-on: format device (red on after power-on, goes out after 3 secs, on at 6s and off at 9s))

Status:
- logging: indication with short purple LED blink
- red blinky LED: failure (e.g. I2C)
- blue/green LED blinking: connection with host. green: charging

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

Energy:
- small battery: 0.4 Wh capacity
- device turned off: 3.08 uA current (mostly gauge IC)
- 1 Hz, no code optimization, adaptive (no watchdog): 3.75V, 1 Minute: 137 uWh (avg 2.72mA), 1h 8.22 mWh, 1 day 197 mWh

- New:
 1 minute with blue LWU blinky: avr. 119 uA, E 7.45 uWh ==> 10'728 uWh => 10.728 mWh ==> 0.4 Wh Batt => 36 Tage Idle mode
 

