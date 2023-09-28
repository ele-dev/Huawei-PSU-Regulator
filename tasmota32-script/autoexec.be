import json
import math

u = udp()
lastPower = 0
lastCmdTime = 0
hysteresis = 4

def f()
# fetch latest power value and tick the timer variable
var sensors = json.load(tasmota.read_sensors())
var power = sensors['SML']['Power_curr']
lastCmdTime += 1

# detect relevant state change (hysteresis)
if math.abs(power - lastPower) < hysteresis && lastCmdTime < 50
return
end

# send power change event and reset timer variable
u.send("192.168.XXX.XXX", 2000, bytes().fromstring(str(power)))
print("sent power change event: " + str(power))
lastPower = power
lastCmdTime = 0
end

tasmota.add_cron("*/1 * * * * *", f, "every_1_s")