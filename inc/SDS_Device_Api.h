
/*
creater: SDS
date: 2017-08-12
desc: 设备接入sdk接口
SDS_Device_Api
his:
*/

#ifndef _SDS_Device_Api_h_
#define _SDS_Device_Api_h_

#pragma once

#include "SDS_Device_DataDefine.h"

int		SDS_NetDevice_test();

namespace SDS_Device
{
	namespace sdk                                               
	{

#if defined(WIN32) || defined(_WINDOWS) || defined(_WIN32)

	#ifdef SDSDEVICESDK_EXPORTS
		#define SDS_Device_Api extern "C" __declspec(dllexport)
	#else
		#define SDS_Device_Api extern "C" __declspec(dllimport)
	#endif

#else

	#define SDS_Device_Api  extern "C" 

	#define CALLBACK  

#endif

		typedef void*		_handle;

		//-------------------------------------------------------------------------------------
		// 功能：事件回调函数
		// 参数：	llUserData:用户数据
		//			strEvent:数据类型  见请求数据类型定义
		// 返回：无
		//-------------------------------------------------------------------------------------
		//Function:
		//	return alert or operation information to application level.
		//Parameters :
		//	llUserData : user self - specified parameter;
		//  strEvent: detail information of callback, string type with json format.
		//	event_type: event type
		//Remarks :
		//	in josn string, eventType is event type, see eNetEvent declaration.
		typedef int(CALLBACK *OnEventCB)(long long llUserData, const char* strEvent, int event_type);

		//-------------------------------------------------------------------------------------
		// 功能：帧数据回调函数
		// 参数：	stream:流句柄
		//			lpBuf:帧数据
		//			size:长度
		//			llStamp:时间戳
		//			type:类型
		//			llUserData:用户数据
		// 返回：无
		//-------------------------------------------------------------------------------------
		//Function:
		//	return decoded information to application level.
		//Parameters :
		//	stream : session handle of stream.
		//	lpBuf : video frame data;
		//	size: video frame data length;
		//	llstamp: time stamp of frame;
		//	type: frame type, see eFrameType declaration;
		//	channel: device channel (index based 1)
		//	llUserData: user self - specified parameter;
		typedef int(CALLBACK *OnFrameCB)(_handle stream, BYTE* lpBuf, int size, long long llStamp, int type, int channel, long long llUserData);


		typedef struct st_work_param
		{
			char		server[20];				//ip to listen							//监听ip
			int			port;					//port to listen						//监听端口
			OnEventCB	callbackEvent;			//event callback						//事件回调(信令服务使用)
			OnFrameCB	callbackFrame;			//frame callback						//帧数据回调(媒体服务使用)
			long long	llUserData;				//user self-specified parameter			//用户参数
		}st_work_param;



		//-------------------------------------------------------------------------------------
		// 功能：初始化
		// 参数：
		// 返回：0 成功 或错误码, 见 SDS_NETDEVICE_ERR说明;
		//-------------------------------------------------------------------------------------
		//function: init
		//Return value :
		//	Error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	port: listening port;
		//	callbackEvent: callback function, see Event Callback description;
		//	llUserData: user self - specified parameter
		SDS_Device_Api int		SDS_NetDevice_Init(st_work_param param);

		//-------------------------------------------------------------------------------------
		// 功能：停止
		// 参数：无
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function:Uninitialization
		//Return value :
		//		NA
		SDS_Device_Api void		SDS_NetDevice_Uninit();

		//-------------------------------------------------------------------------------------
		// 功能：设置流媒体服务器参数
		// 参数：szIp：媒体服务器ip port : 媒体服务器监听端口
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function : Set media server parameters.
		//Return value :
		// Error code, see SDS_NETDEVICE_ERR description;
		//Parameter :
		//szIp：media server ip address
		//	port : media server listening port
		SDS_Device_Api	int	SDS_NetDevice_SetMediaParam(const char* szIp, int port);
		SDS_Device_Api	int	SDS_NetDevice_GetMediaParam(char* szIp, int& port);

