#include "http/stream.hh"

namespace sylar {

int Stream::readFixSize(void *buffer, size_t length)
{
	size_t offset = 0;
	size_t left = length;
	size_t read_size = 0;
	while(left > 0) {
		read_size = read((char*)buffer+offset, length);
		if(read_size <= 0) {
			return read_size;
		}

		offset += read_size;
		left -= read_size;
	}

	return length;
}

int Stream::readFixSize(ByteArray::ptr ba, size_t length)
{
	size_t left = length;
	size_t read_size = 0;
	while(left > 0) {
		read_size = read(ba, length);
		if(read_size <= 0) {
			return read_size;
		}

		left -= read_size;
	}

	return length;
}


int Stream::writeFixSize(const void *buffer, size_t length)
{
	size_t offset = 0;
	size_t write_size = 0;
	size_t left = length;

	while(left > 0) {
		write_size = write((char*)buffer+offset, length);
		if(write_size <= 0) {
			return write_size;
		}

		offset += write_size;
		left -= write_size;
	}

	return length;
}

int Stream::writeFixSize(ByteArray::ptr ba, size_t length)
{
	size_t write_size = 0;
	size_t left = length;

	while(left > 0) {
		write_size = write(ba, length);
		if(write_size <= 0) {
			return write_size;
		}

		left -= write_size;
	}

	return length;
}

}; // namespace sylar
