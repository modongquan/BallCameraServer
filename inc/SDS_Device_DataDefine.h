/*
creater: SDS
date: 2017-07-12
desc: 设备接入数据定义
his:
*/


#ifndef _SDS_Device_datadefine_h_
#define _SDS_Device_datadefine_h_

#pragma once

namespace SDS_Device
{	
	namespace constants
	{
		
	}

	namespace enums
	{
		// note: this definition is the same as the DB table
		typedef enum eNetworkType
		{
			eLocalAreaNetwork	= 1,
			e2G					= 2,
			e3G					= 3,
			e4G					= 4,
			eWifi				= 5,
			e5G					= 6
		}eNetworkType;

		//媒体类型	//media type
		typedef enum eMediaType
		{
			eCOMMON_VIDEO	= 1,		//common video				//普通录像  
			eALERT_VIDEO	= 2,		// video of alert			//报警录像
			eAll_VIDEO		= 3,		// video of all				//所有录像
			eCOMMON_SNAP	= 4,		// common snapped picture	//普通截图
			eALERT_SNAP		= 5,		// snapped picture of alert	//报警截图
			eAll_SNAP		= 6,		// snapped picture of all	//所有截图
		}eMediaType;

		// 通天星返回的媒体类型，和请求的不同，TTX搞反了
		typedef enum eReturnMediaType
		{
			eRetAlertVideo	= 1,	// video of alert			//报警录像
			eRetCommonVideo = 2,	// common video				//普通录像  
		}eReturnMediaType;

		//云台控制类型
		typedef enum
		{
			ePtzActionNull = 0x00,				// invalid			// 无效
			ePtzActionUp = 0x01,				// up				// 上
			ePtzActionDown = 0x02,				// down				// 下
			ePtzActionLeft = 0x03,				// left				// 左
			ePtzActionRight = 0x04,				// right			// 右
			ePtzActionLeftUp = 0x05,			// left up			// 左上
			ePtzActionRightUp = 0x06,			// right up			// 左下
			ePtzActionLeftDown = 0x07,			// left down		// 左下
			ePtzActionRigthDown = 0x08,			// right down		// 右下
			ePtzActionZoomIn = 0x09,			// zoom in(camera lens far)		// 变焦大(镜头拉远)
			ePtzActionZoomOut = 0x0a,			// zoom out(camera lens near)	// 变焦小(镜头拉近)
			ePtzActionFocusNear = 0x0b,			// focus near		// 聚焦近
			ePtzActionFocusFar = 0x0c,			// focus far		// 聚焦远
			ePtzActionIrisOpen = 0x0D,			// iris large		// 光圈扩大
			ePtzActionIrisClose = 0x0E,			// iris small		// 光圈缩小	
			ePtzActionRunCruise = 0x0F,			// run cruise		// 开始巡航
			ePtzActionStopCruise = 0x10,		// stop cruise		// 停止巡航
			ePtzActionSetPreset = 0x11,			// set preset		// 设置预置点 
			ePtzActionClearPreset = 0x12,		// clear preset		// 清除预置点 
			ePtzActionGotoPreset = 0x13,		// goto preset		// 转到预置点
			ePtzActionLightOpen = 0x14,			// open light power	// 接通灯光电源
			ePtzActionLightClose = 0x15,		// close light power// 关闭灯光电源
			ePtzActionWiperOpen = 0x16,			// open wiper switch// 接通雨刷开关
			ePtzActionWiperClose = 0x17,		// close wiper switch// 关闭雨刷开关
			ePtzActionStop = 0xFF,				// stop					//停止
		}ePtzActionType;

		//设备控制类型
		typedef enum eCtrlType
		{
			eOil_cutoff = 1,					//cut off oil				//断开油路
			eOil_recover = 2,					//recover oil				//恢复油路
			eCircuit_cutoff = 3,				//cut off circuit			//断开电路
			eCircuit_recover = 4,				//recover circuit			//恢复电路
			eDevice_reboot = 5,					//reboot device				//重启设备
			eFactoryConfig_recover = 6,			//recover factory config	//恢复出厂配置
			eFormatHDD = 14,					//format hdd				//格式化硬盘(高位表示硬盘号,硬盘号从0开始,低位表示命令号)
			eMileageClear = 15,					//clear mileage				//里程清零
			eAlarmClear = 18,					//clear alarm				//清除报警
		}eCtrlType;

		//流类型
		typedef enum eStreamType
		{
			eMainStream = 0,				//main stream	//主码流
			eSubStream = 1,					//sub stream	//子码流
			eAudioStream = 2,				//audio stream	//音频流
		}eStreamType;


