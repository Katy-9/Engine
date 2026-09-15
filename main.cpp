#include <cstddef>
#include <new>
template<typename T>

class vector {
	T* beginPtr = nullptr;
	T* endPtr = nullptr;
	T* capacityPtr = nullptr;

	void Realloc(std::size_t newCapacity)
	{
		T* tmp = static_cast<T*>(::operator new(sizeof(T) * newCapacity));
		//T* temp = new T(newCapacity)
			for (std::size_t i = 0; i < Size(); ++i)
			{
				//tmp[i] = beginPtr[i];
				new (&tmp[i]) T(beginPtr[i]);
			}

		beginPtr = tmp;
		//delete[] beginPtr;
		::operator delete[](tmp);
	}
public:
	std::size_t Size() const
	{
		return endPtr - beginPtr;
	}

	std::size_t Capacity() const
	{
		return capacityPtr - beginPtr;
	}

	T& push_back(const T& data)
	{
		if (Size() >= Capacity())
			Realloc(1 + Capacity() * 2 );
		*(endPtr++) = data;

	}
};

#include <vector>

int main ()
{
	return 0;
}