		//-------------------------------------------------------------------------------------
		// 功能：获取媒体流句柄对应的guid 标识参数
		// 参数：handle：媒体流句柄 szParam : 媒体流句柄参数（约定长度<128）
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function: Get media service session parameters.
		//Return value :
		//	NA
		//Parameters :
		//	handle：session handle
		//	szParam : session parameter
		SDS_Device_Api int		SDS_NetDevice_GetHandleParam(_handle handle, char* szParam);
	
		//-------------------------------------------------------------------------------------
		// 功能：设置媒体流句柄对应的guid 标识参数
		// 参数：handle：媒体流句柄 szParam : 媒体流句柄参数
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function: Set media service session parameters.
		//Return value :
		//	Error code, see SDS_NETDEVICE_ERR description;
		//Parameters :
		//	handle：session handle
		//	szParam : session parameter
		SDS_Device_Api int		SDS_NetDevice_SetHandleParam(_handle handle, const char* szParam);

		//-------------------------------------------------------------------------------------
		// 功能：设置设备对象用户数据
		// 参数：deviceId：设备ID user_data : 用户数据
		// 返回：0 成功 或错误码, 见 SDS_NETDEVICE_ERR说明;
		// 说明：当信令服务器有设备上线时， 调用此接口， 可绑定上层对象到sdk
		//-------------------------------------------------------------------------------------
		//function: Set device user self-defined data.
		//Return value :
		//	Error code, see SDS_NETDEVICE_ERR description;
		//Parameters :
		//	deviceId: device ID, 
		//	user_data : user self-defined data
		SDS_Device_Api int SDS_NetDevice_SetUserData(const char* deviceId, long long user_data);

		//-------------------------------------------------------------------------------------
		// 功能：搜索设备
		// 参数：port：端口
		// 返回：0 成功 或错误码, 见 SDS_NETDEVICE_ERR说明;
		// 搜索结果通过事件回调产生
		//-------------------------------------------------------------------------------------
		//function: Search device on local network
		//Return value :
		//	Error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	port: scan port;
		SDS_Device_Api int		SDS_NetDevice_Search(int port);

		//-------------------------------------------------------------------------------------
		// 功能：停止搜索设备
		// 参数：
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function: Stop Search device on local network
		//Return value :
		//	Error code, see SDS_NETDEVICE_ERR description;
		SDS_Device_Api int		SDS_NetDevice_StopSearch();

		//-------------------------------------------------------------------------------------
		// 功能：登录
		// 参数：ip  port  username: 用户名  psw: 密码   timeout  超时时长
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function: login device (This API is for user to login to the device on local network.)
		//Return value :
		//	Success(= 0) or error code(see SDS_NETDEVICE_ERR description);
		//Parameters:
		//	ip: device IP;
		//	port: device TCP port;
		//	userName: username to login;
		//	psw: user password to login;
		//	timeout: timeout to login(seconds);
		SDS_Device_Api int		SDS_NetDevice_Login(const char* ip, int port, const char* userName, const char* psw, int timeout);

		//-------------------------------------------------------------------------------------
		// 功能：退出
		// 参数：无
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function: user logout
		//Return value :
		//	Error code(see SDS_NETDEVICE_ERR description);
		SDS_Device_Api int		SDS_NetDevice_Logout();

		//-------------------------------------------------------------------------------------
		// 功能：订阅报警信息
		// 参数：deviceId 设备ID, 若为空, 或长度为0, 则订阅全部设备的报警信息;  bSubscribe: true 订阅  false 取消订阅  
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function: Subscribe/Cancel Subscribe Alert
		//Return:
		//	Error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//  deviceId: device ID, if empty or the length of id is zero, then system assume to subscribe all owned device alert;
		//  bSubscribe: true - to subscribe; false - cancel subscribe
		SDS_Device_Api int		SDS_NetDevice_GetAlarm(const char* deviceId, bool bSubscribe);

		//-------------------------------------------------------------------------------------
		// 功能：订阅Gps信息
		// 参数：deviceId 设备ID, 若为空, 或长度为0, 则订阅全部设备的Gps信息;  bSubscribe: true 订阅  false 取消订阅  
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function: Subscribe/Cancel Subscribe gps information
		//Return:
		//	Error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//  deviceId: device ID, if empty or the length of id is zero, then system assume to subscribe all owned device alert;
		//  bSubscribe: true - to subscribe; false - cancel subscribe
		SDS_Device_Api int		SDS_NetDevice_GetGPS(const char* deviceId, bool bSubscribe);

