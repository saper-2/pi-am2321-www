# AM2321 access application
This directory contains files for application that accessing AM2321 and reads it's data.

There is need for external application, because the sensor using non-standard i2c protocol (it encapsulate MODBUS RTU on I2C bus :scream: ).

Using *am2321-read.sh* script from Cron automates data reading.
See file content for more information.

To build application just call:
```
./build.sh
```

