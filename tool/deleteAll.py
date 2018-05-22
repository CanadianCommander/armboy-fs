import time

serial = open("/dev/cu.usbmodem14121","w+")
print(serial.readline())

time.sleep(1);

serial.write("delete all \n")
serial.flush()

print(serial.readline())
serial.close()