		//-------------------------------------------------------------------------------------
		// 功能：录像、抓拍查询
		// 参数：deviceId 设备ID,   type: 1 普通录像 2报警录像 3所有录像 4普通截图 5报警截图 6所有截图 见eMediaType
		//		startTime : 开始时间, 时间格式如 : 年 - 月 - 日 时 : 分:秒	endTime: 结束时间, 时间格式如 : 年 - 月 - 日 时 : 分:秒
		//		chnFlag : 通道编号列表( 从1开始，  98表示所有通道 )
		//		查询结果通过数据帧回调返回， 返回的json 格式信息
		//		query : 查询句柄
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//  function：Query Video or Snap Picture List
		//	Return value :
		//	Handle to query(>= 0) or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//  deviceId: device ID;
		//	type:  media type, see eMediaType
		//	startTime : start time, time format looks like : Year - Month - Day Hour : Minute:Second
		//	endTime : end time.time format is the same as start time.
		//	chnFlag : Channel number list(index based 1, if equal 98, means search all channels);
		//		 query[out]: API output a session handle, user may stop the query to call the SDS_NetDevice_CloseQuery() with this session handle
		//	Remarks :
		//		 Return the json file information as follows :
		//		 {‘num’:2, ‘filelist’ : [{‘filename’: ‘filepath1’, ‘bTime’ : ‘begin time’, ‘eTime’ : ‘end time’, ‘filesize’ : 111, ‘timelen’ : 222, ‘channel’ : 1, ‘type’ : 1}, { ‘filename’: ‘filepath2’, ‘bTime’ : ‘begin time’, ‘eTime’ : ‘end time’, ‘filesize’ : 111, ‘timelen’ : 222, ‘channel’ : 1, ‘type’ : 1 }]}
		SDS_Device_Api int		SDS_NetDevice_Query(const char* deviceId, int type, const char* startTime, const char* endTime, int chnFlag, _handle& query);
		SDS_Device_Api int		SDS_NetDevice_QueryCmd(const char* uuid, const char* deviceId, int type, const char* startTime, const char* endTime, int chnFlag, _handle& query);
	
		//-------------------------------------------------------------------------------------
		// 功能：关闭查询
		// 参数：query:查询句柄;
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function：close query
		//Return value :
		//	Error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	query: handle of this query created by SDS_NetDevice_Query;
		SDS_Device_Api int		SDS_NetDevice_CloseQuery(_handle query);

		//-------------------------------------------------------------------------------------
		// 功能：抓拍下载  
		// 参数：deviceId:设备ID; devPath:设备文件路径  download:下载句柄; 下载数据通过数据帧回调返回
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function：download snap picture
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	deviceId: device ID;
		//	devPath: device file path, see Query Video List description;
		//	download: handle of download session
		SDS_Device_Api int		SDS_NetDevice_DownloadSnap(const char* deviceId, const char* devPath, _handle& download);
		SDS_Device_Api int		SDS_NetDevice_DownloadSnapCmd(const char* uuid, const char* deviceId, const char* devPath, _handle& download);

		//-------------------------------------------------------------------------------------
		// 功能：关闭抓拍下载  
		// 参数：download:下载句柄;
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function：close download
		//Return value :
		//	NA
		//Parameters :
		//	download: handle of download session
		SDS_Device_Api int		SDS_NetDevice_CloseDownloadSnap(_handle download);

		SDS_Device_Api int		SDS_NetDevice_DownloadFile(const char* deviceId, const char* devPath, _handle& download);
		SDS_Device_Api int		SDS_NetDevice_DownloadFileCmd(const char* uuid, const char* deviceId, const char* devPath, _handle& download);
		SDS_Device_Api int		SDS_NetDevice_CloseDownloadFile(_handle download);


		//-------------------------------------------------------------------------------------
		// 功能：设备端抓拍
		// 参数：deviceId:设备ID; channel 通道号  capture:抓拍句柄; 抓拍数据通过数据帧回调返回
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function：snap picture from device
		//Return value :
		//	NA
		//Parameters :
		//	deviceId：device ID
		//	channel：device channel number (index based 1);
		//	capture: handle of capture session
		SDS_Device_Api int		SDS_NetDevice_Capture(const char* deviceId, int channel, _handle& capture);
		SDS_Device_Api int		SDS_NetDevice_CaptureCmd(const char* uuid, const char* deviceId, int channel, _handle& capture);
		

