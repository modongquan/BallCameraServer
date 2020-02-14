						Linux Device SDK 使用说明

1 版本说明 v2.3.0106.1
========================

2 编译和链接
=============

2.1 链接Release版本的sdk
========================
sdk需要静态链接boost c++库，库的链接顺序如下：

SDK_Device_Sdk.so SDS_BusinessBase.so SDS_Utility.so libboost_log.a libboost_filesystem.a libboost_system.a libboost_thread.a libboost_date_time.a
