#ifndef SINGLETON_HPP
#define SINGLETON_HPP

template <typename T>
class Singleton
{
  public:
    inline static T* get();

  private:
    static T* m_obj;
};

template <typename T>
T* Singleton<T>::m_obj = nullptr;

template <typename T>
T* Singleton<T>::get()
{
  return m_obj;
}

#endif /* SINGLETON_HPP */
