# tt-status
A small test program using libmodbus to show status of a Triangle Tube boiler

## Help

    $ tt-status -h
     Usage: tt-status [-h] [-d] [-s serial port][-i ip addr [-p port]]
    
    -h      Show this help
    -d      Enable debug
    -s      Serial Port Device for ModBus/RTU
    -i      IP Address for ModBus/TCP
    -p      TCP Port for ModBus/TCP (optional, default 502)

## Example
    $ tt-status -i 10.0.0.122
    Status:
     Standby
    Supply temp:            105 °F
    Return temp:            100 °F
    DHW Storage temp:       118 °F
    Flue temp:               87 °F
    Outdoor temp:            41 °F
    Flame Ionization:         0 μA
    Firing rate:              0 %
    CH1 Maximum Setpoint:   141 °F
    DHW Setpoint:           123 °F

##See also:
* http://sandeen.net/modbus-boiler/
* http://sandeen.net/wordpress/energy/residential-boiler-monitoring-via-modbus/

