# Dash_BmwF10_Idrive integration

This repo contains a Vehicle Plugin for [Dash](https://github.com/OpenDsh/dash/) to support a 2010 BMW F10.
Short showcase [video](https://www.youtube.com/watch?v=plySBRlPMZQ)

Current functionality:

* Idrive rotary input to scroll OpenAuto UI
* Idrive left/right/up/down/press to simulate OpenAuto UI keyboard input
* Reverse camera activation when gear shifter in Reverse.

It is likely that this plugin works with other years of F01/F07F10/F11 or even across the BMW lines. However I make no garuntees.

# Installation

* Checkout repository to [dash/plugins/vehicle](https://github.com/openDsh/dash/tree/develop/plugins/vehicle) folder

```
$ cd ~/dash/plugins/vehicle
$ git clone https://github.com/wes1993/Dash_BMW_CIC.git
```

* run install script

```
$ cd ~/dash
$ install.sh --dash
```
#Remember to DISABLE can Messages from dash otherwise the CAR ECUs won't power off.. (Check LED of Start/Stop button)
Comment OUT the line "bus->writeFrame(cmd.frame);" in “src/app/pages/vehicle.cpp” of Dash Project:
```
    this->timer = new QTimer(this);
    connect(this->timer, &QTimer::timeout, [this, bus, cmds]() {
        for (auto cmd : cmds) {
            // bus->writeFrame(cmd.frame);
        }
    });
```

# Hardware

- cheap [MCP2515 board](https://www.aliexpress.com/item/4000548754013.html?spm=a2g0s.9042311.0.0.27424c4dsagQ4T). Keep in mind that you need to connect board VCC  to Raspberry 3.3V pin, not 5V pin. It works fine. 

# Automatic CAN interface startup

In order to start CAN interface automatically, you will need to add following line in `/etc/rc.local` file, just before last line `exit 0`:
```
sudo ip link set can0 up type can bitrate 100000 restart-ms 100
```
Note last parameter *restart-ms*, it is necessary to recover CAN interface from **of-bus** state.

# Links

 - Very detailed and nice [tutorial](https://www.raspberrypi.org/forums/viewtopic.php?t=141052) how to connect everything. Note, that board modification isn't needed. It works fine of 3.3V PIN.
