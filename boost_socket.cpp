#include "boost_socket.h"

namespace boost{

	boost::detail::atomic_count socket_session::m_last_id(0);

	socket_session::socket_session(boost::asio::io_service& io_srv, std::size_t bufflen)
		:m_io_service(io_srv), m_socket(io_srv), m_bussiness_cb(nullptr)
	{
		IsConnected = false;
		m_connection_timeout = 45;
		m_id = ++socket_session::m_last_id;
		//init recvbuf
		m_recvbuf = new char[bufflen];
		m_remainbuf = new char[bufflen*2];
		m_lessLen = 0;
		m_bufflen = bufflen;
	}

	socket_session::~socket_session(void)
	{
		if (m_recvbuf)
			delete[] m_recvbuf;
		if (m_remainbuf)
			delete[] m_remainbuf;
	}

	void socket_session::start()
	{
		m_socket.set_option(boost::asio::ip::tcp::acceptor::linger(true, 0));
		m_socket.set_option(boost::asio::socket_base::keep_alive(true));

		std::time(&m_last_time);
		do_read();
	}

	void socket_session::onConnected()
	{
		if (m_bussiness_cb)
			m_bussiness_cb->OnConnected(m_id, m_address, m_port);
	}

	void socket_session::handle_close()
	{
		try {
			IsConnected = false;
			m_sendQueue.clear();
			if (m_socket.is_open()) {
				boost::system::error_code ec;
				m_socket.shutdown(tcp::socket::shutdown_both, ec);
				m_socket.close(ec);
			}
		}
		catch (...)
		{
		}
	}

	void socket_session::close()
	{
		handle_close();
		if (m_bussiness_cb)
			m_bussiness_cb->OnClose(m_id);
	}

	bool socket_session::is_timeout()
	{
		std::time_t now;
		std::time(&now);
		return (now - m_last_time) >= m_connection_timeout;
	}

	void socket_session::SetLoginStatus(bool s)
	{
		IsConnected = s;
	}

	void socket_session::set_op_time()
	{
		std::time(&m_last_time);
	}

	void socket_session::do_read()
	{
		try {
			m_socket.async_read_some(boost::asio::buffer(m_recvbuf, m_bufflen),
				boost::bind(&socket_session::handle_read, this,
					boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
		catch (...)
		{
			close();
		}
	}

	void socket_session::handle_read(const boost::system::error_code& error, std::size_t readLen)
	{
		try {
			if (error)
			{
				close();
				return;
			}
			
			set_op_time();

			std::size_t dataSize;
			char *pbegin;
			const char *pend;
			if (m_lessLen == 0) {
				pbegin = m_recvbuf;
				dataSize = readLen;
			}
			else {
				dataSize = readLen + m_lessLen;
				if (dataSize > m_bufflen*2) {
					close();
					return;
				}
				memmove(m_remainbuf + m_lessLen, m_recvbuf, readLen);
				pbegin = m_remainbuf;
			}

			while (dataSize > 0) {
				std::size_t packageSize = 0;
				int code = m_protocol_functor(pbegin, dataSize, packageSize);
				if (code == PACKET_FULL) {
					if (m_bussiness_cb) {
						m_bussiness_cb->HandlePackage(m_id, pbegin, packageSize);
					}
					pbegin += packageSize;
					dataSize -= packageSize;
				}
				else if (code == PACKET_LESS) {
					break;
				}
				else {
					close();
					return;
				}
			}

			if (dataSize > 0) {
				m_lessLen = dataSize;
				memmove(m_remainbuf, pbegin, m_lessLen);
			}
			else
			{
				m_lessLen = 0;
			}


		}
		catch (...)
		{
			close();
		}
	}

	void socket_session::HandleAsyncWrite(LocalDataType data)
	{
		m_io_service.post([=]() {
			bool write_in_progress = !m_sendQueue.empty();
			m_sendQueue.emplace_back(data);
			if (!write_in_progress)
			{
				AsyncWrite();
			}
		});

	}

	void socket_session::AsyncWrite()
	{
		if (!IsConnected) return;

		auto msg = m_sendQueue.front();
		boost::asio::async_write(m_socket, boost::asio::buffer(msg->c_str(), msg->size()),
			[this](const boost::system::error_code& ec, std::size_t size)
		{
			if (!ec && IsConnected)
			{
				m_sendQueue.pop_front();
				if (!m_sendQueue.empty())
				{
					AsyncWrite();
				}
			}
		});
	}



}