		//-------------------------------------------------------------------------------------
		// 功能：停止设备端抓拍 
		// 参数：capture:下载句柄;
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function：close download
		//Return value :
		//	NA
		//Parameters :
		//	capture: handle of capture session
		SDS_Device_Api int		SDS_NetDevice_CloseCapture(_handle capture);



		//-------------------------------------------------------------------------------------
		// 功能：设备升级 
		// 参数：deviceId:设备ID; devPath:升级文件路径  upgrade:升级句柄
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function：device upgrade
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	deviceId: device ID;
		//	devPath: upgrade device file path, 
		//	download: handle of upgrade session
		SDS_Device_Api int		SDS_NetDevice_UpgradeStart(const char* deviceId, const char* devPath, const char* version, _handle& upgrade);
		SDS_Device_Api int		SDS_NetDevice_UpgradeStartCmd(const char* uuid, const char* deviceId, const char* devPath, const char* version, _handle& upgrade);

		//-------------------------------------------------------------------------------------
		// 功能：关闭设备升级 
		// 参数：upgrade:升级句柄
		// 返回：无
		//-------------------------------------------------------------------------------------
		//function：close upgrade
		//Return value :
		//	NA
		//Parameters :
		//	download: handle of upgrade session
		SDS_Device_Api int		SDS_NetDevice_UpgradeStop(_handle upgrade);


		//-------------------------------------------------------------------------------------
		// 功能：云台控制  
		// 参数：deviceId:设备ID; channel 通道 action 动作 见 ePtzActionType 说明 value1 参数值1 value2 参数值2
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function：Ptz Control
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	deviceId: device ID;
		//	channel: device channel(index based 1);
		//	action: see ePtzActionType  description;
		//	value1: Reserved1(x speed / y speed / presetting bit);
		//	value2: Reserved2(y speed);
		SDS_Device_Api int		SDS_NetDevice_PtzCtrl(const char* deviceId, int channel, int action, int value1, int value2);


		//-------------------------------------------------------------------------------------
		// 功能：获取设备io状态  
		// 参数：deviceId:设备ID; dwInputStatus:输入口状态（按bit位表示，1 表示 有输入） nInputCount：输入口数量 dwOutputStatus：输出口状态 nOutputCount：输出口数量
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function：Get Device IO Status
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	deviceId: device ID;
		//	dwInputStatus[output]: status of input ports, every bit in the DWORD represents for one port status
		//	nInputCount[output] : the number of input ports
		//	dwOutputStatus[output] : status of output ports
		//	nOutputCount[output] : the number of output ports

		SDS_Device_Api int		SDS_NetDevice_GetIoStatus(const char* deviceId, DWORD& dwInputStatus, int& nInputCount, DWORD& dwOutputStatus, int& nOutputCount);


		//-------------------------------------------------------------------------------------
		// 功能：请求获取设备信息  
		// 参数：deviceId:设备ID; 
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		// 发送请求后， 设备会应答上报设备相关软硬件信息
		//-------------------------------------------------------------------------------------
		//function: Request Device Information
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	deviceId: device ID;
		// after send request, device will report its information about software version, hardware status
		SDS_Device_Api int		SDS_NetDevice_RequestDeviceInfo(const char* deviceId);
	
		//-------------------------------------------------------------------------------------
		// 功能：设备控制  
		// 参数：deviceId:设备ID; nCtrlType:控制类型，	1：断开油路2：恢复油路3：断开电路 4：恢复电路5：设备重启6：恢复出厂配置 见 eCtrlType
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function: Device Control
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	deviceId: device ID;
		//	nCtrlType: control operation type, 1 : break oil - way 2 : restore oil - way 3 : break electric circuit 4 : restore electric circuit 5 : restart device 6 : restore Factory Settings
		SDS_Device_Api int		SDS_NetDevice_Ctrl(const char* deviceId, int nCtrlType);
	

