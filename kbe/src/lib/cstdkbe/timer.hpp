/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2012 kbegine Software Ltd
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
same license as the rest of the engine.
*/

#ifndef __TIMER__
#define __TIMER__
#include "cstdkbe/cstdkbe.hpp"
#include "cstdkbe/timestamp.hpp"
#include "helper/debug_helper.hpp"

namespace KBEngine
{
class TimesBase;
class TimeBase;

class TimerHandle
{
public:
	explicit TimerHandle(TimeBase * pTime = NULL) : pTime_( pTime ) {}

	void cancel();
	void clearWithoutCancel()	{ pTime_ = NULL; }

	bool isSet() const		{ return pTime_ != NULL; }

	friend bool operator==( TimerHandle h1, TimerHandle h2 );
	TimeBase * time() const	{ return pTime_; }

private:
	TimeBase * pTime_;
};

inline bool operator==( TimerHandle h1, TimerHandle h2 )
{
	return h1.pTime_ == h2.pTime_;
}


/**
 *	This is an interface which must be derived from in order to
 *	receive time queue events.
 */
class TimerHandler
{
public:
	TimerHandler() : numTimesRegistered_( 0 ) {}
	virtual ~TimerHandler()
	{
		KBE_ASSERT( numTimesRegistered_ == 0 );
	};

	virtual void handleTimeout(TimerHandle handle, void * pUser) = 0;

protected:
	virtual void onRelease( TimerHandle handle, void * pUser ) {}

private:
	friend class TimeBase;
	void incTimerRegisterCount() { ++numTimesRegistered_; }
	void decTimerRegisterCount() { --numTimesRegistered_; }
	void release( TimerHandle handle, void * pUser )
	{
		this->decTimerRegisterCount();
		this->onRelease( handle, pUser );
	}

	int numTimesRegistered_;
};

class TimeBase
{
public:
	TimeBase(TimesBase &owner, TimerHandler * pHandler, void * pUserData);
	void cancel();
	void * getUserData()const	{ return pUserData_; }
	bool isCancelled()const{ return state_ == TIME_CANCELLED; }
	bool isExecuting()const{ return state_ == TIME_EXECUTING; }
protected:
	enum TimeState
	{
		TIME_PENDING,
		TIME_EXECUTING,
		TIME_CANCELLED
	};

	TimesBase& owner_;
	TimerHandler * pHandler_;
	void *pUserData_;
	TimeState state_;
};

class TimesBase
{
public:
	virtual void onCancel() = 0;
};

template<class TIME_STAMP>
class TimesT : public TimesBase
{
public:
	typedef TIME_STAMP TimeStamp;

	TimesT();
	~TimesT();
	
	inline uint32 size() const	{ return timeQueue_.size(); }
	inline bool empty() const	{ return timeQueue_.empty(); }
	
	int	process(TimeStamp now);
	bool legal( TimerHandle handle ) const;
	TIME_STAMP nextExp( TimeStamp now ) const;
	void clear( bool shouldCallCancel = true );
	
	bool getTimerInfo( TimerHandle handle, TimeStamp& time, TimeStamp&	interval,
					void *&	pUser ) const;
	
	TimerHandle	add(TimeStamp startTime, TimeStamp interval,
						TimerHandler* pHandler, void * pUser);
	
	TIME_STAMP timerDeliveryTime(TimerHandle handle) const;
	TIME_STAMP timerIntervalTime(TimerHandle handle) const;
	TIME_STAMP& timerIntervalTime(TimerHandle handle);
private:
	
	typedef std::vector<KBEngine::TimeBase *> Container;
	Container container_;

	void purgeCancelledTimes();
	void onCancel();

	class Time : public TimeBase
	{
	public:
		Time( TimesBase & owner, TimeStamp startTime, TimeStamp interval,
			TimerHandler * pHandler, void * pUser );

		TIME_STAMP time() const			{ return time_; }
		TIME_STAMP interval() const		{ return interval_; }
		TIME_STAMP &intervalRef()		{ return interval_; }

		TIME_STAMP deliveryTime() const;

		void triggerTimer();

	private:
		TimeStamp			time_;
		TimeStamp			interval_;

		Time( const Time & );
		Time & operator=( const Time & );
	};

	class Comparator
	{
	public:
		bool operator()(const Time* a, const Time* b)
		{
			return a->time() > b->time();
		}
	};
	
	class PriorityQueue
	{
	public:
		typedef std::vector<Time *> Container;

		typedef typename Container::value_type value_type;
		typedef typename Container::size_type size_type;

		bool empty() const				{ return container_.empty(); }
		size_type size() const			{ return container_.size(); }

		const value_type & top() const	{ return container_.front(); }

		void push( const value_type & x )
		{
			container_.push_back( x );
			std::push_heap( container_.begin(), container_.end(),
					Comparator() );
		}

		void pop()
		{
			std::pop_heap( container_.begin(), container_.end(), Comparator() );
			container_.pop_back();
		}

		Time * unsafePopBack()
		{
			Time * pTime = container_.back();
			container_.pop_back();
			return pTime;
		}

		Container & container()		{ return container_; }

		void heapify()
		{
			std::make_heap( container_.begin(), container_.end(),
					Comparator() );
		}

	private:
		Container container_;
	};
	
	PriorityQueue	timeQueue_;
	Time * 			pProcessingNode_;
	TimeStamp 		lastProcessTime_;
	int				numCancelled_;

	TimesT( const TimesT & );
	TimesT & operator=( const TimesT & );

};

}

#ifdef CODE_INLINE
#include "timer.ipp"
#endif
#endif // __TASKS__
