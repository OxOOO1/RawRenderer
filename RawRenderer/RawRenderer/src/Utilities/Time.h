#pragma once
#include <chrono>
#include <string>

#include "Types.h"
#include "CommandContext.h"

class Time
{
public:
	friend class SApplication;

	static Time& Get()
	{
		static Time time;
		return time;
	}

	float GetDeltaTimeMs() const noexcept
	{
		return dTime * 1000.f;
	}

	float GetDeltaTimeSec() const noexcept
	{
		return dTime;
	}

	float GetTimeAfterLaunchMs()
	{
		return std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start).count() * 1000.f;
	}
	float GetTimeAfterLaunchSec()
	{
		return std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start).count();
	}
protected:
	void OnUpdate();
private:
	Time();
	std::chrono::steady_clock::time_point last;
	std::chrono::time_point<std::chrono::steady_clock> start;
	float dTime = 1.f;
};


class SystemTime
{
public:

	// Query the performance counter frequency
	static void Initialize(void);

	// Query the current value of the performance counter
	static int64_t GetCurrentTick(void);

	static void BusyLoopSleep(float SleepTime);

	static inline double TicksToSeconds(int64_t TickCount)
	{
		return TickCount * sm_CpuTickDelta;
	}

	static inline double TicksToMillisecs(int64_t TickCount)
	{
		return TickCount * sm_CpuTickDelta * 1000.0;
	}

	static inline double TimeBetweenTicks(int64_t tick1, int64_t tick2)
	{
		return TicksToSeconds(tick2 - tick1);
	}

private:

	// The amount of time that elapses between ticks of the performance counter
	static double sm_CpuTickDelta;
};


class RCpuTimer
{
public:

	RCpuTimer()
	{
		m_StartTick = 0ll;
		m_ElapsedTicks = 0ll;
	}

	void Start()
	{
		if (m_StartTick == 0ll)
			m_StartTick = SystemTime::GetCurrentTick();
	}

	void Stop()
	{
		if (m_StartTick != 0ll)
		{
			m_ElapsedTicks += SystemTime::GetCurrentTick() - m_StartTick;
			m_StartTick = 0ll;
		}
	}

	void Reset()
	{
		m_ElapsedTicks = 0ll;
		m_StartTick = 0ll;
	}

	double GetTime() const
	{
		return SystemTime::TicksToSeconds(m_ElapsedTicks);
	}

	void PrintTime(const std::string& ProcessName);

	void StopAndPrintTime(const std::string& ProcessName)
	{
		Stop();
		Reset();
		PrintTime(ProcessName);
	}

	void BeginFrame()
	{
		Start();
	}

	void EndFrame()
	{
		Stop();
		DeltaTime = SystemTime::TicksToMillisecs(m_ElapsedTicks);
		Reset();
	}

	static double GetDeltaTimeMs();

private:

	int64_t m_StartTick;
	int64_t m_ElapsedTicks;

	static double DeltaTime;
};




namespace RGpuTimeManager
{
	void Initialize(uint32_t MaxNumTimers = 4096);
	void Shutdown();

	// Reserve a unique timer index
	uint32_t NewTimer(void);

	// Write start and stop time stamps on the GPU timeline
	void StartTimer(RCommandList& Context, UINT TimerIdx);
	void StopTimer(RCommandList& Context, UINT TimerIdx);

	// Bookend all calls to GetTime() with Begin/End which correspond to Map/Unmap.  This
	// needs to happen either at the very start or very end of a frame.
	void BeginReadBack(void);
	void EndReadBack(void);

	// Returns the time in milliseconds between start and stop queries
	float GetTime(uint32_t TimerIdx);
}


class RGpuTimer
{
public:

	RGpuTimer()
	{
		m_TimerIndex = RGpuTimeManager::NewTimer();
	}

	void Start(RCommandList& Context)
	{
		RGpuTimeManager::StartTimer(Context, m_TimerIndex);
	}

	void Stop(RCommandList& Context)
	{
		RGpuTimeManager::StopTimer(Context, m_TimerIndex);
	}

	/*float GetTime(void)
	{
		return RGpuTimeManager::GetTime(m_TimerIndex);
	}*/

	float GetFrameTime()
	{
		float time;
		RGpuTimeManager::BeginReadBack();
		time = RGpuTimeManager::GetTime(m_TimerIndex);
		RGpuTimeManager::EndReadBack();
		return time;
	}

	uint32_t GetTimerIndex(void)
	{
		return m_TimerIndex;
	}
private:

	uint32_t m_TimerIndex;
};
