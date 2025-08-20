#include "sylar/bytearray.hh"
#include <string.h>

#include "sylar/log.hh"
#include "sylar/endian.hh"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace sylar {

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

ByteArray::Node::Node(size_t s)
	: ptr(new char[s]),
	  next(nullptr),
	  size(s)
{ }

ByteArray::Node::Node()
	:ptr(nullptr),
	 next(nullptr),
	 size(0)
{ }

	ByteArray::Node::~Node() {
		if(ptr) {
			delete [] ptr;
		}
	}

ByteArray::ByteArray(size_t base_size)
	:m_baseSize(base_size),
	 m_position(0),
	 m_capacity(base_size),
	 m_size(0),
	 m_endian(SYLAR_BIG_ENDIAN),
	 m_root(new Node(base_size)),
	 m_cur(m_root)
{
}

ByteArray::~ByteArray()
{
	Node* tmp = m_root;
	while(tmp) {
		m_cur = tmp;
		tmp = tmp->next;
		delete m_cur;
	}
}

void ByteArray::writeFint8(int8_t value)
{
	write(&value, sizeof(value));
}

void ByteArray::writeFuint8(uint8_t value)
{
	write(&value, sizeof(value));
}

void ByteArray::writeFint16(int16_t value)
{
	if(m_endian != SYLAR_BYTE_ORDER) {
		value = byteswap(value);
	}
	write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value)
{
	if(m_endian != SYLAR_BYTE_ORDER) {
		value = byteswap(value);
	}
	write(&value, sizeof(value));
}

void ByteArray::writeFint32(int32_t value)
{
	if(m_endian != SYLAR_BYTE_ORDER) {
		value = byteswap(value);
	}
	write(&value, sizeof(value));
}

void ByteArray::writeFuint32(uint32_t value)
{
	if(m_endian != SYLAR_BYTE_ORDER) {
		value = byteswap(value);
	}
	write(&value, sizeof(value));
}

