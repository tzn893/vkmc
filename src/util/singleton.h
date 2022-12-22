//tag a class as singleton object
class Is_Signleton 
{
public:
	Is_Signleton() {}
	Is_Signleton(const Is_Signleton&) = delete;
	void operator=(const Is_Signleton&) = delete;
};

template<typename T>
class Singleton
{
public:
	static T& Get()
	{
		static ptr<T> singleton = nullptr;
		
		static_assert(std::is_base_of_v<Is_Signleton, T>,"only class derived from Is_Signleton can get from signleton");

		if (singleton == nullptr)
		{
			singleton = ptr<T>(new T());
		}

		return *singleton;
	}
};