		typedef enum
		{
			NETEVENT_UNKNOWN = -1,				// not defined		// 未定义
			NETEVENT_LOGINING = 0,				// on login			// 正在登录
			NETEVENT_LOGIN_OK = 1,				// login succ		// 登录成功
			NETEVENT_LOGIN_ERROR = 2,			// login fail		// 登录失败
			NETEVENT_LOGOUT = 3,				// logout			// 登出
			NETEVENT_STREAM_OPENING = 4,		// open stream		// 流正在打开
			NETEVENT_STREAM_OK = 5,				// stream open succ	// 流打开成功
			NETEVENT_STREAM_ERROR = 6,			// stream open fail	// 流打开失败
			NETEVENT_STREAM_CLOSE = 7,			// stream close		// 流关闭
			NETEVENT_PB_OPENING = 8,			// opening playback stream	// 回放流正在打开
			NETEVENT_PB_OK = 9,					// open playback stream succ// 回放流打开成功
			NETEVENT_PB_ERROR = 10,				// open playback stream fail// 回放流打开失败
			NETEVENT_PB_CLOSE = 11,				// close playback stream	// 回放流关闭
			NETEVENT_TALK_OPENING = 12,			// talk opening		// 对讲正在打开
			NETEVENT_TALK_OK = 13,				// open talk succ	// 对讲打开成功
			NETEVENT_TALK_ERROR = 14,			// open talk fail	// 对讲打开失败
			NETEVENT_TALK_CLOSE = 15,			// close talk		// 对讲关闭
			NETEVENT_PTZ_CTRL = 16,				// ptz control		// 云台控制
			NETEVENT_DEV_CTRL = 17,				// device control	// 设备控制
			NETEVENT_STREAM_LOST = 18,			// stream lost		// 流丢失
			NETEVENT_NOTIFY = 19,				// alert event		// 报警事件
			NETEVENT_VIDEO_LIST = 20,			// get video list	// 获取列表
			NETEVENT_TRANS_PERCENT = 21,		// transport progress percentage // 传输百分比
			NETEVENT_GPS_INFO = 22,				// gps information	// GPS信息
			NETEVENT_SEARCH_DEVICE = 23,		// search device	// 搜索设备信息
			NETEVENT_DEVICE_ONLINE = 24,		// device online	// 设备上线
			NETEVENT_DEVICE_OFFLINE = 25,		// device offline	// 设备下线
			NETEVENT_DEVICE_REGISTER = 26,		// device register	// 设备注册
			NETEVENT_CAPTURE = 27,				// device capture   // 设备抓拍
			NETEVENT_ANSWER = 28,				// answer			// 应答
			NETEVENT_SUBSCRIBE_GPS = 29,		// subscribe gps	// 订阅gps
			NETEVENT_SUBSCRIBE_ALARM = 30,		// subscribe alarm	// 订阅alarm
			NETEVENT_QUERY = 31,				// query			// 查询
			NETEVENT_DOWNLOAD_FILE = 32,		// download snap	// 抓拍下载
			NETEVENT_GET_IO_STATUS = 33,		// get io status	// 获取io状态
			NETEVENT_DEVICE_CONFIG = 34,		// device config	// 设备参数
			NETEVENT_DEVICE_GETCOLOUR = 35,		// get colour		// 获取设备色彩参数
			NETEVENT_DEVICE_SETCOLOUR = 36,		// set colour		// 设置设备色彩参数
			NETEVENT_QUERYEND = 37,				// query end		// 查询结束
			NETEVENT_QUERYRET = 38,				// query ret		// 查询结果返回
			NETEVENT_DEVICE_INFO = 39,			// get device info	// 获取设备信息
			NETEVENT_COMMONPORT_OPEN_OK = 40,	// open port succ	// 打开串口成功
			NETEVENT_COMMONPORT_OPEN_ERROR = 41,// open port error	// 打开串口失败
			NETEVENT_COMMONPORT_CLOSE = 42,		// close port		// 关闭
			NETEVENT_COMMONPORT_DATA = 43,		// port data	    // 串口数据
			NETEVENT_DOWNLOADING_FILE = 44,		// downloading snap	// 抓拍、日志文件下载中
		}eNetEvent;

