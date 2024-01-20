//
// File: worker.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <any>

#include "am/string/string.h"
#include "am/system/ptr.h"
#include "rage/atl/array.h"
#include "am/types.h"

#include <functional>
#include <mutex>
#include <Windows.h>

namespace rageam
{
	enum eBackgroundTaskState
	{
		TASK_STATE_PENDING,
		TASK_STATE_RUNNING,
		TASK_STATE_SUCCESS,
		TASK_STATE_FAILED,
	};

	/**
	 * \brief Dummy structure for task without return value.
	 */
	struct BackgroundTaskEmptyResult {};

	/**
	 * \brief State of background task.
	 */
	class BackgroundTask
	{
		friend class BackgroundWorker;

		std::atomic<eBackgroundTaskState> m_State;

	protected:

		// This value must be set before returning from worker function
		std::any m_Result;

	public:
		eBackgroundTaskState GetState() const { return m_State; }

		bool IsSuccess() const  { return m_State == TASK_STATE_SUCCESS; }
		bool IsFinished() const { return m_State == TASK_STATE_SUCCESS || m_State == TASK_STATE_FAILED; }

		void Wait() const { while (!IsFinished()) { /* ... */ } }

		// This value can be safely accessed if IsSuccess returns True.
		template<typename T>
		T& GetResult()
		{
			AM_ASSERTS(IsSuccess());
			return std::any_cast<T&>(m_Result);
		}

		std::function<void()> UserDelegate;
	};
	using BackgroundTaskPtr = amPtr<BackgroundTask>;
	using Tasks = List<BackgroundTaskPtr>;

	/**
	 * \brief Dispatcher of long-running background tasks.
	 */
	class BackgroundWorker
	{
		using TLambda = std::function<bool()>;

		class BackgroundJob
		{
			static constexpr u32 TASK_NAME_MAX = 256;

			amPtr<BackgroundTask> m_Task;
			TLambda               m_Lambda;
			wchar_t               m_Name[TASK_NAME_MAX];

		public:
			BackgroundJob(amPtr<BackgroundTask> task, TLambda lambda, ConstWString name)
				: m_Task(std::move(task)), m_Lambda(std::move(lambda))
			{
				String::Copy(m_Name, TASK_NAME_MAX, name);
			}

			amPtr<BackgroundTask>& GetTask() { return m_Task; }
			TLambda&               GetLambda() { return m_Lambda; }
			ConstWString           GetName() const { return m_Name; }
		};

		static constexpr u32 MAX_BACKGROUND_THREADS = 16;

		static HANDLE                       sm_ThreadPool[];
		static List<amUPtr<BackgroundJob>>  sm_Jobs;
		static std::mutex                   sm_Mutex;
		static std::condition_variable      sm_Condition;
		static inline std::atomic_bool      sm_WeAreClosing = false;
		static inline bool                  sm_Initialized = false;
		static inline thread_local std::any tl_Result; // Per-worker unique result value, set from lambda function

		static DWORD ThreadProc(LPVOID lpParam);
		static amPtr<BackgroundTask> RunVA(const TLambda& lambda, ConstWString fmt, va_list args);
		static void Init();

	public:
		static void Shutdown();

		WPRINTF_ATTR(2, 3) static amPtr<BackgroundTask> Run(const TLambda& lambda, ConstString  fmt, ...);
		PRINTF_ATTR(2, 3)  static amPtr<BackgroundTask> Run(const TLambda& lambda, ConstWString fmt, ...);
		static amPtr<BackgroundTask>                    Run(const TLambda& lambda);

		/**
		 * \brief Pauses current thread until given list of tasks ran to completion state (either TASK_STATE_SUCCESS or TASK_STATE_FAILED).
		 * \return True if all tasks were finished with TASK_STATE_SUCCESS or False if either of the tasks was finished with TASK_STATE_FAILED.
		 */
		static bool WaitFor(const Tasks& tasks);

		// Call it from lambda function to set BackgroundTask::GetResult, must be allocated via operator new
		template<typename T>
		static void SetCurrentResult(const T& object) { tl_Result = object; }

		static inline std::function<void(const wchar_t*)> TaskCallback; // Used for UI status bar
	};
}
