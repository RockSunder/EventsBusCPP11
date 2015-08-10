#pragma once
#include <vector>
#include <list>
#include <map>
#include <functional>
#include <algorithm>

using namespace std;
using namespace std::placeholders;

class EventBus
{
public:

	template<class C, class ...Args>
	static void subscribe(const C* aReceiver, Args ...aArgs)
	{
		subscribeInternal0(aReceiver, aArgs...);
	}

	template<class C>
	static void unsubscribe(const C* aReceiver)
	{
		IReceiver* receiver = s_Receivers[(void*)aReceiver];
		if (receiver == 0)
			return;
		delete receiver;
		s_Receivers.erase((void*)aReceiver);
	}

	template<class T>
	static void send(const T& aEvent)
	{
		ObserversCollection<T>::send(aEvent);
	}


	template<class C, class ...Args>
	static void produce(const C* aProducer, Args ...aArgs)
	{
		produceInternal0(aProducer, aArgs...);
	}

	template<class C>
	static void unproduce(const C* aProducer)
	{
		IProducer* producer = s_Producers[(void*)aProducer];
		if (producer == 0)
			return;
		delete producer;
		s_Producers.erase((void*)aProducer);
	}

private:

	class IReceiver{public:	virtual ~IReceiver(){};};
	class IObserver{public:	virtual ~IObserver(){};};
	
	class IProducer{public:	virtual ~IProducer(){};};
	class IProducerEvent{public: virtual ~IProducerEvent(){};};

	//Container for typed event observers
	template<class T>
	class ObserversCollection
	{
		typedef function<void(const T&)> Observer_t;
		typedef function<T()> ProducerEvent_t;
	public:
		static void add(Observer_t* aObserver)
		{
			if (aObserver == 0)
				return;
			s_Observers.push_back(aObserver);
			for (unsigned i = 0; i < s_Producers.size(); i++)
			{
				if (s_Producers[i] == 0)
					continue;
				(*aObserver)((*s_Producers[i])());
			}
		}

		static void send(const T& aEvent)
		{
			for (unsigned i = 0; i < s_Observers.size(); i++)
			{
				if (s_Observers[i] == 0)
					continue;
				(*s_Observers[i])(aEvent);
			}
		}

		static void remove(Observer_t* aObserver)
		{
			for (unsigned i = 0; i < s_Observers.size(); i++)
			{
				if (s_Observers[i] == aObserver)
					s_Observers[i] = 0;
			}
		}

		static void repair()
		{
			s_Observers.erase(std::remove(s_Observers.begin(), s_Observers.end(), (Observer_t*)0), s_Observers.end());
		}

		static void addProducer(ProducerEvent_t* aProducer)
		{
			if (aProducer == 0)
				return;
			s_Producers.push_back(aProducer);
		}

		static void removeProducer(ProducerEvent_t* aProducer)
		{
			for (unsigned i = 0; i < s_Producers.size(); i++)
			{
				if (s_Producers[i] == aProducer)
					s_Producers[i] = 0;
			}
		}

		static void repairProducers()
		{
			s_Producers.erase(std::remove(s_Producers.begin(), s_Producers.end(), (ProducerEvent_t*)0), s_Producers.end());
		}
	private:
		static vector<Observer_t*> s_Observers;
		static vector<ProducerEvent_t*> s_Producers;
	};


	//Container for object that receives event
	template<class T>
	class Receiver : public IReceiver
	{
	public:
		virtual ~Receiver()
		{
			for (unsigned i = 0; i < m_Observers.size(); i++)
			{
				delete m_Observers[i];
			}
		};

		void addObserver(IObserver* aObserver)
		{
			m_Observers.push_back(aObserver);
		}
	private:
		vector<IObserver*> m_Observers;
	};


	//Container for event handling function
	template<class C, class T>
	class Observer : public IObserver
	{
		typedef function<void(const T&)> Observer_t;
	public:
		Observer(const C* aObject, void(C::*aMember)(const T&))
			:m_Observer(std::forward<Observer_t>(bind(aMember, (C*)aObject, _1)))
		{
			ObserversCollection<T>::add(&m_Observer);
			s_Count++;
		}

