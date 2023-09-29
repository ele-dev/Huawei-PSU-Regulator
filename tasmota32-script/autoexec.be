import json
import math

# configure power hysteresis here
hysteresis = 4
lastPower = 0
lastCmdTime = 0
u = udp()

def notifyRegulator()
# fetch latest power value and tick the timer variable
var sensors = json.load(tasmota.read_sensors())
var power = sensors['SML']['Power_curr']
lastCmdTime += 1

# detect relevant power state changes (hysteresis)
if math.abs(power - lastPower) < hysteresis && lastCmdTime < 50
return
end

# send power change event to the regulator via UDP and reset timer variable
u.send("192.168.XXX.XXX", 2000, bytes().fromstring(str(power)))
print("sent power change event: " + str(power))
lastPower = power
lastCmdTime = 0
end

# register cron task to run the notify function every second
tasmota.add_cron("*/1 * * * * *", notifyRegulator, "every_1_s")