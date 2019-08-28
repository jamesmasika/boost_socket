#pragma once
#ifndef _BOOST_SOCKET_FACTORY_H
#define _BOOST_SOCKET_FACTORY_H
#include "boost_client.h"
#include "boost_server.h"
#include "boost/serialization/singleton.hpp"
#include "boost_io_service_pool.h"
#include "boost_server.h"

namespace boost {

	typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
	typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

	class boost_socket_factory : public boost::serialization::singleton<boost_socket_factory>
	{
	public:

		void Init()
		{
			if (pool_ != nullptr)
				return;
			int size = boost::thread::hardware_concurrency();
			pool_ = boost::make_shared<io_service_pool>(size);
			pool_->run();
		}

		boost::shared_ptr<boost_tcp_client> CreatClient(const std::string& ipaddress, const int port, const std::size_t socket_bufflen)
		{
			assert(pool_ != nullptr);

			return boost::make_shared<boost_tcp_client>(pool_->get_io_service(), ipaddress, port, socket_bufflen);
		}

		boost::shared_ptr<boost_tcp_server> CreatServer(const int port)
		{
			assert(pool_ != nullptr);

			return boost::make_shared<boost_tcp_server>(pool_, port);
		}

		void Join()
		{
			pool_->stop();
		}

	private:
		boost::shared_ptr<io_service_pool> pool_;
	};
}
#endif