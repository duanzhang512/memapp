# memapp

This tool can be used to read/write registers. Below is an example, used in OpenBMC. 

```
root@bmc-oob:~# memapp 
**********************************************
*              MemApp TOOL V0.1               *
**********************************************
Usage: memapp [OPTIONS...]
OPTIONS:
	-r|w      Mode of operation read/write
	-a [hex]  Address to read/write
	-d [hex]  Data to write
	-l [dec]  length of data to read
	-b [dec]  Bit of data
	-e        Big endian
	-h|?      This help text
	-v        Print version
	-E        Show example
root@bmc-oob:~# memapp -r -a 0x1e6e2500
REG(0x1e6e2500): 0x000010c0
root@bmc-oob:~# memapp -r -a 0x1e6e2500 -b 25
REG(0x1e6e2500): 0x000010c0	[25]=0
root@bmc-oob:~# memapp -r -a 0x1e6e2500 -b 4
REG(0x1e6e2500): 0x000010c0	[4]=0
root@bmc-oob:~# memapp -r -a 0x1e6e2000 -l 32 
1e6e2000: 0000 0001 0500 0303 0000 0000 0000 0000    ................
1e6e2010: 1688 a8a8 0500 0303 0000 0000 0000 0000    ................
root@bmc-oob:~# memapp -r -a 0x1e6e2000 -l 32 -e
1e6e2000: 0100 0000 0303 0005 0000 0000 0000 0000    ................
1e6e2010: a8a8 8816 0303 0005 0000 0000 0000 0000    ................
```
