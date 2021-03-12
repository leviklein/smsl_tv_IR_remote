import struct, fcntl, os, time

f = open("testlockfile", "w")

print("acquiring lock!")


fcntl.lockf(f, fcntl.LOCK_EX)

print("lock acquired!")


print("done!")
time.sleep(5)