		virtual ~Observer()
		{
			ObserversCollection<T>::remove(&m_Observer);
			s_Count--;
			if (s_Count <= 0)
			{
				ObserversCollection<T>::repair();
				s_Count = 0;
			}
		};
	private:
		Observer_t m_Observer;
		static unsigned int s_Count;
	};


	//Container for events producer
	template<class C>
	class Producer : IProducer
	{
	public:

		void addProducerEvent(IProducerEvent* aProducerEvent)
		{
			m_Events.push_back(aProducerEvent);
		}

		virtual ~Producer()
		{
			for (unsigned i = 0; i < m_Events.size(); i++)
			{
				delete m_Events[i];
			}
		}
	private:
		vector<IProducerEvent*> m_Events;
	};

	//Container for producing event function
	template<class C, class T>
	class ProducerEvent : public IProducerEvent
	{
		typedef function<T()> ProducerEvent_t;
	public:
		ProducerEvent(const C* aObject, T(C::*aMember)())
			:m_Producer(std::forward<ProducerEvent_t>(bind(aMember, (C*)aObject)))
		{
			ObserversCollection<T>::addProducer(&m_Producer);
			s_Count++;
		}

		virtual ~ProducerEvent()
		{
			ObserversCollection<T>::removeProducer(&m_Producer);
			s_Count--;
			if (s_Count <= 0)
			{
				ObserversCollection<T>::repairProducers();
				s_Count = 0;
			}
		}
	private:
		ProducerEvent_t m_Producer;
		static unsigned int s_Count;
	};



	template<class C, class ...Args>
	static void subscribeInternal0(const C* aReceiver, Args ...aArgs)
	{
		Receiver<C>* receiver = (Receiver<C>*)s_Receivers[(void*)aReceiver];
		if (receiver == 0)
		{
			receiver = new Receiver<C>();
			s_Receivers[(void*)aReceiver] = (IReceiver*)receiver;
		}
		subscribeInternal1(receiver, aReceiver, aArgs...);
	}

	template<class C, class T, class ...Args>
	static void subscribeInternal1(Receiver<C>* aReceiver, const C* aObject, void(C::*aMember)(const T&), Args ...aArgs)
	{
		IObserver* observer = (IObserver*)new Observer<C, T>(aObject, aMember);
		aReceiver->addObserver(observer);

		subscribeInternal1(aReceiver, aObject, aArgs...);
	}

	template<class C, class ...Args>
	static void produceInternal0(const C* aProducer, Args ...aArgs)
	{
		Producer<C>* producer = (Producer<C>*)s_Producers[(void*)aProducer];
		if (producer == 0)
		{
			producer = new Producer<C>();
			s_Producers[(void*)aProducer] = (IProducer*)producer;
		}
		produceInternal1(producer, aProducer, aArgs...);
	}

	template<class C, class T, class ...Args>
	static void produceInternal1(Producer<C>* aProducer, const C* aObject, T(C::*aMember)(), Args ...aArgs)
	{
		IProducerEvent* producerEvent = (IProducerEvent*)new ProducerEvent<C, T>(aObject, aMember);
		aProducer->addProducerEvent(producerEvent);

		produceInternal1(aProducer, aObject, aArgs...);
	}


	template<class C>
	static void subscribeInternal1(Receiver<C>* aReceiver, const C* aObject){}
	template<class C>
	static void produceInternal1(Producer<C>* aProducer, const C* aObject){}

	static map<void*, IReceiver*> s_Receivers;
	static map<void*, IProducer*> s_Producers;
};

map<void*, EventBus::IReceiver*> EventBus::s_Receivers = map<void*, EventBus::IReceiver*>();
map<void*, EventBus::IProducer*> EventBus::s_Producers = map<void*, EventBus::IProducer*>();

template<class T>
vector<function<void(const T&)>*> EventBus::ObserversCollection<T>::s_Observers = vector<function<void(const T&)>*>();
template<class T>
vector<function<T()>*> EventBus::ObserversCollection<T>::s_Producers = vector<function<T()>*>();


template<class C, class T>
unsigned int EventBus::Observer<C, T>::s_Count = 0;

template<class C, class T>
unsigned int EventBus::ProducerEvent<C, T>::s_Count = 0;
