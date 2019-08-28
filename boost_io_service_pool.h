#pragma once
#ifndef _BOOST_IO_SERVICE_POOL_HPP
#define _BOOST_IO_SERVICE_POOL_HPP
#include <boost/asio.hpp>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>

namespace boost {
	class io_service_pool : private boost::noncopyable
	{
	public:
		explicit io_service_pool(std::size_t pool_size){
			next_io_service_ = 0;
			if (pool_size == 0)
				throw std::runtime_error("io_service_pool size is 0");

			for (std::size_t i = 0; i < pool_size; ++i)
			{
				io_service_ptr io_service(new boost::asio::io_service);
				work_ptr work(new boost::asio::io_service::work(*io_service));
				io_services_.push_back(io_service);
				work_.push_back(work);
			}
		}

		void run()
		{
			for (std::size_t i = 0; i < io_services_.size(); ++i)
			{
				boost::shared_ptr<boost::thread> threadEntity(boost::make_shared<boost::thread>(boost::bind(&boost::asio::io_service::run, io_services_[i])));
				threads_.push_back(threadEntity);
			}
		}

		void stop()
		{
			for (std::size_t i = 0; i < io_services_.size(); ++i)
				io_services_[i]->stop();

			for (std::size_t i = 0; i < threads_.size(); ++i)
				threads_[i]->join();
		}

		boost::asio::io_service& get_io_service()
		{
			boost::asio::io_service& io_service = *io_services_[next_io_service_];
			++next_io_service_;
			if (next_io_service_ == io_services_.size())
				next_io_service_ = 0;
			return io_service;
		}

	private:
		typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
		typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

		std::vector<shared_ptr<boost::thread> > threads_;
		std::vector<io_service_ptr> io_services_;
		std::vector<work_ptr> work_;
		std::size_t next_io_service_;
	};
}

#endif 