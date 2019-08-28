#include "boost_server.h"

namespace boost{

	boost_tcp_server::boost_tcp_server(boost::shared_ptr<io_service_pool> pool, int port)
		: m_acceptor_(net_io_, tcp::endpoint(tcp::v4(), port)), work_(net_io_),
		m_manager_(net_io_), pool_(pool), protocol_func_(nullptr), bussiness_cb_(nullptr)
	{
		//default package length
		socket_bufflen_ = 10240;
		net_thread_ = new boost::thread(boost::bind(&boost::asio::io_service::run, &net_io_));
		//net_thread->detach();
		//m_manager_.start();
	}

	boost_tcp_server::~boost_tcp_server()
	{
		if (net_thread_) {
			net_thread_->join();
			net_thread_ = nullptr;
		}
	}

	void boost_tcp_server::listen()
	{
		try {
			socket_session_ptr new_session(make_shared<socket_session>(pool_->get_io_service(), socket_bufflen_));
			m_acceptor_.async_accept(new_session->get_socket(),
				boost::bind(&boost_tcp_server::handle_accept, this, new_session,
					boost::asio::placeholders::error));
		}
		catch (...)
		{
			//SPDLOG(warn, "{} start listen error", __FUNCTION__);
		}
	}

	void boost_tcp_server::register_protocol_func(protocol_functor pf, BussinessCallback* bussCB, std::size_t socket_bufflen, bool check_login)
	{
		if(pf)
			protocol_func_ = pf;
		if (bussCB)
			bussiness_cb_ = bussCB;
		socket_bufflen_ = socket_bufflen;
		check_login_ = check_login;
	}

	void boost_tcp_server::send2target(int64_t sessionID, LocalDataType data)
	{
		m_manager_.send2target(sessionID, data);
	}

	void boost_tcp_server::send2clients(LocalDataType data)
	{
		m_manager_.send2clients(data);
	}

	void boost_tcp_server::closeSession(int64_t sessionID)
	{
		m_manager_.closeSession(sessionID);
	}

	void boost_tcp_server::handle_accept(socket_session_ptr session, const boost::system::error_code& error)
	{
		if (error)
			return;

		try {
			if (session != nullptr)
			{
				if(protocol_func_)
					session->registerDataCB(protocol_func_);
				if (bussiness_cb_)
					session->registerBussinessCB(bussiness_cb_);
				session->set_remote_info();
				m_manager_.add_session(session);
				session->onConnected();

				//printf("new session id:%d\n", session->id());
				/*SPDLOG(warn, "new sessionid:{}, ip:{}, port:{}", session->id(),
					session->socket().remote_endpoint().address().to_string(),
					session->socket().remote_endpoint().port());*/
			}

			socket_session_ptr new_session(make_shared<socket_session>(pool_->get_io_service(), socket_bufflen_));
			m_acceptor_.async_accept(new_session->get_socket(),
				boost::bind(&boost_tcp_server::handle_accept, this, new_session,
					boost::asio::placeholders::error));
		}
		catch (...)
		{
			//SPDLOG(error, "{} ,unknown error", __FUNCTION__);
		}
	}

}////////////////////////////////// namespace boost