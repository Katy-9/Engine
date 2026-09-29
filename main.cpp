#include <new>
#include <assert.h>
template<typename T>
class Vector {
    T* beginPtr;
    T* endPtr;
    T* capacityPtr;
    void Realloc(std::size_t newCapacity) {
        T* newMemory = static_cast<T*>(::operator new(newCapacity * sizeof(T)));
        const std::size_t size = Size();
        for (std::size_t i = 0; i < size; ++i)
        {
            new (newMemory + i) T(static_cast<T&&>(beginPtr[i]));
            beginPtr[i].~T();
        }
        //for (T* ptr = beginPtr; ptr != endPtr; ++ptr)
        //    ptr->~T();
        ::operator delete(beginPtr);
        beginPtr = newMemory;
        endPtr = beginPtr + size;
        capacityPtr = beginPtr + newCapacity;
    }
    void Copy(const Vector& other) {
        beginPtr = static_cast<T*>(::operator new(other.Capacity() * sizeof(T)));
        endPtr = beginPtr;
        capacityPtr = beginPtr + other.Capacity();
        for (const T& data : other) {
            new (endPtr++) T(data);
        }
    }
public:
    Vector() : beginPtr(nullptr), endPtr(nullptr), capacityPtr(nullptr) {}
    std::size_t Size() const {
        return endPtr - beginPtr;
    }
    Vector(const Vector& other) {
        Copy(other);
    }
    Vector(Vector&& other) noexcept : beginPtr(other.beginPtr), endPtr(other.endPtr), capacityPtr(other.capacityPtr) {
        other.beginPtr = nullptr;
        other.endPtr = nullptr;
        other.capacityPtr = nullptr;
    }
    ~Vector() {
        Clear();
        ::operator delete(beginPtr);
    }
    Vector& operator=(const Vector& other) {
        Clear();
        ::operator delete(beginPtr);
        Copy(other);
        return *this;
    }
    Vector& operator=(Vector&& other) {
        Clear();
        ::operator delete(beginPtr);
        beginPtr = other.beginPtr;
        other.beginPtr = nullptr;
        endPtr = other.endPtr;
        other.endPtr = nullptr;
        capacityPtr = other.capacityPtr;
        other.capacityPtr = nullptr;
        return *this;
    }
    void Reserve(std::size_t rSize) {
        if (Capacity() < rSize)
            Realloc(rSize);
    }
    bool Empty() const {
        return beginPtr == endPtr;
    }
    void Clear() {
        for (T* ptr = beginPtr; ptr != endPtr; ++ptr)
            ptr->~T();
        endPtr = beginPtr;
    }
    std::size_t Capacity() const {
        return capacityPtr - beginPtr;
    }
    T& push_back(const T& element) {
        if (Size() >= Capacity())
            Realloc(1 + Capacity() * 2);
        return *(new (endPtr++) T(element));
    }
    template<typename... Arg>
    T& emplace_back(Arg&&...args) {
        if (Size() >= Capacity())
            Realloc(1 + Capacity() * 2);
        return *(new (endPtr++) T(static_cast<Arg&&>(args)...));
    }
    void pop_back() {
        (--endPtr)->~T();
    }
    T& operator[](std::size_t idx) {
        return beginPtr[idx];
    }
    const T& operator[](std::size_t idx) const {
        return beginPtr[idx];
    }
    T& at(std::size_t idx) {
        assert(Size() > idx && "Index out of bounds");
        return beginPtr[idx];
    }
    const T& at(std::size_t idx) const {
        assert(Size() > idx && "Index out of bounds");
        return beginPtr[idx];
    }
    T* begin() {
        return beginPtr;
    }
    T* end() {
        return endPtr;
    }
    const T* begin() const {
        return beginPtr;
    }
    const T* end() const {
        return endPtr;
    }
};

#include <iostream>
int main() {
    Vector<int> a;
    a.emplace_back(2);
    a.push_back(3);
    std::cout << "Size: " << a.Size() << std::endl;
    a.pop_back();
    Vector<int> c(static_cast<Vector<int>&&>(a));
    std::cout << "Size: " << a.Size() << std::endl;
    for (const int& data : c)
        std::cout << data << std::endl;
    return 0;
}