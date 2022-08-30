//------------------------------------------------------------------------------
/**
 * @file i2c_test.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief server main control.
 * @version 0.1
 * @date 2022-08-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */
//------------------------------------------------------------------------------
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

//------------------------------------------------------------------------------
#define DUPLEX_HALF 0x00
#define DUPLEX_FULL 0x01

#define ETHTOOL_GSET 0x00000001 /* Get settings command for ethtool */

struct ethtool_cmd {
	unsigned int cmd;
	unsigned int supported; /* Features this interface supports */
	unsigned int advertising; /* Features this interface advertises */
	unsigned short speed; /* The forced speed, 10Mb, 100Mb, gigabit */
	unsigned char duplex; /* Duplex, half or full */
	unsigned char port; /* Which connector port */
	unsigned char phy_address;
	unsigned char transceiver; /* Which tranceiver to use */
	unsigned char autoneg; /* Enable or disable autonegotiation */
	unsigned int maxtxpkt; /* Tx pkts before generating tx int */
	unsigned int maxrxpkt; /* Rx pkts before generating rx int */
	unsigned int reserved[4];
};

//------------------------------------------------------------------------------
// for my lib
//------------------------------------------------------------------------------
/* 많이 사용되는 define 정의 모음 */
#include "typedefs.h"

/* framebuffer를 control하는 함수 */
#include "lib_fb.h"

/* file parser control 함수 */
#include "lib_ui.h"

#include "i2c_test.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
__s32 i2c_smbus_access(int file, char read_write, __u8 command,
		       int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;
	__s32 err;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;

	err = ioctl(file, I2C_SMBUS, &args);
	if (err == -1)
		err = -errno;
	return err;
}