		//数据类型
		typedef enum ENUM_DATATYPE_FrameType
		{
			em_FrameType_RHeader_Frame = 0x69643030,	//head frame			//回放头帧('00di')  i  69   j 6a  k 6b l 6c m  6d  n 6e  o 6f
			em_FrameType_Header_Frame = 0x68643030,		//head frame			//头帧('00dh')  i  69   j 6a  k 6b l 6c m  6d  n 6e  o 6f
			em_FrameType_Video_IFrame = 0x62643030,		//video main frame		//视频主帧('00db')
			em_FrameType_Video_VFrame = 0x63643030,		//video virtual frame	//视频虚帧('00dc')
			em_FrameType_Audio_Frame = 0x62773030,		//audio frame g726		//音频帧('00wb') G726
			em_FrameType_Audio_FrameT = 0x62773130,		//audio frame g711		//音频帧('01wb') G711
			em_FrameType_GPS_Frame = 0x73706730,		//gps frame				//GPS帧('0gps')
			em_FrameType_FileList_Frame = 0x74736C66,	//filelist frame		//文件列表帧('flst') 
			em_FrameType_FileListEnd_Frame = 0x6574736C,//filelist frame		//文件列表结束帧('lste') 
			em_FrameType_Snap_Frame = 0x70616e73,		//snap frame			//抓拍帧('snap') 
			em_FrameType_Data_Frame = 0x74616430,		//data frame			//数据帧('0dat') 
			em_FrameType_Status_Frame = 0x74617473,		//status frame			//状态帧('stat') 任务描述
			em_FrameType_Data2_Frame = 0x63643930,		//data frame			//数据帧('09dc')
			em_FrameType_Log_Frame = 0x676F6C30,		//data frame			//数据帧('0log')
		}ENUM_DATATYPE_FrameType;

		typedef enum
		{
			eAlarmTypeInput = 0x01,			// input alert		// 输入报警
			eAlarmTypeMotion = 0x02,		// motion detect	// 移动侦测
			eAlarmTypeVideoLost = 0x03,		// video lost		// 视频丢失
			eAlarmTypeBlock = 0x04,			// video block		// 视频遮挡
			eAlarmTypeSpeedOver = 0x05,		// over speed alert	// 超速报警
			eAlarmTypeGpsErr = 0x06,		// gps module exception		//GPS模块异常
			eAlarmTypeGsensorErr = 0x07,	// Gsensor module exception	// Gsensor模块异常
			eAlarmTypeSimErr = 0x08,		// SIM card lost	// SIM 卡丢失
			eAlarmTypeStoreErr = 0x09,		// storage device exception	// 设备存储异常
			eAlarmTypeSystemErr = 0x0A,		// system exception			// 系统异常
			eAlarmTypeVoltageLow = 0x0B,	// low voltage				// 低电压
			eAlarmTypeDiskTemperatureOver = 0x0C,	//temperature of hard disk too high		// 硬盘温度过高
			eAlarmTypeFrontDoorOpen = 0x0D,			//front panel opened		// 前面板打开
			eAlarmTypeCustom = 0x0E,			//custom		// 自定义
			eAlarmTypePeopleInOut = 0x0F,			//people in out	// 上下车人数报警
			eAlarmTypeRfidCard = 0x10,				//rfid	// rfid 刷卡报警
			eAlarmTypeAccOn = 0x11,					//AccOn		// ACC开启报警
			eAlarmTypeAccOff = 0x12,				//AccOff	// ACC关闭报警
			eAlarmTypeFileUpload = 0x13,			//FileUpload// 文件上传报警

			eAlarmTypeLowSpeed = 0x15,				//low speed alert	// 低速报警
			eAlarmTypeEfence = 0x16,				//efence alert	// 电子围栏报警
			eAlarmTypeNightDrive = 0x17,			//night drive alert	// 夜间行车报警
			eAlarmTypeParkingOuttime = 0x18,		//parking outtime alert	// 停车超时报警
			eAlarmTypeFatigueDrive = 0x19,			//fatigue drive alert	// 疲劳驾驶报警
			eAlarmTypeRapidAcceleration = 0x1A,		//rapid acceleration alert	// 急加速
			eAlarmTypeRapidDeceleration = 0x1B,		//rapid deceleration alert	// 急减速

		}eAlarmType;

		typedef enum em_res_type
		{
			em_res_type_realav = 0,		// 实时音视频（云台）
			em_res_type_realaudio = 1,	// 实时音频
			em_res_type_snap = 2,		// 抓拍
			em_res_type_talk = 3,		// 对讲
			em_res_type_downsnap = 4,	// 下载抓拍
			em_res_type_query = 5,		// 查询
			em_res_type_replay = 6,		// 回放
			em_res_type_upgrade = 7,	// 升级
			em_res_type_download = 8,	// download video file or segment
			em_res_type_undispatched_query = 9,		// query video file size
			em_res_type_undispatched_download = 10,	// download video file or segment
			em_res_type_undispatched_dl_file = 11	// download special file
		}em_res_type;
	}

	namespace datastruct                                              
	{
		typedef struct st_device_config
		{
			int		nLength;					// length of data buffer	// 数据缓存长度
			char	cBuffer[2048];				// data content				// 数据内容
		} st_device_config, *pst_device_config;
	}
}

#endif