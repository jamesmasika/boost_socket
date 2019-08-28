#pragma once
#ifndef _BOOST_SOCKET_CALLBACK_H
#define _BOOST_SOCKET_CALLBACK_H
#include <string>
#include <functional>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
using std::string;

namespace boost {
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

	enum
	{
		PACKET_LESS = 0,
		PACKET_FULL = 1,
		PACKET_ERR = -1,
	};

	typedef boost::shared_ptr<std::string> LocalDataType;
	/**
	   *first parameter --- the source buffer
	   *second parameter --- the size of buffer
	   *third parameter --- if the buffer meets the protocol, assign to the complete size
	*/
	typedef std::function<int(const char *, const std::size_t, std::size_t &)> protocol_functor;

	/**
	   *first parameter --- socket id
	   *second parameter --- data ptr
	   *third parameter --- data length
	*/
	//typedef std::function<int(int64_t, const char *, const std::size_t)> bussiness_functor;

	

	class AsyncSocketCallback
	{
	public:
		enum FAILED_CODE
		{
			Failed_Net = 0x01,      //网络出错
			Failed_Connect = 0x02,      //连接服务器出错
			Failed_Protocol = 0x03,     //协议解析错误
			Failed_Close = 0x04,      //服务器主动关闭了链接
			Failed_ConnectTimeout = 0x05,//连接服务器超时
		};

		virtual void onPackage(const char* sBuffer, std::size_t length) = 0;

		virtual void onConnected() = 0;

		virtual void onClose(const std::string& errmsg) = 0;
	};

	class BussinessCallback {
	public:
		virtual int HandlePackage(int64_t sessionID, const char* sBuffer, std::size_t length) = 0;

		virtual void OnConnected(int64_t sessionID, const std::string& ip, int port) = 0;

		virtual void OnClose(int64_t sessionID) = 0;
	};

}

#endif



