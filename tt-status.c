/*
 * Copyright (c) 2014,2017 Eric Sandeen <sandeen@sandeen.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * tt-status: Show status of a Triangle Tube Solo Prestige boiler via ModBus
 *
 * Usually pointed at a RS-485 serial port device, but may also query through
 * a ModBus/TCP gateway such as mbusd (http://http://mbus.sourceforge.net/)
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <modbus/modbus.h>

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

void usage(void)
{
	printf("Usage: tt-status [-hkd] [-S slave] [-s serial port][-i ip addr [-p port]]\n\n");
	printf("-h\tShow this help\n");
	printf("-k\tQuery Lochinvar boiler\n");
	printf("-d\tEnable debug\n");
	printf("-S\tModbus slave ID, default 1\n");
	printf("-s\tSerial Port Device for ModBus/RTU\n");
	printf("-i\tIP Address for ModBus/TCP\n");
	printf("-p\tTCP Port for ModBus/TCP (optional, default 502)\n");
	exit(1);
}

int c_to_f(float c)
{
	return ((c * 9)/5 + 32);
}

struct status_bits {
	int	bit;
	char	*desc;
};

struct status_bits tt_status[] = {
	{ 0, "PC Manual Mode" },
	{ 1, "DHW Mode" },
	{ 2, "CH Mode" },
	{ 3, "Freeze Protection Mode" },
	{ 4, "Flame Present" },
	{ 5, "CH(1) Pump" },
	{ 6, "DHW Pump" },
	{ 7, "System / CH2 Pump" }
};


int query_triangle_tube(modbus_t *mb)
{
	int		i;
	uint16_t	regs[8];	/* Holds results of register reads */

	/* Read 1 register from the address 0 for status bitfield */
	if (modbus_read_input_registers(mb, 0, 1, regs) != 1) {
		printf("Error: Modbus read of 1 byte at addr 0 failed\n");
		return 1;
	}

	printf("Status:\n");
	if (regs[0] == 0)
		printf(" Standby\n");

	for (i = 0; i < 7; i++) {
		if (CHECK_BIT(regs[0], i))
			printf(" %s\n", tt_status[i].desc);
	}

	/* Read 9 registers from the address 0x300 */
	if (modbus_read_input_registers(mb, 0x300, 9, regs) != 9) {
		printf("Error: Modbus read of 9 bytes at addr 0x300 failed\n");
		return 1;
	}

	/* Supply temp: 0.1 degree C, 16 bits */
	printf("Supply temp:\t\t%3d °F\n", c_to_f(regs[0]/10));

	/* Return temp: degrees C, 8 bits */
	printf("Return temp:\t\t%3d °F\n", c_to_f(regs[1]));

	/* DHW storage temp */
	printf("DHW Storage temp:\t%3d °F\n", c_to_f(regs[2]));

	/* Flue temp: degrees C, 8 bits */
	printf("Flue temp:\t\t%3d °F\n", c_to_f(regs[3]));

	/* Outdoor temp: degrees C, 8 bits */
	printf("Outdoor temp:\t\t%3d °F\n", c_to_f((int16_t)regs[4]));

	/* Future use */

	/* Flame Ionization: μA, 8 bits */
	printf("Flame Ionization:\t%3d μA\n", regs[6]);

	/* Firing rate: % 8 bits */
	printf("Firing rate:\t\t%3d %\n", regs[7]);

	/* Boiler setpoint: degrees C, 8 bits, only if firing */
	if (regs[8] != 0x8000)
		printf("Boiler Setpoint:\t\t%3d °F\n", c_to_f(regs[8]));

	/* Read 3 registers from the address 0x200*/
	/* modbus_read_registers(mb, 0x200, 3, regs); */

	/* Read 2 registers from the address 0x500 */
	if (modbus_read_registers(mb, 0x500, 2, regs) != 2) {
		printf("Error: Modbus read of 2 bytes at addr 0x500 failed\n");
		return 1;
	}

	/* CH1 Maximum Setpoint C, 8 bits */
	printf("CH1 Maximum Setpoint:\t%3d °F\n", c_to_f(regs[0]));

	/* DHW setpoint: degrees C, 8 bits, only if set */
	if (regs[1] != 0x8000)
		printf("DHW Setpoint:\t\t%3d °F\n", c_to_f(regs[1]));

	return 0;
}

