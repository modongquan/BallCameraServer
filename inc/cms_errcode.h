/*
* @SDKS Tech, All rights reserved.
* file: cms_errcode.h
* description:
*		define system error code
* version:	1.0
* Author:	Steven
* Date:		2017-07-17
*/

#ifndef _CMS_ERRCODE_H_
#define _CMS_ERRCODE_H_

//
// Define
//
//   7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//  +-------+-------+----------------+---------------+--------------+
//  |  E    |         Reserved       |  Module Domain|     Code     |
//  +------ +-------+----------------+---------------+--------------+
//
//  where
//
//      E - is the Error bit
//
//          0 - Success
//          E - Error
//
//      Module Domain - is the Error module domain, 8 bit
//		Code - error code in a certain module, 8 bits


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// Error bit
//
#define ERR_E					0xE0000000	// 兼容Windows用户自定义的错误码区域

//
// error module domain
//
#define ERR_NETWORK_DOMAIN		(ERR_E | 0x0100)
#define ERR_USER_MNG_DOMAIN		(ERR_E | 0x0200)
#define ERR_DEVICE_MNG_DOMAIN	(ERR_E | 0x0300)
#define ERR_DATABASE_DOMAIN		(ERR_E | 0x0400)
#define ERR_BASE_DOMAIN			(ERR_E | 0x0500)
#define ERR_CLIENT_DOMAIN		(ERR_E | 0x0600)
#define ERR_DEVICESDK_DOMAIN	(ERR_E | 0x0700)

