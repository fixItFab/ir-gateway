# RC MQTT Gateway

Receiving mqtt message and send rc commands based on [rc-switch](https://github.com/sui77/rc-switch)

Board: ESP8266 / Wemos D1 mini

Command Format:

```JSON
{ code: 4259861, protocol: 1 }
```

## Usage

Rename the `evn.sample.h` file to `evn.h` and enter your wifi and mqtt data
