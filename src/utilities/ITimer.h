#pragma once
#include <timeapi.h>

/**
 *	A high-resolution timer.
 */
class ITimer
{
public:
	ITimer();
	~ITimer();

	static void Init(void);
	static void DeInit(void);

	void Start(void);

	double GetElapsedTime(void);  // seconds

private:
	uint64_t m_qpcBase;   // QPC
	uint32_t m_tickBase;  // timeGetTime

	static double s_secondsPerCount;
	static TIMECAPS s_timecaps;
	static bool s_setTime;

	// safe QPC stuff
	static uint64_t GetQPC(void);

	static uint64_t s_lastQPC;
	static uint64_t s_qpcWrapMargin;
	static bool s_hasLastQPC;

	static uint32_t s_qpcWrapCount;
	static uint32_t s_qpcInaccurateCount;
};