void ByteArray::writeFint64(int64_t value)
{
	if(m_endian != SYLAR_BYTE_ORDER) {
		value = byteswap(value);
	}
	write(&value, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value)
{
	if(m_endian != SYLAR_BYTE_ORDER) {
		value = byteswap(value);
	}
	write(&value, sizeof(value));
}

static uint32_t EncodingZigzag32(const int32_t& v) {
	if(v < 0) {
		return (uint32_t)((v << 1) ^ (v >> 31));
	} else {
		return v << 1;
	}
}

static uint64_t EncodingZigzag64(const int64_t& v) {
	if(v < 0) {
		return (uint64_t)((v << 1) ^ (v >> 31));
	} else {
		return v << 1;
	}
}

static int32_t DecodingZigzag32(const uint32_t& v) {
	return ((v >> 1) ^ -(v & 1));
}

static int64_t DecodingZigzag64(const uint64_t& v) {
	return ((v >> 1) ^ -(v & 1));
}

void ByteArray::writeInt32(int32_t value)
{
	writeUint32(EncodingZigzag32(value));
}

void ByteArray::writeUint32(uint32_t value)
{
	// 可变长32位数据编码
	uint8_t tmp[5];
	uint8_t i = 0;
	while(value >= 0x80) {
		tmp[i++] = (value & 0x7F) | 0x80;
		// 这里 & 0x80,即 msb=1 表示后续还有多余字节

		value >>= 7;
	}

	// 退出循环后, msb 为0, 表示没有后续多余字节了, 直接赋值即可
	tmp[i++] = value;
	write(tmp, i);
}
void ByteArray::writeInt64(int64_t value)
{
	writeUint64(EncodingZigzag64(value));
}

void ByteArray::writeUint64(uint64_t value)
{
	uint8_t v[10];
	uint8_t i = 0;
	while(value >= 0x80) {
		// 每次编码一个字节, 即8位
		v[i++] = (value & 0x7F) | 0x80;
		value >>= 7;
	}
	v[i++] = value; // 对最后一个字节进行编码
	write(v, i);
}

void ByteArray::writeFloat(float value)
{
	uint32_t v;
	memcpy(&v, &value, sizeof(value));
	writeFuint32(v);
}

void ByteArray::writeDouble(double value)
{
	uint64_t v;
	memcpy(&v, &value, sizeof(value));
	writeFuint64(v);
}

void ByteArray::writeStringF16(const std::string &value)
{
	writeFuint16(value.size());          // 传入数据的位数
	write(value.c_str(), value.size()); // 写入 size 位数据
}

void ByteArray::writeStringF32(const std::string &value)
{
	writeFuint32(value.size());          // 传入数据的位数
	write(value.c_str(), value.size()); // 写入 size 位数据
}

void ByteArray::writeStringF64(const std::string &value)
{
	writeFuint64(value.size());          // 传入数据的位数
	write(value.c_str(), value.size()); // 写入 size 位数据
}

void ByteArray::writeStringVint(const std::string &value)
{
	writeUint64(value.size());          // 传入数据的位数
	write(value.c_str(), value.size()); // 写入 size 位数据
}

void ByteArray::writeStringWithoutLength(const std::string &value)
{
	write(value.c_str(), value.size()); // 写入 size 位数据
}

int8_t ByteArray::readFint8()
{
	int8_t v;
	read(&v, sizeof(v));
	return v;
}

#define XX(type)								                \
	type v;										                \
	read(&v, sizeof(v));						               	\
	if(m_endian == SYLAR_BYTE_ORDER) {			                \
		return v;								                \
	} else {								                    \
		return byteswap(v);						                \
	}

int16_t ByteArray::readFint16()
{
	XX(int16_t);
}

int32_t ByteArray::readFint32()
{
	XX(int32_t);
}

int64_t ByteArray::readFint64()
{
	XX(int64_t);
}

uint8_t ByteArray::readFuint8()
{
	uint8_t v;
	read(&v, sizeof(v));
	return v;
}
uint16_t ByteArray::readFuint16()
{
	XX(uint16_t);
}
uint32_t ByteArray::readFuint32()
{
	XX(uint32_t);
}
uint64_t ByteArray::readFuint64()
{
	XX(uint64_t);
}

#undef XX


int32_t ByteArray::readInt32()
{
	return DecodingZigzag32(readUint32());
}

uint32_t ByteArray::readUint32() {
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8(); // 每次读取8位
        if(b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        } else {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

int64_t ByteArray::readInt64()
{
	return DecodingZigzag64(readUint64());
}

uint64_t ByteArray::readUint64() {
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint64_t)b) << i;
            break;
        } else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

float ByteArray::readFloat()
{
	uint32_t v = readFuint32();
	float value;
	memcpy(&value, &v, sizeof(v));
	return value;
}

double ByteArray::readDouble()
{
	uint64_t v = readFuint64();
	double value;
	memcpy(&value, &v, sizeof(v));
	return value;
}

#define XX(type, fun)					   		\
	type len = fun();									\
	std::string buff;									\
	buff.resize(len);									\
	read(&buff[0], len);								\
	return buff;

std::string ByteArray::readStringF16()
{
	XX(uint16_t, readFuint16);
}

std::string ByteArray::readStringF32()
{
	XX(uint32_t, readFuint32);
}
std::string ByteArray::readStringF64()
{
	XX(uint64_t, readFuint64);
}

std::string ByteArray::readStringFVint()
{
	XX(uint64_t, readUint64);
}
#undef XX

void ByteArray::clear()
{
	m_position = m_size = 0;
	m_capacity = m_baseSize;
	Node* temp = m_root->next;
	while(temp) {
		m_cur = temp;
		temp = temp->next;
		delete m_cur;
	}
	m_cur = m_root;
	m_root->next = nullptr;
}
void ByteArray::write(const void *buf, size_t size)
{
	if(size == 0) {
		return;
	}
	addCapacity(size);

	size_t npos = m_position % m_baseSize; // 	当前 Node 内部的偏移位置
	size_t ncap = m_cur->size - npos; // 当前 Node 剩余可写容量
	size_t bpos = 0; // 要写入的源数据 buf 中的偏移位置, 用于分段拷贝

	while(size > 0) {
		if(ncap >= size) {
			// 当前块够写完全部数据
			memcpy(m_cur->ptr + npos, (const char*)buf+bpos, size);
			if(m_cur->size == (npos + size)) {
				m_cur = m_cur->next; // 写满之后, 就移动一个节点
			}
			m_position += size;
			bpos += size;
			size = 0;
		} else {
			// 当前块不够写完, 只写 ncap, 然后跳下一个块
			memcpy(m_cur->ptr + npos, (const char*)buf+bpos,ncap);
			m_position += ncap;
			bpos += ncap;
			size -= ncap;
			m_cur =m_cur->next;
			ncap = m_cur->size;
			npos = 0;
		}
	}

	if(m_position > m_size) {
		m_size = m_position;
	}
}
void ByteArray::read(void *buf, size_t size)
{
	if(size >getReadSize()){
		throw std::out_of_range("not enough len");
	}

	size_t npos = m_position % m_baseSize; // 当前node位置
	size_t ncap = m_cur->size - npos; // 当前 node 的剩余容量
	size_t bpos = 0; // buffer 中的偏移量
	while(size > 0) {
		if(ncap >= size) {
			// memcpy(m_cur->ptr + bpos, (const char*)buf+npos, size);
			memcpy((char*)buf+bpos, m_cur->ptr + npos,  size);
			if(m_cur->size == (npos+size)) {
				m_cur = m_cur->next;
			}
			m_position += size;
			bpos += size;
			size = 0;
		} else {
			memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
			size -= ncap;
			m_position += ncap;
			bpos += ncap;
			m_cur = m_cur->next;
			ncap = m_cur->size;
			npos = 0;
		}
	}
}

void ByteArray::read(void *buf, size_t size, size_t position) const
{
	if(size > (m_size - position)){
		throw std::out_of_range("not enough len");
	}

	size_t npos = m_position % m_baseSize; // 当前node位置
	size_t ncap = m_cur->size - npos; // 当前 node 的剩余容量
	size_t bpos = 0; // buffer 中的偏移量
	Node * cur = m_cur;
	while(size > 0) {
		if(ncap >= size) {
			memcpy((char*)buf+bpos, cur->ptr + npos, size);
			if(cur->size == (npos+size)) {
				cur = cur->next;
			}
			position += size;
			bpos += size;
			size = 0;
		} else {
			memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
			size -= ncap;
			position += ncap;
			bpos += ncap;
			cur = cur->next;
			ncap = cur->size;
			npos = 0;
		}
	}
}

void ByteArray::setPosition(size_t v)
{
	if(v > m_capacity) {
		throw std::out_of_range("ByteArray set position out of range");
	}

	if(m_position > m_size) {
		m_size = m_position;
	}

	m_position = v;
	m_cur = m_root;
	while( v >= m_cur->size) {
		v -= m_cur->size;
		m_cur = m_cur->next;
	}
}
bool ByteArray::writeToFile(const std::string &name) const
{
	std::ofstream ofs;
	ofs.open(name, std::ios::trunc | std::ios::binary);

	if(!ofs) {
		SYLAR_LOG_ERROR(g_logger) << "writeToFile name = " << name
								  << " error ,errno=" << errno << " errstr = " << strerror(errno);
		return false;
	}
	int64_t read_size = getReadSize();
	int64_t pos = m_position;
	Node* cur = m_cur;

	while(read_size > 0) {
		int diff = pos % m_baseSize;
		int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize: read_size)- diff;
		ofs.write(cur->ptr+diff, len);
		cur = cur->next;
		pos += len;
		read_size -= len;
	}
	return true;
}
bool ByteArray::readFromFile(const std::string &name)
{
	std::ifstream ifs;
	ifs.open(name, std::ios::binary);
	if(!ifs) {
		SYLAR_LOG_ERROR(g_logger) <<"readFroomFile name=" << name
		<< " error, errno=" << errno << " errstr=" << strerror(errno);
		return false;
	}

	std::shared_ptr<char>buf(new char[m_baseSize], [](char* ptr) { delete [] ptr; });

	while(!ifs.eof()) {
		ifs.read(buf.get(), m_baseSize);
		write(buf.get(), ifs.gcount());
	}
	return true;
}
bool ByteArray::isLittleEndian() const
{
	return m_endian == SYLAR_LITTLE_ENDIAN;
}
void ByteArray::setLittleEndian(bool val)
{
	if(val) {
		m_endian = SYLAR_LITTLE_ENDIAN;
	} else {
		m_endian = SYLAR_BIG_ENDIAN;
	}
}
std::string ByteArray::toString() const
{
	std::string str;
	str.resize(getReadSize());
	if(str.empty()) {
		return str;
	}
	read(&str[0], str.size(), m_position);
	return str;
}
std::string ByteArray::toHexString() const
{
	std::string str = toString();
	std::stringstream ss;
	for(size_t i = 0; i <str.size(); ++i) {
		if(i > 0 && i % 32 == 0) {
			ss << std::endl;
		}
		ss << std::setw(2) << std::setfill('0') << std::hex
		   << (int)(uint8_t)str[i] << " ";
	}

	return ss.str();
}
uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers,uint64_t len) const
{
	len = len >getReadSize() ? getReadSize() : len;
	if(len == 0) {
		return 0;
	}

	uint64_t size = len;
	size_t npos = m_position % m_baseSize;
	size_t ncap = m_cur->size - npos;
	struct iovec iov;
	Node * cur = m_cur;

	while(len > 0) {
		if(ncap >= len) {
			iov.iov_base = cur->ptr + npos;
			iov.iov_len = len;
			len = 0;
		} else {
			iov.iov_base = cur->ptr + npos;
			iov.iov_len = ncap;
			len -= ncap;
			cur = cur->next;
			ncap = cur->size;
			npos = 0;
		}
		buffers.push_back(iov);
	}
	return size;
}
uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers,
								   uint64_t len,
								   uint64_t position) const
{
	len = len >getReadSize() ? getReadSize() : len;
	if(len == 0) {
		return 0;
	}

	uint64_t size = len;
	size_t npos = m_position % m_baseSize;
	size_t count = position / m_baseSize;
	Node*cur = m_root;;
	while(count > 0) {
		cur = cur->next;
		--count;
	}

	size_t ncap = m_cur->size - npos;
	struct iovec iov;

	while(len > 0) {
		if(ncap >= len) {
			iov.iov_base = cur->ptr + npos;
			iov.iov_len = len;
			len = 0;
		} else {
			iov.iov_base = cur->ptr + npos;
			iov.iov_len = ncap;
			len -= ncap;
			cur = cur->next;
			ncap = cur->size;
			npos = 0;
		}
		buffers.push_back(iov);
	}
	return size;

}
uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len)
{
    if(len == 0) {
        return 0;
    }
    addCapacity(len);
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;

    struct iovec iov;
    Node* cur = m_cur;

    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

void ByteArray::addCapacity(size_t size)
{
	if(size == 0) {
		return ;
	}

	size_t old_cap = getCapacity(); // 旧容量
	if(old_cap >= size ){
		return;
	}
	size = size - old_cap; // 还需要扩容的容量

	// 还需要创建的node的个数
	size_t count = (size / m_baseSize) + (((size % m_baseSize) > old_cap) ?1 :0);

	Node* tmp =m_root;
	while(tmp->next) {
		tmp = tmp->next;
	}

	Node* first = NULL;
	for(size_t i = 0; i < count; ++i) {
		tmp->next = new Node(m_baseSize);
		if(first == NULL) {
			first = tmp->next;
		}
		tmp = tmp->next;
		m_capacity += m_baseSize;
	}

	if(old_cap == 0) {
		m_cur = first;
	}
}
} // namespace sylar
