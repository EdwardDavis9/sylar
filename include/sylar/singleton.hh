#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__
#include <memory>

namespace sylar {

template <class T, class X = void, int N = 0>
class Singleton {
  public:
    static T *GetInstance()
    {
        static T v;
        return &v;
    }
};

template <class T, class X = void, int N = 0>
class SingletonPtr {
  public:
    static std::shared_ptr<T> GetInstance()
    {
        // static std::shared_ptr<T> v(new T);
        static std::shared_ptr<T> v(std::make_shared<T>());
        return v;
    }
};

} // namespace sylar

#endif /* __SYLAR_SINGLETON_H__ */
