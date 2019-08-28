#pragma once
#ifndef _BOOST_CLIENT_H
#define _BOOST_CLIENT_H
#include <list>
#include "boost_socket_callback.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/atomic/atomic.hpp>
#include "boost_socket.h"

using std::list;

namespace boost{
	using boost::asio::ip::tcp;

	class boost_tcp_client
	{
	public:
		boost_tcp_client(boost::asio::io_service& io_service, const std::string& ipaddress, int port, std::size_t socket_bufflen);

		virtual ~boost_tcp_client();

		void init_login(LocalDataType login_msg, bool autoSend);

		void init_heartbeat(LocalDataType heartbeat_msg, int heartbeat, bool autoSend);

		void register_callback(AsyncSocketCallback *cb, const protocol_functor& pf);

		void connect();

		void send(LocalDataType data);

		void close();

	private:

		void closeSocket();

		void do_heartbeat(const boost::system::error_code& error);

		void do_login(LocalDataType msg);

		void do_read();

		void handle_read(std::size_t length);

		void AsyncWrite();

		void async_reconnect(int type);

	private:
		boost::asio::io_service& io_service_;
		boost::asio::deadline_timer heartbeat_timer_;
		boost::asio::deadline_timer reconnect_timer_;

		LocalDataType login_package_;
		LocalDataType heartbeat_package_;
		bool autoSendLogin_;
		bool autoSendHeartbeat_;
		int heartbeat_;

		std::size_t lessLen;
		char *m_buff;
		char *readbuff;

		std::list<LocalDataType> m_sendQueue;

		AsyncSocketCallback *callback;
		protocol_functor protocol_functor_;
		std::size_t socket_bufflen_;

		tcp::socket socket_;
		std::string ip_;
		int port_;
		boost::atomic<bool> connected_;
		boost::atomic<bool> CanReconnected_;

		std::string errmsg;
		int errcode;
	};
}

#endif
