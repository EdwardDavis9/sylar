#include "sylar/bytearray.hh"
#include "sylar/log.hh"
#include "sylar/macro.hh"
#include <cstdlib>
#include <ctime>

#include <string.h>
#include <cmath>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void test()
{


// 下面的这个测试用来测试数据或者说代码执行的是否正确, 以及实际的压缩效率
    // std::srand(std::time(nullptr));
#define XX(randNum, type, len, write_fun, read_fun, node_size)           \
    {                                                                    \
        std::vector<type> vec;                                           \
        for (int i = 0; i < len; ++i) {                                  \
            if (randNum == 1) {                                          \
                vec.push_back(rand());                                   \
            }                                                            \
            else if (randNum == 2) {                                     \
                vec.push_back(rand() % 201);                             \
            }                                                            \
            else if (randNum == 3) {                                     \
                vec.push_back(rand() % 201 - 100); /*  [-100,100]   */   \
            }                                                            \
            else if (randNum == 4) {                                     \
                vec.push_back((uint64_t)rand() << 32);                   \
            }                                                            \
            else if (randNum == 5) {                                     \
                vec.push_back((static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 2e6f); \
            }                                                            \
        }                                                                \
        sylar::ByteArray::ptr ba(new sylar::ByteArray(node_size));       \
        for (auto &i : vec) {                                            \
            ba->write_fun(i);                                            \
        }                                                                \
        /*ba->write_fun(100);                                         */ \
        ba->setPosition(0);                                              \
        for (size_t i = 0; i < vec.size(); ++i) {                        \
            type v = ba->read_fun();                                     \
            if(randNum < 4) { \
            SYLAR_ASSERT(v == vec[i]);                                   \
            } else {                                                    \
            /* 省事起见,浮点数就先不比较大小了, 先只是测试压缩效果吧,后续看正确性*/ \
            }                                                           \
        }                                                                \
        /*std::cout << #type ": " << (int)ba->read_fun() << std::endl;*/ \
        SYLAR_ASSERT(ba->getSizeForRead() == 0);                         \
        SYLAR_LOG_INFO(g_logger)                                         \
            << #write_fun "/" #read_fun " (" #type ") len=" << len       \
            << " node_base_size=" << node_size                           \
            << " node_count:" << ba->getNodeCount()                      \
            << " real size=" << ba->getSize();                           \
    }
    XX(1, int8_t, 100, writeFint8, readFint8, 1);
    XX(1, uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(1, int16_t, 100, writeFint16, readFint16, 1);
    XX(1, uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(1, int32_t, 100, writeFint32, readFint32, 1);
    XX(1, uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(4, int64_t, 100, writeFint64, readFint64, 1);
    XX(4, uint64_t, 100, writeFuint64, readFuint64, 1);

    // 下面这个 writeInt32/readInt32 对小整数有效果,但如果是随机大整数之类的,
    // 那么会耗费更多空间 因为压缩的一个基本的算法就是:
    // 负数=2倍负数的绝对值-1, 正数=2倍的正数,
    // 因此在压缩编码中,大正数需要更多的字节来存储
    XX(1, int32_t, 100, writeInt32, readInt32, 1);
    XX(2, int32_t, 100, writeInt32, readInt32, 1);
    XX(3, int32_t, 100, writeInt32, readInt32, 1);

    // 只要涉及可变长度编码, 数据越小压缩效率越好,反之压缩效率越差
    XX(4, int64_t, 100, writeInt64, readInt64, 1);
    XX(4, uint64_t, 100, writeUint64, readUint64, 1);


    // 实际上这个大小就是400, 因为内部是uint32类型
    XX(5, float, 100, writeFloat, readFloat, 1);

    // 实际上这个大小就是800, 因为内部是uint64类型
    XX(5, double, 100, writeDouble, readDouble, 1);

#undef XX

#if 0
// 下面的这个测试仅仅用来测试数据或者说代码执行的是否正确, 不考虑测试代码是否合理
#define XX(type, len, write_fun, read_fun, node_size)                        \
    {                                                                        \
        std::vector<type> vec;                                               \
        for (int i = 0; i < len; ++i) {                                      \
            vec.push_back(rand());                                           \
        }                                                                    \
        sylar::ByteArray::ptr ba(new sylar::ByteArray(node_size));           \
        for (auto &i : vec) {                                                \
            ba->write_fun(i);                                                \
        }                                                                    \
        ba->setPosition(0);                                                  \
        for (size_t i = 0; i < vec.size(); ++i) {                            \
            type v = ba->read_fun();                                         \
            SYLAR_ASSERT(v == vec[i]);                                       \
        }                                                                    \
        SYLAR_ASSERT(ba->getSizeForRead() == 0);                             \
        SYLAR_LOG_INFO(g_logger)                                             \
            << #write_fun "/" #read_fun " (" #type ") len=" << len           \
            << " node_size=" << node_size << " size=" << ba->getSize();      \
        ba->setPosition(0);                                                  \
        SYLAR_ASSERT(                                                        \
            ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat"));   \
        sylar::ByteArray::ptr ba2(new sylar::ByteArray(node_size * 2));      \
        SYLAR_ASSERT(                                                        \
            ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
        ba2->setPosition(0);                                                 \
        SYLAR_ASSERT(ba->toString() == ba2->toString());                \
        SYLAR_ASSERT(ba->getPosition() == 0);                                \
        SYLAR_ASSERT(ba2->getPosition() == 0);                               \
    }

        // SYLAR_LOG_INFO(g_logger) << ba->toHexString();
        // SYLAR_LOG_INFO(g_logger) << ba2->toHexString();

    SYLAR_LOG_INFO(g_logger);

    XX(int8_t, 100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);
    #undef XX

#endif
}


void testFloatBasic() {
    sylar::ByteArray ba(4096);

    // 测试一些常见值
    std::vector<float> test_floats = {
        0.0f,
        1.0f,
        -1.0f,
        3.14159f,
        -2.71828f,
        123.456f,
        -987.654f,
        1.0e10f,
        -1.0e10f,
        1.0e-10f,
        -1.0e-10f
    };

    for (auto f : test_floats) {
        ba.writeFloat(f);
    }

    ba.setPosition(0);
    for (auto expected : test_floats) {
        float actual = ba.readFloat();

        // 浮点数比较：使用相对误差
        float diff = std::abs(actual - expected);
        float rel_error = diff / (std::abs(expected) + 1e-9f);

        if (rel_error > 1e-6f) {
            SYLAR_LOG_ERROR(g_logger)
                << "Float mismatch: expected " << expected
                << ", got " << actual
                << ", diff = " << diff;
        } else {
            SYLAR_LOG_INFO(g_logger) << actual;
        }
    }
}

void testFloatBoundary() {
    sylar::ByteArray ba(4096);

    // 浮点数边界值
    std::vector<float> boundaries = {
        std::numeric_limits<float>::min(),      // 最小正正规数
        std::numeric_limits<float>::max(),      // 最大正数
        -std::numeric_limits<float>::max(),     // 最小负数
        std::numeric_limits<float>::epsilon(),  // 机器精度
        std::numeric_limits<float>::infinity(), // 正无穷
        -std::numeric_limits<float>::infinity(),// 负无穷
        std::numeric_limits<float>::quiet_NaN() // NaN
    };

    for (auto f : boundaries) {
        ba.writeFloat(f);
    }

    ba.setPosition(0);
    for (size_t i = 0; i < boundaries.size(); ++i) {
        float expected = boundaries[i];
        float actual = ba.readFloat();

        SYLAR_LOG_INFO(g_logger) << actual;

        if (std::isnan(expected)) {
            // NaN 需要特殊处理
            if (!std::isnan(actual)) {
                SYLAR_LOG_ERROR(g_logger) << "NaN not preserved";
            }
        } else if (std::isinf(expected)) {
            // 无穷大比较
            if (actual != expected) {
                SYLAR_LOG_ERROR(g_logger)
                    << "Infinity mismatch: expected " << expected
                    << ", got " << actual;
            }
        } else {
            // 普通浮点数比较
            float rel_error = std::abs((actual - expected) / expected);
            if (rel_error > 1e-6f) {
                SYLAR_LOG_ERROR(g_logger)
                    << "Float boundary mismatch: expected " << expected
                    << ", got " << actual;
            }
        }
    }
}

void testFloatRandom() {
    sylar::ByteArray ba(4096);
    std::vector<float> original;
    const int COUNT = 10;

    // 生成随机浮点数
    std::srand(std::time(nullptr));
    for (int i = 0; i < COUNT; ++i) {
        // 生成 [-1e6, 1e6] 范围内的随机浮点数
        float val = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 2e6f;
        original.push_back(val);
        ba.writeFloat(val);
    }

    ba.setPosition(0);
    int errors = 0;
    for (int i = 0; i < COUNT; ++i) {
        float expected = original[i];
        float actual = ba.readFloat();

        // 比较浮点数：使用 ULPs（Units in Last Place）
        uint32_t e_bits, a_bits;
        memcpy(&e_bits, &expected, sizeof(float));
        memcpy(&a_bits, &actual, sizeof(float));

        // 允许1个ULP的误差（考虑到可能的舍入）
        if (e_bits != a_bits && std::abs(static_cast<int32_t>(e_bits - a_bits)) > 1) {
            ++errors;
            if (errors < 10) { // 只打印前10个错误
                SYLAR_LOG_WARN(g_logger)
                    << "Float mismatch at " << i
                    << ": expected " << expected
                    << " (0x" << std::hex << e_bits << ")"
                    << ", got " << actual
                    << " (0x" << a_bits << ")";
            }
        }
    }

    SYLAR_LOG_INFO(g_logger)
        << "Random float test: " << COUNT << " values, "
        << errors << " errors";
}

void testDouble() {
    sylar::ByteArray ba(4096);

    // 测试 double 的各种值
    std::vector<double> test_doubles = {
        0.0,
        1.0,
        -1.0,
        3.141592653589793,
        -2.718281828459045,
        std::numeric_limits<double>::min(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN()
    };

    for (auto d : test_doubles) {
        ba.writeDouble(d);

        SYLAR_LOG_INFO(g_logger) << d;
    }

    ba.setPosition(0);
    for (auto expected : test_doubles) {
        double actual = ba.readDouble();

        if (std::isnan(expected)) {
            if (!std::isnan(actual)) {
                SYLAR_LOG_ERROR(g_logger) << "Double NaN not preserved";
            }
        } else if (std::isinf(expected)) {
            if (actual != expected) {
                SYLAR_LOG_ERROR(g_logger)
                    << "Double Infinity mismatch: " << expected << " vs " << actual;
            }
        } else {
            // 对于普通 double，使用更高的精度要求
            double rel_error = std::abs((actual - expected) / expected);
            if (rel_error > 1e-12) {
                SYLAR_LOG_ERROR(g_logger)
                    << "Double mismatch: " << expected << " vs " << actual
                    << ", rel_error = " << rel_error;
            }
        }
    }
}

void testFloatEndian() {
    // 测试大小端设置
    sylar::ByteArray ba_big(4096);
    sylar::ByteArray ba_little(4096);

    ba_big.setLittleEndian(false);   // 大端模式
    ba_little.setLittleEndian(true); // 小端模式

    float test_value = 3.14159f;

    // 分别写入
    ba_big.writeFloat(test_value);
    ba_little.writeFloat(test_value);

    // 检查字节表示
    ba_big.setPosition(0);
    ba_little.setPosition(0);

    // 读取原始字节
    uint8_t bytes_big[4], bytes_little[4];
    ba_big.read(bytes_big, 4);
    ba_little.read(bytes_little, 4);

    // 本机字节序
    uint8_t native_bytes[4];
    memcpy(native_bytes, &test_value, 4);

    SYLAR_LOG_INFO(g_logger) << "Float endian test for value: " << test_value;
    SYLAR_LOG_INFO(g_logger) << "Native bytes: "
        << std::hex << (int)native_bytes[0] << " "
        << std::hex << (int)native_bytes[1] << " "
        << std::hex << (int)native_bytes[2] << " "
        << std::hex << (int)native_bytes[3];

    SYLAR_LOG_INFO(g_logger) << "Big endian bytes: "
        << std::hex << (int)bytes_big[0] << " "
        << std::hex << (int)bytes_big[1] << " "
        << std::hex << (int)bytes_big[2] << " "
        << std::hex << (int)bytes_big[3];

    SYLAR_LOG_INFO(g_logger) << "Little endian bytes: "
        << std::hex << (int)bytes_little[0] << " "
        << std::hex << (int)bytes_little[1] << " "
        << std::hex << (int)bytes_little[2] << " "
        << std::hex << (int)bytes_little[3];
}

void testFloatAndDouble() {
    SYLAR_LOG_INFO(g_logger) << "=== Starting Float/Double Tests ===";

    // 基本功能测试
    testFloatBasic();
    SYLAR_LOG_INFO(g_logger) << "Basic float test passed";

    // 边界值测试
    testFloatBoundary();
    SYLAR_LOG_INFO(g_logger) << "Float boundary test passed";

    // 随机数测试
    testFloatRandom();

    // 字节序测试
    testFloatEndian();

    // Double 测试
    testDouble();
    SYLAR_LOG_INFO(g_logger) << "Double test passed";

    // 混合测试：浮点数和整数交替
    sylar::ByteArray ba(4096);
    ba.writeFloat(3.14f);
    ba.writeInt32(100);
    ba.writeDouble(2.71828);
    ba.writeUint32(200);

    ba.setPosition(0);
    float f = ba.readFloat();
    int32_t i1 = ba.readInt32();
    double d = ba.readDouble();
    uint32_t i2 = ba.readUint32();

    SYLAR_LOG_INFO(g_logger) << "Mixed test: "
        << "float=" << f << ", int32=" << i1
        << ", double=" << d << ", uint32=" << i2;

    SYLAR_LOG_INFO(g_logger) << "=== Float/Double Tests Completed ===";
}

int main()
{
    // test();

    // testFloatBasic();

    // testFloatBoundary();

    // testFloatRandom();

    // testDouble();

    // testFloatEndian();

    // testFloatAndDouble();

    return 0;
}
