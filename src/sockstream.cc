#include "http/socketstream.hh"


namespace sylar{


SocketStream::SocketStream(Socket::ptr sock, bool owner)
	:m_socket(sock),
	 m_owner(owner)
{ }

SocketStream::~SocketStream()
{
	if(m_owner && m_socket) {
		m_socket->close();
	}
}

int SocketStream::read(void *buffer, size_t length)
{
	if(!isConnected()) {
		return -1;
	}
	return m_socket->recv(buffer, length);
}

int SocketStream::read(ByteArray::ptr ba, size_t length)
{
	if(!isConnected()) {
		return -1;
	}
	std::vector<iovec> iovs;
	ba->getWriteBuffers(iovs, length);
	int read_size = m_socket->recv(&iovs[0], iovs.size());
	if(read_size > 0) {
		ba->setPosition(ba->getPosition() + read_size);
	}
	return read_size;
}

int SocketStream::write(const void *buffer, size_t length)
{
	if(!isConnected()) {
		return -1;
	}
	return m_socket->send(buffer, length);
}

int SocketStream::write(ByteArray::ptr ba, size_t length)
{
	if(!isConnected()) {
		return -1;
	}
	std::vector<iovec> iovs;
	ba->getWriteBuffers(iovs, length);
	int write_size = m_socket->send(&iovs[0], iovs.size());
	if(write_size > 0) {
		ba->setPosition(ba->getPosition() + write_size);
	}
	return write_size;
}

void SocketStream::close()
{
	if(m_socket) {
		m_socket->close();
	}
}


bool SocketStream::isConnected() const
{
	return m_socket && m_socket->isConnected();
}

};
