/*
 *  Copyright (C) 2007 Austin Robot Technology, Patrick Beeson
 *  Copyright (C) 2009, 2010 Austin Robot Technology, Jack O'Quin
 *  Copyright (C) 2015, Jack O'Quin
 *	Copyright (C) 2017, Robosense, Tony Zhang
 *
 *
 *  License: Modified BSD Software License Agreement
 *
 *  $Id$
 */

/** \file
 *
 *  Input classes for the RSLIDAR RS-16 3D LIDAR:
 *
 *     Input -- base class used to access the data independently of
 *              its source
 *
 *     InputSocket -- derived class reads live data from the device
 *              via a UDP socket
 *
 *     InputPCAP -- derived class provides a similar interface from a
 *              PCAP dump
 */

#ifndef __RSLIDAR_INPUT_H_
#define __RSLIDAR_INPUT_H_

#include <unistd.h>
#include <stdio.h>
#include <pcap.h>
#include <netinet/in.h>
#include <string.h>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <carmen/carmen.h>

namespace rslidar_driver
{
const int RSLIDAR_MSG_BUFFER_SIZE = 1248;

typedef struct rslidar_param
{
	std::string device_ip;
}  __attribute__((packed)) rslidar_param_t ;

typedef struct rslidarPacket
{
	double stamp;
	uint8_t data[RSLIDAR_MSG_BUFFER_SIZE];
}  __attribute__((packed)) rslidarPacket_t ;

static uint16_t MSOP_DATA_PORT_NUMBER = 6699;   // rslidar default data port on PC
static uint16_t DIFOP_DATA_PORT_NUMBER = 7788;  // rslidar default difop data port on PC
/**
 *  从在线的网络数据或离线的网络抓包数据（pcap文件）中提取出lidar的原始数据，即packet数据包
 * @brief The Input class,
 *
 * @param private_nh  一个NodeHandled,用于通过节点传递参数
 * @param port
 * @returns 0 if successful,
 *          -1 if end of file
 *          >0 if incomplete packet (is this possible?)
 */
class Input
{
public:
	Input(rslidar_param private_nh, uint16_t port);

	virtual ~Input(){}

	virtual int getPacket(rslidarPacket_t* pkt, const double time_offset) = 0;

	int getRpm(void);
	int getReturnMode(void);
	bool getUpdateFlag(void);
	void clearUpdateFlag(void);

protected:
	rslidar_param private_nh_;
	uint16_t port_;
	std::string devip_str_;
	int cur_rpm_;
	int return_mode_;
	bool npkt_update_flag_;
};

/** @brief Live rslidar input from socket. */
class InputSocket : public Input
{
public:
	InputSocket(rslidar_param private_nh, uint16_t port = MSOP_DATA_PORT_NUMBER);

	virtual ~InputSocket();

	virtual int getPacket(rslidarPacket_t* pkt, const double time_offset);

private:
private:
	int sockfd_;
	in_addr devip_;

};
//
///** @brief rslidar input from PCAP dump file.
// *
// * Dump files can be grabbed by libpcap
// */
//class InputPCAP : public Input
//{
//public:
//	InputPCAP(rslidar_param private_nh, uint16_t port = MSOP_DATA_PORT_NUMBER, double packet_rate = 0.0,
//			std::string filename = "", bool read_once = false, bool read_fast = false, double repeat_delay = 0.0);
//
//	virtual ~InputPCAP();
//
//	virtual int getPacket(rslidarPacket_t* pkt, const double time_offset);
//
//private:
//	rslidar_param private_nh_;
//	double packet_rate_;
//	std::string filename_;
//	pcap_t* pcap_;
//	bpf_program pcap_packet_filter_;
//	char errbuf_[PCAP_ERRBUF_SIZE];
//	bool empty_;
//	bool read_once_;
//	bool read_fast_;
//	double repeat_delay_;
//};
}

#endif  // __RSLIDAR_INPUT_H