		//-------------------------------------------------------------------------------------
		// 功能：设备串口数据透传
		// 参数：deviceId:设备ID; lpData：透传数据 len：数据长度
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function: Device CommonPort Transparent
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	deviceId: device ID;
		//	lpData：tran data   len：data length
		SDS_Device_Api int		SDS_NetDevice_SendData2CommonPort_Transparent(const char* deviceId, BYTE* lpData, int len);


		//-------------------------------------------------------------------------------------
		// 功能：设备参数设置及获取  
		// 参数：deviceId:设备ID; nType:参数类型，	pInCfg : 设置参数	pOutCfg: 获取参数
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function:Device Vendor Special Data Transport
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	deviceId: device ID;
		//	nType: type of transport, vendor special code.
		//	pInCfg : the data send to the device.
		//	pOutCfg[output] : the responded data from the device
		SDS_Device_Api int		SDS_NetDevice_TranConfig(const char* deviceId, int nType, datastruct::pst_device_config pInCfg, datastruct::pst_device_config pOutCfg);
		SDS_Device_Api int		SDS_NetDevice_TranConfigEx(const char* deviceId, int nType, datastruct::pst_device_config pInCfg);
	
		//-------------------------------------------------------------------------------------
		// 功能：打开实时流  
		// 参数：deviceId:设备ID;			channel:设备通道号(从1开始);	stream_type: 流类型 0 主码流， 1 子码流，2 音频流 见 eStreamType
		//		 stream:流句柄
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function: Open Realtime Video/Audio Stream
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	deviceID: device ID;
		//	channel: device channel number(index based 1);
		//	stream_type:
		//		0 : main stream;
		//		1: sub stream;
		//		2: audio stream
		//	stream : session handle of stream
		SDS_Device_Api int		SDS_NetDevice_OpenStream(const char* deviceId, int channel, int stream_type, _handle& stream);
		SDS_Device_Api int		SDS_NetDevice_OpenStreamCmd(const char* uuid, const char* deviceId, int channel, int stream_type, _handle& stream);
		

		//-------------------------------------------------------------------------------------
		// 功能：关闭实时流  
		// 参数： stream:流句柄
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function:Close Realtime Video/Audio Stream
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	stream: session handle of stream；
		//	open : true : open audio stream, false : close audio stream
		SDS_Device_Api int		SDS_NetDevice_CloseStream(_handle stream);

		//-------------------------------------------------------------------------------------
		// 功能：云台控制  
		// 参数：stream:流句柄 action 动作 见 ePtzActionType 说明 value1 参数值1 value2 参数值2
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;  
		//-------------------------------------------------------------------------------------
		//function：Ptz Control
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	action: see ePtzActionType  description;
		//	value1: Reserved1(x speed / y speed / presetting bit);
		//	value2: Reserved2(y speed);
		SDS_Device_Api int		SDS_NetDevice_PtzCtrlOnStream(_handle stream, int action, int value1, int value2);

		//-------------------------------------------------------------------------------------
		// 功能：打开对讲  
		// 参数：deviceId:设备ID; channel:设备通道号(从1开始) （目前无效，保留）;	talk:流句柄
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function：open talk
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	deviceID: device ID;
		//	channel: device channel number(index based 1);
		//	talk: session handle of talk
		SDS_Device_Api int		SDS_NetDevice_TalkStart(const char* deviceId, int channel, _handle& talk);
		SDS_Device_Api int		SDS_NetDevice_TalkStartCmd(const char* uuid, const char* deviceId, int channel, _handle& talk);
	
		//-------------------------------------------------------------------------------------
		// 功能：发送音频数据  
		// 参数：talk: 对讲句柄; audioType: 音频类型 lpData：音频数据 len：数据长度
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//Send Talk Audio Data
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//talk: session handle of talk
		//	lpData：audio data
		//	len：data length
		SDS_Device_Api int		SDS_NetDevice_TalkSend(_handle talk, int audioType, BYTE* lpData, int len);

		//-------------------------------------------------------------------------------------
		// 功能：停止对讲 
		// 参数：talk: 对讲句柄; 
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function：close talk
		//Return value :
		//	NA;
		//Parameters:
		//	talk: session handle of talk
		SDS_Device_Api int		SDS_NetDevice_TalkStop(_handle talk);

