#pragma once
#ifndef _BOOST_SERVER_H
#define _BOOST_SERVER_H
#include "boost_session_manager.h"
#include "boost_io_service_pool.h"

namespace boost{


	class boost_tcp_server{
	public:
		boost_tcp_server(boost::shared_ptr<io_service_pool> pool, int port);
		~boost_tcp_server();
		void listen();
		void register_protocol_func(protocol_functor pf, BussinessCallback* bussCB, std::size_t socket_bufflen, bool check_login);
		void send2target(int64_t sessionID, LocalDataType data);
		void send2clients(LocalDataType data);
		void closeSession(int64_t sessionID);
	private:
		void handle_accept(socket_session_ptr session, const boost::system::error_code& error);


	private:
		boost::asio::io_service net_io_;
		boost::asio::io_service::work work_;
		boost::thread *net_thread_;
		tcp::acceptor m_acceptor_;
		session_manager m_manager_;
		boost::shared_ptr<io_service_pool> pool_;

		protocol_functor protocol_func_;
		bool check_login_;
		BussinessCallback* bussiness_cb_;
		std::size_t socket_bufflen_;
	};

}

#endif