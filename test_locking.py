import struct, fcntl, os, time

LOCKFILE = '/var/lock/irrp'

f = open(LOCKFILE, "w")

print("acquiring lock!")


fcntl.lockf(f, fcntl.LOCK_EX)

print("lock acquired!")


print("done!")
time.sleep(5)