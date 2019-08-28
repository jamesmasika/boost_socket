#include "boost_client.h"
#include <boost/locale/encoding.hpp>
#include <iostream>

namespace boost{

#define READ_SIZE 10240
#define MAX_BUFFER_SIZE 10240*4

#define QUIET 99
#define NOTIFY 100

	boost_tcp_client::boost_tcp_client(boost::asio::io_service & io_service, const std::string & ipaddress, int port, std::size_t socket_bufflen)
	: io_service_(io_service), socket_(io_service_),
		heartbeat_timer_(io_service_), reconnect_timer_(io_service_),
		ip_(ipaddress), port_(port), socket_bufflen_(socket_bufflen),
		callback(nullptr), connected_(false), heartbeat_(8),
		autoSendLogin_(false),autoSendHeartbeat_(false)
	{
		lessLen = 0;
		CanReconnected_ = true;
		if (socket_bufflen_ < 2048)
			socket_bufflen_ = 2048;
		m_buff = new char[socket_bufflen_*2];
		readbuff = new char[socket_bufflen_];
	}

	boost_tcp_client::~boost_tcp_client()
	{
		if (m_buff)
			delete[] m_buff;
		if (readbuff)
			delete[] readbuff;
	}

	void boost_tcp_client::init_login(LocalDataType login_msg, bool autoSendLogin)
	{
		login_package_ = login_msg;
		autoSendLogin_ = autoSendLogin;
	}

	void boost_tcp_client::init_heartbeat(LocalDataType heartbeat_msg, int heartbeat, bool autoSendHeartbeat)
	{
		heartbeat_package_ = heartbeat_msg;
		heartbeat_ = heartbeat;
		autoSendHeartbeat_ = autoSendHeartbeat;
	}

	void boost_tcp_client::register_callback(AsyncSocketCallback * cb, const protocol_functor & pf)
	{
		callback = cb;
		protocol_functor_ = pf;
	}

	void boost_tcp_client::connect()
	{
		if (!CanReconnected_)
			return;
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(ip_), port_);
		socket_.async_connect(ep, [this](boost::system::error_code ec)
		{
			if (!ec)
			{
				if (callback) {
					callback->onConnected();
				}
				// µÇÂ¼
				if(autoSendLogin_)
					do_login(login_package_);
				else {
					connected_ = true;
					do_read();
				}

				// ÐÄÌø°ü
				heartbeat_timer_.expires_from_now(boost::posix_time::seconds(heartbeat_));
				heartbeat_timer_.async_wait(boost::bind(&boost_tcp_client::do_heartbeat, this, boost::asio::placeholders::error));
			}
			else
			{
				errmsg = ec.message();
				//errcode = ec.value();
				async_reconnect(NOTIFY);
			}
		});
	}

	void boost_tcp_client::send(LocalDataType data)
	{
		if (!data || !connected_)
			return;
		io_service_.post([=]() {
			bool write_in_progress = !m_sendQueue.empty();
			m_sendQueue.emplace_back(data);
			if (!write_in_progress)
			{
				AsyncWrite();
			}
		});

	}

	void boost_tcp_client::close()
	{
		CanReconnected_ = false;
		closeSocket();
	}

	void boost_tcp_client::closeSocket()
	{
		connected_ = false;
		heartbeat_timer_.cancel();

		if (socket_.is_open()) {
			boost::system::error_code ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			socket_.close(ec);
		}

		lessLen = 0;
	}

	void boost_tcp_client::do_heartbeat(const boost::system::error_code& error)
	{
		if (!error) {
			if (socket_.is_open()) {
				if(autoSendHeartbeat_)
					send(heartbeat_package_);

				heartbeat_timer_.expires_from_now(boost::posix_time::seconds(heartbeat_));
				heartbeat_timer_.async_wait(boost::bind(&boost_tcp_client::do_heartbeat, this, boost::asio::placeholders::error));
			}
		}
	}

	void boost_tcp_client::do_login(LocalDataType msg)
	{
		boost::asio::async_write(socket_,
			boost::asio::buffer(msg->c_str(), msg->size()),
			[this](boost::system::error_code ec, std::size_t /*length*/)
		{
			if (!ec)
			{
				connected_ = true;
				do_read();
			}
			else
			{
				errmsg = ec.message();
				//errcode = ec.value();
				async_reconnect(NOTIFY);
			}
		});
	}

	void boost_tcp_client::do_read()
	{
		socket_.async_read_some(boost::asio::buffer(readbuff, READ_SIZE),
			[this](boost::system::error_code ec, std::size_t length)
		{
			if (!ec) {
				handle_read(length);
			}
			else
			{
				errmsg = ec.message();
				//errcode = ec.value();
				async_reconnect(NOTIFY);
			}
		});
	}

	void boost_tcp_client::handle_read(std::size_t readLen)
	{
		std::size_t dataSize;
		char *pbegin;
		const char *pend;
		if (lessLen == 0) {
			pbegin = readbuff;
			dataSize = readLen;
			//pend = pbegin + readLen;
		}
		else {
			dataSize = readLen + lessLen;
			if(dataSize > MAX_BUFFER_SIZE) {
				errmsg = "remain data size overflow";
				errcode = -1;
				async_reconnect(NOTIFY);
				return;
			}
			memmove(m_buff + lessLen, readbuff, readLen);
			pbegin = m_buff;
			//pend = pbegin + dataSize;
		}

		while (dataSize > 0) {
			std::size_t packageSize = 0;
			int code = protocol_functor_(pbegin, dataSize, packageSize);
			if (code == PACKET_FULL) {
				if (callback) {
					callback->onPackage(pbegin, packageSize);
				}
				pbegin += packageSize;
				dataSize -= packageSize;
			}
			else if (code == PACKET_LESS) {
				break;
			}
			else {
				errmsg = "decode PACKET_ERR";
				errcode = -2;
				async_reconnect(NOTIFY);
				return;
			}
		}

		if (dataSize > 0) {
			lessLen = dataSize;
			memmove(m_buff, pbegin, lessLen);
		}
		else
		{
			lessLen = 0;
		}

		do_read();
	}

	void boost_tcp_client::AsyncWrite()
	{
		auto msg = m_sendQueue.front();
		boost::asio::async_write(socket_, boost::asio::buffer(msg->c_str(), msg->size()),
			[this](const boost::system::error_code& ec, std::size_t size)
		{
			if (!ec && connected_)
			{
				m_sendQueue.pop_front();
				if (!m_sendQueue.empty())
				{
					AsyncWrite();
				}
			}
			else {
				if (!m_sendQueue.empty())
				{
					m_sendQueue.clear();
				}
			}
		});
	}


	void boost_tcp_client::async_reconnect(int type)
	{
		closeSocket();

		if (callback && type != QUIET) {
			std::string utf8_str = boost::locale::conv::between(errmsg, "UTF-8", "GBK");
			callback->onClose(utf8_str);
		}

		reconnect_timer_.expires_from_now(boost::posix_time::seconds(2));
		reconnect_timer_.async_wait(boost::bind(&boost_tcp_client::connect, this));
	}

}