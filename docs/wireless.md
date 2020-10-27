# Raspberry Pi wireless networking
## Headless setup

mount /boot partition on SD card

create file: wpa_supplicant.conf

    ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
    update_config=1
    country=<Insert 2 letter ISO 3166-1 country code here>
    
    network={
        ssid="<Name of your wireless LAN>"
        psk="<Password for your wireless LAN>"
    }

## Ad-Hoc
1. /etc/networking/interfaces
2. wpa_supplicant.conf

### /etc/networking/interfaces
- con: wrong configuration makes device unreachable over ssh
- pro: creates standalone 'dtn-video' network

### wpa_supplicant.conf
- con: few documentation
- pro: can be achieved by editing data on /boot partition (see: headless setup)