//------------------------------------------------------------------------------
__s32 get_net_info (__s8 *eth_name, __u8 *my_ip, __s32 *speed, __u8 *mac)
{
	int fd;
	struct ifreq ifr;
	struct ethtool_cmd ecmd;

	/* this entire function is almost copied from ethtool source code */
	/* Open control socket. */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		info("Cannot get control socket");
		return 0;
	}
	strncpy(ifr.ifr_name, eth_name, IFNAMSIZ);
	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
		info("%s SIOCGIFADDR ioctl Error!!\n", eth_name);
		goto out;
	} else {
		inet_ntop(AF_INET, ifr.ifr_addr.sa_data+2, my_ip, sizeof(struct sockaddr));
		info("%s myOwn IP Address is %s\n", eth_name, my_ip);
	}

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
		info("SIOCGIFHWADDR ioctl Error!!\n");
		goto out;
	} else {
		memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
		info("find device (%s), mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
			ifr.ifr_name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	/* Pass the "get info" command to eth tool driver */
	ecmd.cmd = ETHTOOL_GSET;
	ifr.ifr_data = (caddr_t)&ecmd;

	/* ioctl failed: */
	if (ioctl(fd, SIOCETHTOOL, &ifr))
	{
		info("Cannot get device settings\n");
		goto out;
	}

	info("LinkSpeed = %d MB/s\n", ecmd.speed);
	*speed = ecmd.speed;

	close(fd);
	return 1;
out:
	close(fd);
	return 0;
}

//------------------------------------------------------------------------------
__s32 mac_range_check (app_data_t *app_data, __u8 *mac)
{
	__u8 mac_start[8], mac_end[8], mac_addr[8];

	if (!strncmp(app_data->mac_test, "enable", sizeof("enable")))
		info ("mac address range check enable!\n");
	else
		return 0;

	if (mac[0] != 0x0 || mac[1] != 0x1e || mac[2] != 0x06)
		return 1;

	memset (mac_start, 0x00, sizeof(mac_start));
	memset (mac_end,   0x00, sizeof(mac_end));
	memset (mac_addr,  0x00, sizeof(mac_addr));

	sprintf(mac_start, "%c%c%c%c%c%c",
		app_data->mac_range[0][9],	app_data->mac_range[0][10],
		app_data->mac_range[0][12],	app_data->mac_range[0][13],
		app_data->mac_range[0][15],	app_data->mac_range[0][16]
	);

	sprintf(mac_end, "%c%c%c%c%c%c",
		app_data->mac_range[1][9],	app_data->mac_range[1][10],
		app_data->mac_range[1][12],	app_data->mac_range[1][13],
		app_data->mac_range[1][15],	app_data->mac_range[1][16]
	);

	sprintf(mac_addr, "%02x%02x%02x", mac[3],mac[4],mac[5]);

	{
		__u32 start, end, cur;

		start = strtoul(mac_start, NULL, 10);
		end   = strtoul(mac_end, NULL, 10);
		cur   = strtoul(mac_addr, NULL, 10);

		info ("s : %d, e : %d, c : %d\n", start, end, cur);
		if ((start < cur) && (cur < end))
			return 0;
	}
	return 1;
}

//------------------------------------------------------------------------------
//__s32 get_net_info (char *eth_name, char *my_ip, int *speed, char *mac)
void app_test_net (app_data_t *app_data)
{
	__u8	ip[16], mac[6];
	__s32	speed, mac_start, mac_end;

	memset (ip, 0, sizeof(ip));	memset (mac, 0, sizeof(mac));	speed = 0;
	get_net_info (app_data->eth_name[0], ip, &speed, mac);
//	ui_set_ritem(app_data->pfb, app_data->pui, 6, COLOR_GREEN, -1);
	ui_set_str (app_data->pfb, app_data->pui, 6, -1, -1,
				3, -1, "%s(%s), %d MB/s",
				app_data->eth_name[0], ip, speed);

	// MAC0 Address Check
	ui_set_str (app_data->pfb, app_data->pui, 8, -1, -1,
				3, -1, "MAC(%s) : %02x:%02x:%02x:%02x:%02x:%02x",
				app_data->eth_name[0], mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	if (mac_range_check(app_data, ip))
		ui_set_ritem(app_data->pfb, app_data->pui, 8, COLOR_RED, -1);
	else
		ui_set_ritem(app_data->pfb, app_data->pui, 8, COLOR_GREEN, -1);

	memset (ip, 0, sizeof(ip));	memset (mac, 0, sizeof(mac));	speed = 0;
	get_net_info (app_data->eth_name[1], ip, &speed, mac);
//	ui_set_ritem(app_data->pfb, app_data->pui, 7, COLOR_GREEN, -1);
	ui_set_str (app_data->pfb, app_data->pui, 7, -1, -1,
				3, -1, "%s(%s), %d MB/s",
				app_data->eth_name[1], ip, speed);

	// MAC1 Address Check
	ui_set_str (app_data->pfb, app_data->pui, 9, -1, -1,
				3, -1, "MAC(%s) : %02x:%02x:%02x:%02x:%02x:%02x",
				app_data->eth_name[1], mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	if (mac_range_check(app_data, ip))
		ui_set_ritem(app_data->pfb, app_data->pui, 9, COLOR_RED, -1);
	else
		ui_set_ritem(app_data->pfb, app_data->pui, 9, COLOR_GREEN, -1);
}

//------------------------------------------------------------------------------
void app_test_i2c (app_data_t *app_data)
{
	int fd, i;

	for (i = 0; i < 2; i++) {
		ui_set_str (app_data->pfb, app_data->pui, i + 2, -1, -1,
					3, -1, "Found I2C Node(%s)", app_data->i2c_node_name[i]);
		if (access (app_data->i2c_node_name[i], F_OK) != 0) {
			ui_set_ritem(app_data->pfb, app_data->pui, i + 2, COLOR_RED, -1);
		}
		else {
			ui_set_ritem(app_data->pfb, app_data->pui, i + 2, COLOR_GREEN, -1);
			ui_set_str (app_data->pfb, app_data->pui, i + 4, -1, -1,
						3, -1, "Check %s Device (Addr = 0x%02x)",
						app_data->i2c_node_name[i],
						app_data->i2c_test_addr[i]);

			if ((fd = open(app_data->i2c_node_name[i], O_RDWR)) < 0) {
				ui_set_ritem(app_data->pfb, app_data->pui, i + 4, COLOR_RED, -1);
			} else {
				// set the I2C slave address for all subsequent I2C device transfers
				if (ioctl(fd, I2C_SLAVE, app_data->i2c_test_addr[i]) < 0) {
					err("Error failed to set I2C address [0x%02x].\n",
						app_data->i2c_test_addr[i]);
					ui_set_ritem(app_data->pfb, app_data->pui, i + 4, COLOR_RED, -1);
				} else {
					union i2c_smbus_data data;
					int err;

					err = i2c_smbus_access(fd, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
					if (err)
						ui_set_ritem(app_data->pfb, app_data->pui, i + 4, COLOR_RED, -1);
					else
						ui_set_ritem(app_data->pfb, app_data->pui, i + 4, COLOR_GREEN, -1);
				}
				close(fd);
			}
		}
	}
}

//------------------------------------------------------------------------------
void app_info_display (app_data_t *app_data)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

    ui_set_str (app_data->pfb, app_data->pui, 0, -1, -1,
                4, -1, "ODROID-H3 I2C Test App");
    ui_set_str (app_data->pfb, app_data->pui, 1, -1, -1,
                3, -1, "%d/%d/%d, %02d:%02d:%02d",
				tm.tm_year + 1900, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	app_test_i2c (app_data);
	app_test_net (app_data);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int app_main (app_data_t *app_data)
{
	while (1) {
		app_info_display(app_data);
		sleep(1);
	}
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
