/*************************************************************************
 *************************************************************************
 ***                                                                    **
 ***               (c)Copyright 2019 Celestica Inc.                     **
 ***                                                                    **
 ***                      All Rights Reserved.                          **
 ***                                                                    **
 ***      Section A701, Building 1, Zhang Jiang River Front Harbor      **
 ***                                                                    **
 ***           3000 Long dong Ave. Shanghai. 201203, P.R.C              **
 ***                                                                    **
 *************************************************************************
 *************************************************************************
 *************************************************************************
 *
 * memapp.c
 * memory tool to read / write register
 *
 * Author: Fred Shao <fazhishao@celestica.com>
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#define TOOL_VERSION		"1.2"
#define MAP_SIZE			4096UL
#define MAP_MASK 			(MAP_SIZE - 1)
#define DISP_LINE_LEN		16

typedef enum 
{
	READ_MMAP,
	WRITE_MMAP,
	END_OF_FUNCLIST
}mmap_cmds;

mmap_cmds cmd = END_OF_FUNCLIST;

void ShowHelp(char *str)
{
	printf("**********************************************\n");
	fprintf(stdout, "*              MemApp TOOL V%s               *\n", TOOL_VERSION);
	printf("**********************************************\n");
	printf("Usage: %s [OPTIONS...]\n", str);
	printf("OPTIONS:\n");
	printf("\t-r|w      Mode of operation read/write\n");
	printf("\t-a [hex]  Address to read/write\n");
	printf("\t-d [hex]  Data to write\n");
	printf("\t-l [dec]  length of data to read\n");
	printf("\t-b [dec]  Bit of data\n");
	printf("\t-e        Big endian\n");
	printf("\t-h|?      This help text\n");
	printf("\t-v        Print version\n");
    printf("\t-E        Show example\n");
}

void ShowVersion(void)
{
	fprintf(stdout, "version %s\n", TOOL_VERSION);
}

void ShowExample(void)
{
    printf(" Example:\n");
    
    printf("~ # memapp -r -a 1e78502c\n");
    printf("REG(0x1e78502c): 0x00000092\n");

    printf("~ # memapp -r -a 1e78502c -b 0\n");
    printf("REG(0x1e78502c): 0x00000092	[0]=0\n");

    printf("~ # memapp -w -a 1e78502c -b 0 -d 1\n");
    printf("REG(0x1e78502c): 0x00000093	[0]=1\n");

    printf("~ # memapp -r -a 20000000 -l 16\n");
    printf("20000000: ea00 0014 e59f f014 e59f f014 e59f f014    ................\n");    
}

uint32_t GetAlignedAddress(uint32_t reg, uint32_t align)
{
	return (uint32_t)(((reg + align) & ~(align - 1)) - align);
}

uint32_t EndianConvert(uint32_t val)
{
	return (uint32_t)(((val & 0xff) << 24) + (((val >> 8) & 0xff) << 16) + (((val >> 16) & 0xff) << 8) + ((val >> 24) & 0xff));
}

uint8_t ReadBitOfValue(uint32_t val, uint8_t bit)
{
	return (uint8_t)((val & (1 << bit)) >> bit);
}

uint32_t WriteBitOfValue(uint32_t val, uint8_t bit, uint8_t num)
{
	if(num)		
		return (uint32_t)(val | (1 << bit));
	else 
		return (uint32_t)(val & ~(1 << bit));		
}

uint32_t StringToNumber(char *str, uint8_t system)
{
	return (uint32_t)strtol(str, NULL, system);
}

static void *RegisterMap(uint32_t addr, int *fop)
{
	int fd;
	void *base, *virt_addr;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	
	if(fd < 0)
	{
		printf("open /dev/mem error!\n");
		return (void *)-1;
	}
	
	base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr & ~MAP_MASK);
	
	if(base == (void *)-1)
	{
		printf("map base is NULL!\n");
		return (void *)-1;
	}
	
	virt_addr = base + (addr & MAP_MASK);
	*fop = fd;

	return virt_addr;
}

static void RegisterUnmap(void *addr, int fd)
{
	munmap(addr, MAP_SIZE);
	close(fd);
}

uint32_t ReadRegister(uint32_t reg)
{
	void *scu_addr;
	int scu_fd;
	uint32_t val;
	
	scu_addr = RegisterMap(reg, &scu_fd);
	val = *(volatile uint32_t *)scu_addr;

	RegisterUnmap(scu_addr, scu_fd);
	return (uint32_t)val;
}

void WriteRegister(uint32_t reg, uint32_t val)
{
	void *scu_addr;
	int scu_fd;

	scu_addr = RegisterMap(reg, &scu_fd);
	*(volatile uint32_t *)scu_addr = val;

	RegisterUnmap(scu_addr, scu_fd);
}

void PrintLine(uint32_t reg, uint32_t length, uint8_t big_endian)
{
	int i, nbytes, linebytes, line_align;
	uint8_t linebuf[DISP_LINE_LEN];
	uint8_t *cp;
	uint32_t val, line = 0;

	nbytes = length * 4;
	
	do
	{
		linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;
		for(i = 0;i < linebytes / 4;i++)
		{
			val = ReadRegister(GetAlignedAddress(reg + i * 4, 4));
			if(big_endian)
			{
				linebuf[i*4]   = val & 0xff;
				linebuf[i*4+1] = (val>>8) & 0xff;
				linebuf[i*4+2] = (val>>16) & 0xff;
				linebuf[i*4+3] = (val>>24) & 0xff;
			}
			else
			{
				linebuf[i*4]   = (val>>24) & 0xff;
				linebuf[i*4+1] = (val>>16) & 0xff;
				linebuf[i*4+2] = (val>>8) & 0xff;
				linebuf[i*4+3] = val & 0xff;
			}
		}
		
		printf("%08x:", GetAlignedAddress(reg, 4));
		cp = linebuf;
		for (i=0; i<linebytes; i++) 
		{
			if(0 == i % 2)
				printf(" %02x", *cp++);
			else
				printf("%02x", *cp++);
			reg++;
		}
		
		line_align = 4 - ((linebytes / 4) % 4);
		if(4 == line_align)
			line_align = 0;
		for(i=0;i<line_align;i++)
			printf("          ");
		printf("    ");
		
		cp = linebuf;
		for (i=0; i<linebytes; i++) 
		{
			if ((*cp < 0x20) || (*cp > 0x7e))
				putchar('.');
			else
				printf("%c", *cp);
			cp++;
		}
		putchar('\n');

		line++;
		nbytes -= linebytes;
	}while(nbytes > 0);

}

static const char *optString = "a:d:l:b:rwevE";

int main(int argc, char **argv)
{	
	char ch = 0, opt_end = -1;
	uint8_t bit = 0;
	uint8_t bit_mode = 0, list_mode = 0, big_endian_mode = 0; 
	uint32_t addr = 0, data = 0, val = 0, len = 1;
	
	opterr = 0;
	
	if(argc == 1)
	{
		ShowHelp(argv[0]);
		exit(EXIT_SUCCESS);
	}
	
	while((ch = getopt(argc, argv, optString)) != opt_end)
	{
		switch(ch) 
		{
			case 'a':
				addr = StringToNumber(optarg, 16);
			break;
			case 'd':
				data = StringToNumber(optarg, 16);
			break;			
			case 'l':
				len = StringToNumber(optarg, 10);
				list_mode = 1;
			break;
			case 'b':
				bit = StringToNumber(optarg, 10);
				bit_mode = 1;
			break;
			case 'r':
				cmd = READ_MMAP;
			break;			
			case 'w':
				cmd = WRITE_MMAP;
			break;			
			case 'e':				
				big_endian_mode = 1;
			break;
			case 'v':
				ShowVersion();
				exit(EXIT_SUCCESS);
			break;
            case 'E':
                ShowExample();
            break;
			case 'h':
			case '?':
			default :
				ShowHelp(argv[0]);
				exit(EXIT_SUCCESS);
			break;
		}
	}
		
	if(cmd == READ_MMAP)
	{
		if(list_mode)
		{
			PrintLine(addr, (len - 1) / 4 + 1, big_endian_mode);
		}
		else
		{
			val = ReadRegister(GetAlignedAddress(addr, 4));
			if(bit_mode)
			{
				if(bit < 32)
					printf("REG(0x%08x): 0x%08x\t[%d]=%d\n", GetAlignedAddress(addr, 4), val, bit, ReadBitOfValue(val, bit));
				else
					printf("REG(0x%08x): 0x%08x\n", GetAlignedAddress(addr, 4), val);				
			}			
			else
			{
				printf("REG(0x%08x): 0x%08x\n", GetAlignedAddress(addr, 4), val);
			}		
		}
	}
	else if(cmd == WRITE_MMAP)
	{
		if(bit_mode)
		{
			val = ReadRegister(GetAlignedAddress(addr, 4));
			WriteRegister(GetAlignedAddress(addr, 4), WriteBitOfValue(val, bit, data));

			val = ReadRegister(GetAlignedAddress(addr, 4));
			if(bit < 32)
				printf("REG(0x%08x): 0x%08x\t[%d]=%d\n", GetAlignedAddress(addr, 4), val, bit, ReadBitOfValue(val, bit));
			else
				printf("REG(0x%08x): 0x%08x\n", GetAlignedAddress(addr, 4), val);							
		}
		else
		{
			WriteRegister(GetAlignedAddress(addr, 4), data);
			
			val = ReadRegister(GetAlignedAddress(addr, 4));			
			printf(" REG(0x%08x): 0x%08x\n", GetAlignedAddress(addr, 4), val);
		}
	}
	
	return EXIT_SUCCESS;
}
