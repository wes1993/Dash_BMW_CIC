#!/bin/bash -e

CAN_INTERFACE=can0
CAN_BITRATE=100k
CAN_OSCILATOR=8000000
CAN_INTERRUPT=12

# install required packages:
sudo apt-get install can-utils

echo "Setup can interface to bring up automatically on reboot..."

# setup can interface to bring up automatically on reboot.
FILE_CAN_INTERFACE=/etc/network/interfaces.d/$CAN_INTERFACE
if [ -f "$FILE_CAN_INTERFACE" ]; then
    echo "can0 network interface found, skipping..."
else
    echo "Setting up can0 interface"
    cat > $FILE_CAN_INTERFACE <<EOF
auto $CAN_INTERFACE
iface $CAN_INTERFACE can manual
   pre-up /sbin/ip link set $CAN_INTERFACE type can bitrate $CAN_BITRATE restart-ms 100
   up /sbin/ifconfig $CAN_INTERFACE up txqueuelen 125
   down /sbin/ifconfig $CAN_INTERFACE down
EOF
    echo "File patched: $FILE_CAN_INTERFACE";
    cat $FILE_CAN_INTERFACE
fi

echo "Enable NetworkManager"
sudo systemctl enable systemd-networkd
sudo systemctl start systemd-networkd
sudo systemctl status systemd-networkd

echo "Creating config file..."
FILE_CAN_INTERFACE=/etc/systemd/network/80-can.network
if [ -f "$FILE_CAN_INTERFACE" ]; then
    echo "can0 network interface found, skipping..."
else
    echo "Setting up can0 interface"
    sudo cat > $FILE_CAN_INTERFACE <<EOF
[Match]
Name=$CAN_INTERFACE

[CAN]
BitRate=$CAN_BITRATE
RestartSec=100ms
EOF
    echo "File patched: $FILE_CAN_INTERFACE";
    sudo cat $FILE_CAN_INTERFACE
fi

echo "Enable SPI..."
sudo raspi-config nonint do_spi 0

#echo "Disable boot splash..."
#sudo raspi-config nonint do_boot_splash 1

echo "Disable screen blanking..."
sudo raspi-config nonint do_blanking 1

# patch config.txt
FILE_BOOT_CFG=/boot/config.txt
# detect Raspberry BCM chip
# BCM_CHIP=`cat /proc/cpuinfo | grep -oE 'BCM[0-9]{4}' | tr '[:upper:]' '[:lower:]'`
# if [ -z "$BCM_CHIP" ]; then
#     echo "ERROR: Could not detect BCM chip. Exiting..."
#     exit
# fi
# check if config already patched
if grep -q ^dtoverlay=mcp2515 "$FILE_BOOT_CFG"; then 
    echo "config.txt already patched, skipping..."
else    
    # check if file writable
    if [ ! -w "$FILE_BOOT_CFG" ]; then
        echo "/boot not writable, remounting"
        sudo mount -o remount,rw /boot
    fi
    sudo cat >> $FILE_BOOT_CFG <<EOF

# CAN bus
dtoverlay=mcp2515,spi0-0,oscillator=$CAN_OSCILATOR,interrupt=$CAN_INTERRUPT
EOF
    echo "File patched: $FILE_BOOT_CFG"; 
fi

echo "Done. Restart raspberry to enable CAN interface".
echo "If needed, use command to restart CAN interface: sudo systemctl restart systemd-networkd"