namespace sds {
	namespace error {

//
// Error Code 
//
typedef const unsigned long CDWORD;


CDWORD ERR_SUCCESS = 0;
CDWORD ERR_PART_SUCCESS = 0x000000001;	// 多个操作部分成功

//====================================================================
// 1 Network communication domain error
//====================================================================
//packet error
CDWORD ERR_COMM_PACKET = ERR_NETWORK_DOMAIN + 1;
//unknown packet head
CDWORD ERR_UNKNOWN_PACKET = ERR_NETWORK_DOMAIN + 2;
// pcpk code not matched
CDWORD ERR_PCPK_NOT_MATCHED = ERR_NETWORK_DOMAIN + 3;

CDWORD ERR_NETWORK_COMMUNICATION_FAULT = ERR_NETWORK_DOMAIN + 4;
//
CDWORD ERR_RECV_PACKET_UNMATCHED = ERR_NETWORK_DOMAIN + 5;
//
CDWORD ERR_PACKET_LENGTH_TOOSMALL = ERR_NETWORK_DOMAIN + 6;

CDWORD ERR_BLOCK_LENGTH_INVALID = ERR_NETWORK_DOMAIN + 7;
//
CDWORD ERR_BLOCK_SEND_DATA_FAILED = ERR_NETWORK_DOMAIN + 8;
//
CDWORD ERR_BLOCK_RECV_DATA_FAILED = ERR_NETWORK_DOMAIN + 9;
//
CDWORD ERR_NETWORK_INIT_FAILED = ERR_NETWORK_DOMAIN + 0x0A;
// time out
CDWORD ERR_NETWORK_TIME_OUT		= ERR_NETWORK_DOMAIN + 0x0B;
CDWORD ERR_NETWORK_DISCONNECTED	= ERR_NETWORK_DOMAIN + 0x0C;
CDWORD ERR_NETWORK_PENDING		= ERR_NETWORK_DOMAIN + 0x0D;
CDWORD ERR_NETWORK_PARAM_INVALID = ERR_NETWORK_DOMAIN + 0x0E;


//====================================================================
// 2 user management domain error
//====================================================================
//user not authentication
CDWORD ERR_USER_NOT_AUTHENTICATION = (0x01 | ERR_USER_MNG_DOMAIN);
//user name and password don't match	
CDWORD ERR_USER_PASSWD_WRONG = (0x02 | ERR_USER_MNG_DOMAIN);
//the user's acount is using
CDWORD ERR_USER_USED = (0x03 | ERR_USER_MNG_DOMAIN);
//No this user
CDWORD ERR_USER_NO_USER = (0x04 | ERR_USER_MNG_DOMAIN);
//user has no company
CDWORD ERR_USER_NO_COMPANY = (0x05 | ERR_USER_MNG_DOMAIN);
//company is used
CDWORD ERR_USER_COMPANY_USED = (0x06 | ERR_USER_MNG_DOMAIN);
//user has no fleet
CDWORD ERR_USER_NO_FLEET = (0x07 | ERR_USER_MNG_DOMAIN);
//user name is empty
CDWORD ERR_USER_NAME_EMPTY = (0x08 | ERR_USER_MNG_DOMAIN);
//user name is existent
CDWORD ERR_USER_NAME_EXIST = (0x09 | ERR_USER_MNG_DOMAIN);
//user ID is nonexistent
CDWORD ERR_NO_USER_ID = (0x0a | ERR_USER_MNG_DOMAIN);
//company name is existent
CDWORD ERR_COMPANY_NAME_EXIST = (0x0b | ERR_USER_MNG_DOMAIN);
//no this company ID
CDWORD ERR_NO_COMPANY_ID = (0x0c | ERR_USER_MNG_DOMAIN);
//encrypt error
CDWORD ERR_USER_ENCRYPT_ERROR = (0x0D | ERR_USER_MNG_DOMAIN);
//decrypt error
CDWORD ERR_USER_DECRYPT_ERROR = (0x0E | ERR_USER_MNG_DOMAIN);
// 
CDWORD ERR_USER_INVALID_REQUEST = (0x0F | ERR_USER_MNG_DOMAIN);
CDWORD ERR_USER_DB_INVALID		= (0x10 | ERR_USER_MNG_DOMAIN);
CDWORD ERR_USER_TOKEN_NOT_EXIST = (0x11 | ERR_USER_MNG_DOMAIN);		//
CDWORD ERR_DB_EXECSQL_FAILED = (0x12 | ERR_USER_MNG_DOMAIN);
CDWORD ERR_USER_REQUEST_FAILED = (0x13 | ERR_USER_MNG_DOMAIN);




//====================================================================
// 3 Device management domain error
//====================================================================
//device need upgrade
CDWORD ERR_NEED_UPGRADE = (0x0d | ERR_DEVICE_MNG_DOMAIN);
//device is existed
CDWORD ERR_DEVICE_ALREADY_EXIST = (0X10 | ERR_DEVICE_MNG_DOMAIN);
//No this device
CDWORD ERR_NO_THIS_DEVICE = (0X11 | ERR_DEVICE_MNG_DOMAIN);
//Group is existed
CDWORD ERR_GROUP_ALREADY_EXIST = (0X12 | ERR_DEVICE_MNG_DOMAIN);
//No this Group
CDWORD ERR_NO_THIS_GROUP = (0X13 | ERR_DEVICE_MNG_DOMAIN);
//company exist same ClientID
CDWORD ERR_DEVICE_SAME_ID = (0X14 | ERR_DEVICE_MNG_DOMAIN);
//company exist same display name
CDWORD ERR_DEVICE_SAME_DISPLAY_NAME = (0x15 | ERR_DEVICE_MNG_DOMAIN);
//company exist same IP
CDWORD ERR_DEVICE_SAME_IP = (0x16 | ERR_DEVICE_MNG_DOMAIN);
//company exist same groupID
CDWORD ERR_GROUP_SAME_ID = (0x17 | ERR_DEVICE_MNG_DOMAIN);
//company exist same group name			
CDWORD ERR_GROUP_SAME_NAME = (0x18 | ERR_DEVICE_MNG_DOMAIN);
//resource is used
CDWORD ERR_DEVICE_RESOURCE_USED = (0x19 | ERR_DEVICE_MNG_DOMAIN);

//encrypt error
CDWORD ERR_DEVICE_ENCRYPT_ERROR = (0x25 | ERR_DEVICE_MNG_DOMAIN);
//decrypt error
CDWORD ERR_DEVICE_DECRYPT_ERROR = (0x26 | ERR_DEVICE_MNG_DOMAIN);

//device not online
CDWORD ERR_DEVICE_NOT_ONLINE = (0X27 | ERR_DEVICE_MNG_DOMAIN);



//====================================================================
// 4 Database management domain error
//====================================================================
CDWORD ERR_DATABASE_ACCESS =		ERR_DATABASE_DOMAIN + 1;
CDWORD ERR_DATABASE_INIT_FAIL =		ERR_DATABASE_DOMAIN + 2;
CDWORD ERR_CFG_FILE_ERROR =			ERR_DATABASE_DOMAIN + 3;

//====================================================================
// 5 base lib domain error
//====================================================================
CDWORD ERROR_CODE_SUCC = 0;
CDWORD ERROR_CODE_OBJECT_NOT_EXIST = ERR_BASE_DOMAIN + 1;		//object not exist	//对象不存在
CDWORD ERROR_CODE_BUFFER_OVER = ERR_BASE_DOMAIN + 2;			//buffer over		//缓冲满
CDWORD ERROR_CODE_DATA_ERR = ERR_BASE_DOMAIN + 3;				//data err			//数据有误
CDWORD ERROR_CODE_SEND_ERR = ERR_BASE_DOMAIN + 4;				//send datad err	//发送数据失败
CDWORD ERROR_CODE_MSG_ANALY_ERR = ERR_BASE_DOMAIN + 5;			//msg alaly err		//消息解析有误
CDWORD ERROR_CODE_BUFFER_NOTENOUGH= ERR_BASE_DOMAIN + 6;		//buffer not enough	//缓冲不足
CDWORD ERROR_CODE_OBJECT_ALREADY_EXIST	= ERR_BASE_DOMAIN + 7;	//object is already existed
CDWORD ERROR_CODE_SERVER_OVERLOAD		= ERR_BASE_DOMAIN + 8;	//object is already existed

//====================================================================
// 6 client lib domain error
//====================================================================
CDWORD SDS_NETCLIENT_ErrNotInit = ERR_CLIENT_DOMAIN + 1;		//not init			// 未初始化
CDWORD SDS_NETCLIENT_ErrHandle = ERR_CLIENT_DOMAIN + 2;			//handle not exist	// 句柄不存在
CDWORD SDS_NETCLIENT_ErrParam = ERR_CLIENT_DOMAIN + 3;			//param err			// 参数错误
CDWORD SDS_NETCLIENT_ErrBuffSize = ERR_CLIENT_DOMAIN + 4;		//buffer over		// 缓存满
CDWORD SDS_NETCLIENT_ErrNoMem = ERR_CLIENT_DOMAIN + 5;			//no memory			// 内存不足
CDWORD SDS_NETCLIENT_ErrRecv = ERR_CLIENT_DOMAIN + 6;			//receive err		// 接收错误
CDWORD SDS_NETCLIENT_ErrSend = ERR_CLIENT_DOMAIN + 7;			//send data err		// 发送错误
CDWORD SDS_NETCLIENT_ErrOperate = ERR_CLIENT_DOMAIN + 8;		//operate err		// 操作错误
CDWORD SDS_NETCLIENT_ErrCreateFile = ERR_CLIENT_DOMAIN + 9;		//create file err	// 创建文件错误
CDWORD SDS_NETCLIENT_ErrNoFreePort = ERR_CLIENT_DOMAIN + 10;	//no free port		// 没有空闲通道
CDWORD SDS_NETCLIENT_ErrProtocol = ERR_CLIENT_DOMAIN + 11;		//protocol err		// 协议错误
CDWORD SDS_NETCLIENT_ErrXMLFormat = ERR_CLIENT_DOMAIN + 12;		//xml fromat err	// 错误的XML数据
CDWORD SDS_NETCLIENT_ErrNotSupport = ERR_CLIENT_DOMAIN + 13;	//not support		// 不支持的操作
CDWORD SDS_NETCLIENT_ErrGetParam = ERR_CLIENT_DOMAIN + 14;		//get param err		// 获取参数错误
CDWORD SDS_NETCLIENT_ErrSetParam = ERR_CLIENT_DOMAIN + 15;		//set param err		// 设置参数错误
CDWORD SDS_NETCLIENT_ErrOpenFile = ERR_CLIENT_DOMAIN + 16;		//open file err		// 打开文件出错
CDWORD SDS_NETCLIENT_ErrUpgOpen = ERR_CLIENT_DOMAIN + 17;		//upgrade err		// 升级出错
CDWORD SDS_NETCLIENT_ErrConnect = ERR_CLIENT_DOMAIN + 18;		//connect err		// 连接失败


//====================================================================
// 7 device sdk lib domain error
//====================================================================
CDWORD SDS_DeviceSdk_ErrNotInit					= ERR_DEVICESDK_DOMAIN + 1;		//not init			// 未初始化
CDWORD SDS_DeviceSdk_ErrParam					= ERR_DEVICESDK_DOMAIN + 2;		//param err			// 参数错误
CDWORD SDS_DeviceSdk_NotAnswer					= ERR_DEVICESDK_DOMAIN + 3;		//not answer		// 无应答
CDWORD SDS_DeviceSdk_ParamLength_TooLong		= ERR_DEVICESDK_DOMAIN + 4;		//param too long	// 参数长度过长
CDWORD SDS_DeviceSdk_Control_Failed				= ERR_DEVICESDK_DOMAIN + 5;		//control failed	// 设备控制执行失败
CDWORD SDS_DeviceSdk_Operate_Failed				= ERR_DEVICESDK_DOMAIN + 6;		//operation failed	// 设备操作执行失败





} // namespace error
} // namespace sds

#endif	// _CMS_ERRCODE_H_
