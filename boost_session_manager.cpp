#include "boost_session_manager.h"

namespace boost {
	session_manager::session_manager(boost::asio::io_service& io_srv)
		:m_io_srv(io_srv), m_check_timer(io_srv)
	{
		m_expires_time = 60;
		check_connection();
	}

	session_manager::~session_manager()
	{
		
	}

	//void session_manager::start()
	//{
	//	check_connection();
	//}

	void session_manager::check_connection()
	{
		try {
			writeLock lock(m_mutex);
			session_map::iterator iter = m_sessions.begin();
			while (iter != m_sessions.end())
			{
				if (!iter->second.session->GetSessionStatu() && iter->second.session->is_timeout())
				{
					//printf("del session:%d\n", iter->second.id);
					m_sessions.erase(iter++);
					continue;
				}
				++iter;
			}

			m_check_timer.expires_from_now(boost::posix_time::seconds(m_expires_time));
			m_check_timer.async_wait(boost::bind(&session_manager::check_connection, this));

		}
		catch (std::exception& e)
		{
			//SPDLOG(error, "{}, {}", __FUNCTION__, e.what());
		}
		catch (...)
		{
			//SPDLOG(error, "{}, unkown exception", __FUNCTION__);
		}
	}

	void session_manager::add_session(socket_session_ptr p)
	{
		writeLock lock(m_mutex);
		p->start();

		session_def stuSession;
		stuSession.id = p->id();
		stuSession.address = p->get_remote_addr();
		stuSession.session = p;
		m_sessions.insert(session_map::value_type(p->id(), stuSession));

	}

	int session_manager::get_session_size()
	{
		writeLock lock(m_mutex);
		return m_sessions.size();
	}

	void session_manager::send2target(socket_session_ptr p, LocalDataType data)
	{
		writeLock lock(m_mutex);
		session_map::iterator iter = m_sessions.find(p->id());
		if (iter != m_sessions.end()) {
			iter->second.session->HandleAsyncWrite(data);
		}
	}

	void session_manager::send2clients(LocalDataType data)
	{
		writeLock lock(m_mutex);
		session_map::iterator iter = m_sessions.begin();
		while (iter != m_sessions.end())
		{
			iter->second.session->HandleAsyncWrite(data);
			++iter;
		}
	}

	void session_manager::send2target(int64_t sessionID, LocalDataType data)
	{
		writeLock lock(m_mutex);
		session_map::iterator iter = m_sessions.find(sessionID);
		if (iter != m_sessions.end()) {
			iter->second.session->HandleAsyncWrite(data);
		}
	}

	void session_manager::closeSession(int64_t sessionID)
	{
		del_session(sessionID);
	}

	void session_manager::del_session(int64_t id)
	{
		writeLock lock(m_mutex);

		if (m_sessions.empty())
		{
			return;
		}

		session_map::iterator iter = m_sessions.find(id);
		if (iter != m_sessions.end())
		{
			m_sessions.erase(iter);
		}
	}

	socket_session_ptr session_manager::get_session(int64_t id)
	{
		writeLock lock(m_mutex);

		if (m_sessions.empty())
		{
			return socket_session_ptr();
		}

		session_map::iterator iter = m_sessions.find(id);

		return iter != m_sessions.end() ? iter->second.session : socket_session_ptr();
	}
}