		//-------------------------------------------------------------------------------------
		// 功能：打开远程回放 
		// 参数：deviceID:设备的GUID;;devpath:文件路径
		// channel: 通道编号列表(从1开始，  98表示所有通道)
		//	onlyIFrame:0, 回放所有帧;1, 只回放I帧; startPos:开始位置，时间值，单位秒;	
		//	stream：流句柄
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function：Open Remote Video Playback
		//Return value :
		//	SUCCESS(= 0) Or error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//	deviceID: device GUID;
		//	channel: device channel(index based 1)
		//	devpath: device file path,
		//	onlyIFrame:
		//	startPos : start position of video, specified by time value, unit is second;
		//	stream: session handle of stream

		SDS_Device_Api int		SDS_NetDevice_ReplayStart(const char* deviceId, int channel, const char* devpath, int onlyIFrame, int startPos, _handle& stream);
		SDS_Device_Api int		SDS_NetDevice_ReplayStartCmd(const char* uuid, const char* deviceId, int channel, const char* devpath, int onlyIFrame, int startPos, int endPos, _handle& stream);
	
		//-------------------------------------------------------------------------------------
		// 功能：停止远程回放
		// 参数：stream: 流句柄; 
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function：Close Remote Video Playback
		//Return value :
		//	Error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//stream: session handle of stream
		SDS_Device_Api int		SDS_NetDevice_ReplayStop(_handle stream);



		//-------------------------------------------------------------------------------------
		// 功能：获取色彩参数
		// 参数：deviceId: 设备id; channel: 通道号  exposure 爆光度0-255，brightness亮度0-255，contrast对比度-127 127， hue色调-127 127，saturation饱合度 0-255
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function：get colour param
		//Return value :
		//	Error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//deviceId: device id   channel
		//exposure 0-255  brightness 0-255   contrast -127 127   hue -127 127    saturation 0-255
		SDS_Device_Api int		SDS_NetDevice_GetColourParam(const char* deviceId, int channel, int& exposure, int& brightness, int& contrast, int& hue, int& saturation);

		//-------------------------------------------------------------------------------------
		// 功能：设置色彩参数
		// 参数：deviceId: 设备id;  channel: 通道号 exposure 爆光度0-255，brightness亮度0-255，contrast对比度-127 127， hue色调-127 127，saturation饱合度 0-255
		// 返回：成功(= 0)或错误码, 见 SDS_NetDevice_ERR 说明;
		//-------------------------------------------------------------------------------------
		//function：set colour param
		//Return value :
		//	Error code, see SDS_NETDEVICE_ERR description;
		//Parameters:
		//deviceId: device id   channel
		//exposure 0-255  brightness 0-255   contrast -127 127   hue -127 127    saturation 0-255
		SDS_Device_Api int		SDS_NetDevice_SetColourParam(const char* deviceId, int channel, int exposure, int brightness, int contrast, int hue, int saturation);







		//-------------------------------------------------------------------------------------
		// 功能：打开实时流  
		// 参数：deviceId:设备Ip(设备地址);		channel:设备通道号(从1开始);	stream_type: 流类型 0 子码流， 1 主码流，2 音频流 见 eStreamType
		//		 stream:流句柄
		// 返回：成功(= 0)或错误码, 见 SDS_NETCLIENT_ERR 说明;
		//-------------------------------------------------------------------------------------
		SDS_Device_Api int		SDS_NetDevice_DirectOpenStream(const char* deviceIp, int port, int channel, int stream_type, _handle& stream);


		//-------------------------------------------------------------------------------------
		// 功能：关闭实时流  
		// 参数： stream:流句柄
		// 返回：成功(= 0)或错误码, 见 SDS_NETCLIENT_ERR 说明;
		//-------------------------------------------------------------------------------------
		SDS_Device_Api int		SDS_NetDevice_DirectStopStream(_handle stream);

		//-------------------------------------------------------------------------------------
		// 功能：云台控制  
		// 参数：stream:流句柄  action 动作 见 ePtzActionType 说明 value1 参数值1 value2 参数值2
		// 返回：成功(= 0)或错误码, 见 SDS_NETCLIENT_ERR 说明;
		//-------------------------------------------------------------------------------------
		SDS_Device_Api int		SDS_NetDevice_DirectPtzCtrl(const char* deviceIp, int port, int channel, int action, int value1, int value2);

	}
}
#endif