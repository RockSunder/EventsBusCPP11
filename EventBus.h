/*
 * The MIT License (MIT)

 * Copyright (c) 2017 Sergey Dzhaltyr

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _EVENT_BUS_PP_H_
#define _EVENT_BUS_PP_H_

#include <vector>
#include <unordered_map>
#include <algorithm>

using namespace std;

class EventBus
{
public:
    template<class TListener, class ...THandlers>
    static void subscribe(TListener* listener, THandlers ...handlers)
    {
        subscribePrivate0(listener, handlers...);
    }

    template<class TListener>
    static void unsubscribe(TListener* listener)
    {
        unsubscribePrivate0(listener);
    }

    template<class TEvent>
    static void send(const TEvent& event)
    {
        HandlersCollection<TEvent>::sendEvent(event);
    }

    template<class TProducer, class ...TGenerators>
    static void registerProducer(TProducer* producer, TGenerators ...generators)
    {
        registerProducerPrivate0(producer, generators...);
    }

    template<class TProducer>
    static void unregisterProducer(TProducer* producer)
    {
        unregisterProducerPrivate0(producer);
    }

private:
    template<class TObject>
    struct BasicCollection
    {
        void add(TObject object)
        {
            objects.push_back(object);
        }

        void remove(TObject object)
        {
            vector<TObject>::iterator iter = find(objects.begin(), objects.end(), object);
            if (iter != objects.end())
            {
                swap(*iter, objects.back());
                objects.pop_back();
            }
        }

        vector<TObject>    objects;
    };


    struct IListener { virtual ~IListener(){} };
    struct IEventHandler { virtual ~IEventHandler(){} };

    template<class TEvent>
    class EventHandlerBase : public IEventHandler
    {
    public:
        virtual void       send(const TEvent&) = 0;
    };

    template<class TObject>
    class Listener : public IListener
    {
    public:
        virtual ~Listener()
        {
            for (IEventHandler* handler : m_eventHandlers)
            {
                delete handler;
            }
        }

        void addEventHandler(IEventHandler* eventHandler)
        {
            if (find(m_eventHandlers.begin(), m_eventHandlers.end(), eventHandler) == m_eventHandlers.end())
            {
                m_eventHandlers.push_back(eventHandler);
            }
        }

    private:
        vector<IEventHandler*> m_eventHandlers;
    };

    template<class TObject, class TEvent>
    class EventHandler final : public EventHandlerBase<TEvent>
    {
        typedef void(TObject::*TEventHandler)(const TEvent&);
    public:
        EventHandler(TObject* object, TEventHandler handler)
            : m_listener(object)
            , m_handler(handler)
        {
            HandlersCollection<TEvent>::addHandler(this);
        }

        virtual ~EventHandler()
        {
            HandlersCollection<TEvent>::removeHandler(this);
        };

        void send(const TEvent& event) override
        {
            (m_listener->*m_handler)(event);
        }

    private:

        TObject* m_listener;
        TEventHandler m_handler;
    };

    template<class TEvent>
    class HandlersCollection final
    {
    public:
        typedef EventHandlerBase<TEvent> TEventHandler;

        static void addHandler(TEventHandler* handler)
        {
            s_handlers.add(handler);
        }

        static void removeHandler(TEventHandler* handler)
        {
            s_handlers.remove(handler);
        }

        static void sendEvent(const TEvent& event)
        {
            for (TEventHandler* handler : s_handlers.objects)
            {
                if (handler)
                {
                    handler->send(event);
                }
            }
        }

    private:
        static BasicCollection<TEventHandler*> s_handlers;
    };


    struct IProducer { virtual ~IProducer(){} };
    struct IEventGenerator { virtual ~IEventGenerator(){} };

    template<class TObject>
    class Producer : public IProducer
    {
    public:
        virtual ~Producer()
        {
            for (IEventGenerator* generator : m_eventGenerators)
            {
                delete generator;
            }
        }

        void addEventGenerator(IEventGenerator* eventGenerator)
        {
            if (find(m_eventGenerators.begin(), m_eventGenerators.end(), eventGenerator) == m_eventGenerators.end())
            {
                m_eventGenerators.push_back(eventGenerator);
            }
        }

    private:
        vector<IEventHandler*> m_eventGenerators;
    };

    template<class TObject, class ...THandlers>
    static void subscribePrivate0(TObject* listnerObject, THandlers ...handlers)
    {
        Listener<TObject>* listener = static_cast<Listener<TObject>*>(s_objectObserversMap[listnerObject]);
        if (!listener)
        {
            listener = new Listener<TObject>();
            s_objectObserversMap[listnerObject] = listener;
        }

        subscribePrivate1(listener, listnerObject, handlers...);
    }

    template<class TObject, class TEvent, class ...THandlers>
    static void subscribePrivate1(Listener<TObject>* listener, TObject* listnerObject, void(TObject::*handler)(const TEvent&), THandlers ...handlers)
    {
        IEventHandler* eventHandler = new EventHandler<TObject, TEvent>(listnerObject, handler);
        listener->addEventHandler(eventHandler);
        // TODO: call producers

        subscribePrivate1(listener, listnerObject, handlers...);
    }

    template<class TObject>
    static void unsubscribePrivate0(TObject* object)
    {
        IListener* listener = s_objectObserversMap[object];
        if (listener == 0)
        {
            return;
        }
        delete listener;
        s_objectObserversMap.erase(object);
    }

    template<class TObject, class ...TGenerators>
    static void registerProducerPrivate0(TObject* object, TGenerators ...generators)
    {
        Producer<TObject>* producer = static_cast<Producer<TObject>*>(s_objectProducersMap[object]);
        if (!producer)
        {
            producer = new Producer<TObject>();
            s_objectProducersMap[object] = producer;
        }

        registerProducerPrivate1(producer, object, generators...);
    }

    template<class TObject, class TEvent, class ...TGenerators>
    static void registerProducerPrivate1(Producer<TObject>* producer, TObject* object, TEvent(TObject::*generator)(), TGenerators ...generators)
    {
        

        registerProducerPrivate1(producer, object, generators...);
    }

    template<class TObject>
    static void unregisterProducerPrivate0(TObject* object)
    {
        IProducer* producer = s_objectProducersMap[object];
        if (producer == 0)
        {
            return;
        }
        delete producer;
        s_objectProducersMap.erase(object);
    }

    template<class TObject> static void subscribePrivate1(Listener<TObject>* , TObject* ) {}
    template<class TObject> static void registerProducerPrivate1(Producer<TObject>*, TObject*) {}

    static unordered_map<void*, IListener*> s_objectObserversMap;
    static unordered_map<void*, IProducer*> s_objectProducersMap;
};

template<class TEvent>
EventBus::BasicCollection<EventBus::EventHandlerBase<TEvent>*> EventBus::HandlersCollection<TEvent>::s_handlers = EventBus::BasicCollection<EventBus::EventHandlerBase<TEvent>*>();

unordered_map<void*, EventBus::IListener*> EventBus::s_objectObserversMap = unordered_map<void*, EventBus::IListener*>();
unordered_map<void*, EventBus::IProducer*> EventBus::s_objectProducersMap = unordered_map<void*, EventBus::IProducer*>();

#endif //_EVENT_BUS_PP_H_
