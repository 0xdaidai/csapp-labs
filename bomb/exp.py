from pwn import *
context(aslr = False, log_level = 'debug')

p = process('./bomb')

gdb.attach(p)
p.recv()
p.sendline("Border relations with Canada have never been better.")
p.recv()
p.sendline("1 2 4 8 16 32")
p.recv()
p.sendline("1 311")
p.recv()
p.sendline("7 0 DrEvil")
p.recv()
p.sendline("ionefg")
p.recv()
p.sendline("4 3 2 1 6 5")
p.recv()
p.sendline("22")

p.interactive()