int query_lochinvar(modbus_t *mb)
{
	int		i;
	uint16_t	regs_30000[17];	/* Holds results of register reads */
	uint16_t	regs_40000[9];	/* Holds results of register reads */

	/*
	 * The Lochinvar modbus map has register regions starting at addresses
	 * 30001 and 40001.  It's convenient in the code to use i.e.
	 * regs_30000[1] to be the value read from address 30001, etc, so
	 * we play a couple games here to get that lined up, because
	 * i.e. address 30000 is not valid to read.  First, set the initial
	 * element to 0 because we won't read anything from there.
	 */
	regs_30000[0] = regs_40000[0] = 0;

	/* Read 16 registers from the address 30001 */
	if (modbus_read_input_registers(mb, 30001, 16, &regs_30000[1]) != 16) {
		printf("Error: Modbus read of 16 bytes at addr 30001 failed\n");
		return 1;
	}

	/* Read 8 registers from the address 40001 */
	if (modbus_read_registers(mb, 40001, 8, &regs_40000[1]) != 8) {
		printf("Error: Modbus read of 8 bytes at addr 40001 failed\n");
		return 1;
	}

# if 0
	/* Not sure about Lochinvar status yet */
	printf("Status:\n");
	if (regs[0] == 0)
		printf(" Standby\n");

	for (i = 0; i < 7; i++) {
		if (CHECK_BIT(regs[0], i))
			printf(" %s\n", status[i].desc);
	}
#else
	printf("Status: 0x%x\n", regs_30000[14]);
#endif

	/* Supply temp: 0.1 degree C, 16 bits */
	printf("Supply temp:\t\t%3d °F\n", c_to_f(regs_30000[9]/10));

	/* Return temp: degrees C, 8 bits */
	printf("Return temp:\t\t%3d °F\n", c_to_f(regs_30000[10]/10));

	/* DHW storage temp */
	printf("DHW Storage temp:\t%3d °F\n", c_to_f(regs_40000[5]/10));

	/* Flue temp: degrees C, 8 bits */
	printf("Flue temp:\t\t%3d °F\n", c_to_f(regs_30000[11]/10));

	/* Outdoor temp: degrees C, 8 bits */
	printf("Outdoor temp:\t\t%3d °F\n", c_to_f(regs_40000[6]/10));

	/* Firing rate: % 8 bits */
	printf("Firing rate:\t\t%3d %\n", regs_30000[12]);

	/* Boiler setpoints, not sure the difference  */
	printf("System Setpoint:\t\t%3d °F\n", c_to_f(regs_30000[4]/2));
	printf("Outlet Setpoint:\t\t%3d °F\n", c_to_f(regs_30000[8]/2));

	/* DHW setpoint: degrees C */
	printf("DHW Setpoint:\t\t%3d °F\n", c_to_f(regs_40000[4]/2));

	return 0;
}

int main(int argc, char **argv)
{
	int c;
	int err = 0;
	int slave = 1;		/* default Modbus slave ID */
	int debug = 0;		/* debug output */
	int port = 502;		/* default ModBus/TCP port */
	char ipaddr[16] = "";	/* ModBus/TCP ip address */
	char serport[32] = "";	/* ModBus/RTU serial port */
	bool lochinvar = false;	/* Query lochinvar not triangle tube */
	modbus_t *mb;		/* ModBus context */

	while ((c = getopt(argc, argv, "hldS:s:i:p:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			break;
		case 'l':
			lochinvar = true;
			break;
		case 'd':
			debug++;
			break;
		case 'S':
			slave = atoi(optarg);
			break;
		case 's':
			strncpy(serport, optarg, sizeof(serport));
			serport[31] = '\0';
			break;
		case 'i':
			strncpy(ipaddr, optarg, sizeof(ipaddr));
			ipaddr[15] = '\0';
			break;
		case 'p':
			port = atoi(optarg);
			break;
		default:
			usage();
		}
	}

	if (!ipaddr[0] && !serport[0]) {
		printf("Error: Must specify either ip addresss or serial port\n\n");
		usage();
	}
	if (ipaddr[0] && serport[0]) {
		printf("Error: Must specify only one of ip addresss or serial port\n\n");
		usage();
	}

	if (ipaddr[0])
		mb = modbus_new_tcp(ipaddr, port);
	else if (lochinvar)
		mb = modbus_new_rtu(serport, 9600, 'N', 8, 2);
	else
		mb = modbus_new_rtu(serport, 38400, 'N', 8, 1);

	if (!mb) {
		perror("Error: modbus_new failed");
		goto out;
	}

	if (debug)
		modbus_set_debug(mb, TRUE);

	if (modbus_set_slave(mb, slave)) {
		perror("Error: modbus_set_slave failed");
		goto out;
	}

	if (modbus_connect(mb)) {
		perror("Error: modbus_connect failed");
		goto out;
	}

	if (lochinvar)
		err = query_lochinvar(mb);
	else
		err = query_triangle_tube(mb);

out:
	modbus_close(mb);
	modbus_free(mb);
	return err;
}
