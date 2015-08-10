##C++11 Events Bus
<br>
####Usage sample:
```c++
struct ResInfo
{
	ResInfo(int w, int h):w(w),h(h){}
	int w, h;
};

class ClassA
{
public:
	ClassA()
	{
		EventBus::produce(this, &ClassA::stringEventProducer);
		EventBus::subscribe(this, &ClassA::stringEvent, &ClassA::resolutionEvent);
	}
	
	~ClassA()
	{
		EventBus::unproduce(this);
		EventBus::unsubscribe(this);
	}
	
	void stringEvent(const std::string& aString)
	{
		printf("String Event: %s\n", aString.c_str());
	}
	
	void resolutionEvent(const ResInfo& aResolution)
	{
		printf("Resolution Event: %d, %d\n", aResolution.w, aResolution.h);
	}

	std::string stringEventProducer()
	{
		return "ClassA::stringEventProducer()";
	}
};

void main()
{
	ClassA c;
	EventBus::send(std::string("Hello world"));
	EventBus::send(ResInfo(999, 999));
}
```
<br>
####Output:<br>
```
--> String Event: ClassA::stringEventProducer()
--> String Event: Hello world
--> Resolution Event: 999, 999
```
