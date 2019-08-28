#pragma once
#ifndef _BOOST_SESSION_MANAGER_H
#define _BOOST_SESSION_MANAGER_H
#include "boost_socket.h"
#include <mutex>
#include <set>
#include <map>

namespace boost{

	struct  session_def
	{
		socket_session_ptr session;
		int port;
		int64_t	id;
		std::string address;
	};

	typedef std::unordered_map<int64_t, session_def> session_map;

	class session_manager
	{
	public:
		typedef std::unique_lock<std::mutex> writeLock;

		session_manager(boost::asio::io_service& io_srv);

		~session_manager();

		//void start();

		void add_session(socket_session_ptr p);

		void send2clients(LocalDataType data);

		void send2target(int64_t sessionID, LocalDataType data);

		void closeSession(int64_t sessionID);

	private:
		int get_session_size();

		void send2target(socket_session_ptr p, LocalDataType data);

		void del_session(int64_t id);

		socket_session_ptr get_session(int64_t id);

	private:
		void check_connection();

	private:
		boost::asio::io_service& m_io_srv;
		boost::asio::deadline_timer m_check_timer;
		std::mutex m_mutex;

		int m_expires_time;
		session_map m_sessions;
	};



}


#endif
