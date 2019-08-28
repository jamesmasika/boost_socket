#pragma once
#ifndef _BOOST_SOCKET_H
#define _BOOST_SOCKET_H
#include <string>
#include <unordered_map>
#include <atomic>
#include "boost_socket_callback.h"

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/detail/atomic_count.hpp>

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/make_unique.hpp>
#include <boost/enable_shared_from_this.hpp>

using std::string;
using boost::asio::ip::tcp;

namespace boost {

	class socket_session;
	typedef boost::shared_ptr<socket_session> socket_session_ptr;

	class socket_session :
		public boost::enable_shared_from_this<socket_session>,
		private boost::noncopyable
	{
	public:

		socket_session(boost::asio::io_service& io_service, std::size_t bufflen);

		~socket_session();

		void start();

		void onConnected();

		void HandleAsyncWrite(LocalDataType data);

	public:
		int64_t id() { return m_id; }

		tcp::socket& get_socket() { return m_socket; }

		std::string get_remote_addr() { return m_address; }

		void set_remote_info() 
		{
			m_address = m_socket.remote_endpoint().address().to_string();
			m_port = m_socket.remote_endpoint().port();
		}
		//register readData CallBackPtr functor
		void registerDataCB(protocol_functor cb) { m_protocol_functor = cb; }

		//register bussiness CallBackPtr functor
		void registerBussinessCB(BussinessCallback* cb) { m_bussiness_cb = cb; }
		
		bool GetSessionStatu() { return IsConnected; }
		
		bool is_timeout();

	private:

		void SetLoginStatus(bool s);

		void set_op_time();

		void close();
		
		void AsyncWrite();

		void do_read();

		void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);

		void handle_close();

	private:
		static boost::detail::atomic_count m_last_id;
		tcp::socket m_socket;
		boost::asio::io_service& m_io_service;

		int64_t m_id;
		std::atomic<bool> IsConnected;
		int m_connection_timeout;

		std::string m_address;
		int m_port;

		char *m_recvbuf;
		char *m_remainbuf;
		std::size_t m_bufflen;
		std::size_t m_lessLen;


		std::time_t m_last_time;

		BussinessCallback* m_bussiness_cb;
		protocol_functor m_protocol_functor;

		std::list< LocalDataType > m_sendQueue;

	};

}

#endif