#include "JobSystem.h"
#include "Engine/Core/EngineCommon.hpp"

Job::Job(uint32_t jobType)
    : m_jobType(jobType)
{
}

JobWorkerThread::JobWorkerThread(int id, uint32_t type, JobSystem* system)
: m_threadID(id), m_workerType(type), m_jobSystem(system)
{
    m_thread = new std::thread(&JobWorkerThread::ThreadMain, this);
}

JobWorkerThread::~JobWorkerThread()
{
    m_isQuitting = true;
    
    if (m_thread && m_thread->joinable())
    {
        m_thread->join();
        delete m_thread;
        m_thread = nullptr;
    }
}

void JobWorkerThread::Join()
{
    if (m_thread && m_thread->joinable())
    {
        m_thread->join();
    }
}

void JobWorkerThread::ThreadMain()
{
    while (!m_jobSystem->m_isQuitting && !m_isQuitting)
    {
        Job* job = nullptr;

		{
			std::unique_lock<std::mutex> lk(m_jobSystem->m_jobQueueMutex);
			m_jobSystem->m_workAvailable.wait(lk, [this] {
				return m_jobSystem->m_isQuitting || m_isQuitting || !m_jobSystem->m_pendingJobs.empty();
				});
			if (m_jobSystem->m_isQuitting || m_isQuitting)
				break;

			// 找到我能干的 job，移出 pending，放到 executing
			for (auto it = m_jobSystem->m_pendingJobs.begin(); it != m_jobSystem->m_pendingJobs.end(); ++it)
			{
				if (CanExecuteJob(*it))
				{
					job = *it;
					m_jobSystem->m_pendingJobs.erase(it);
					m_jobSystem->m_executingJobs.push_back(job);
					break;
				}
			}
		}

		if (!job)
		{
			// dont sleep with lock!
			//std::this_thread::sleep_for(std::chrono::milliseconds(1));
			std::this_thread::yield();
			continue;
		}

		//no lock!
		try 
        {
			job->Execute();

		}
		catch (...) 
        {
		}
		{
			std::lock_guard<std::mutex> lk(m_jobSystem->m_jobQueueMutex);
			auto it = std::find(m_jobSystem->m_executingJobs.begin(), m_jobSystem->m_executingJobs.end(), job);
			if (it != m_jobSystem->m_executingJobs.end())
			{
				m_jobSystem->m_executingJobs.erase(it);
				m_jobSystem->m_completedJobs.push_back(job);
			}
		}
		// m_jobSystem->m_workAvailable.notify_all(); 
	}
	//g_theDevConsole->AddLine(Rgba8::CYAN,
	//	Stringf("Thread %d: Executing job", m_threadID));
}

bool JobWorkerThread::CanExecuteJob(Job* job)
{
    return (job->m_jobType & m_workerType) != 0;
}

JobSystem* g_theJobSystem = nullptr;

JobSystem::JobSystem(JobSystemConfig config)
{
    m_numWorkerThreads = config.m_numWorkerThreads;
    m_numIOThreads = config.m_numIOThreads;
}

JobSystem::~JobSystem()
{
}

void JobSystem::Startup()
{
    m_isQuitting = false;
    
    for (int i = 0; i < m_numWorkerThreads; i++)
    {
        JobWorkerThread* worker = new JobWorkerThread(i, JOB_TYPE_WORKER, this);
        m_workerThreads.push_back(worker);
    }
    
    for (int i = 0; i < m_numIOThreads; i++)
    {
        JobWorkerThread* worker = new JobWorkerThread(
            m_numWorkerThreads + i, JOB_TYPE_IO, this);
        m_workerThreads.push_back(worker);
    }
}

void JobSystem::Shutdown()
{
    m_isQuitting = true;
    
    m_workAvailable.notify_all();
    
    for (JobWorkerThread* worker : m_workerThreads)
    {
        worker->Join();
        delete worker;
    }
    m_workerThreads.clear();

    std::lock_guard<std::mutex> lock(m_jobQueueMutex);
    
    for (Job* job : m_pendingJobs)
    {
        delete job;
    }
    for (Job* job : m_executingJobs)
    {
        delete job;
    }
    for (Job* job : m_completedJobs)
    {
        delete job;
    }
        
    m_pendingJobs.clear();
    m_executingJobs.clear();
    m_completedJobs.clear();
}

void JobSystem::AddPendingJob(Job* job)
{
    std::scoped_lock lock(m_jobQueueMutex);
    m_pendingJobs.push_back(job);

    m_workAvailable.notify_one();
}

std::vector<Job*> JobSystem::RetrieveCompletedJobs()
{
    std::vector<Job*> result;
    
    std::lock_guard<std::mutex> lock(m_jobQueueMutex);
    result.swap(m_completedJobs);  // 高效交换，避免复制
    
    return result;
}

Job* JobSystem::RetrieveOneCompletedJob()
{
    std::lock_guard<std::mutex> lock(m_jobQueueMutex);
    
    if (m_completedJobs.empty())
    {
        return nullptr;
    }
    
    Job* job = m_completedJobs.back();
    m_completedJobs.pop_back();
    return job;
}

void JobSystem::PrintDebugInfo()
{
	//std::lock_guard<std::mutex> lock(m_jobQueueMutex);

	g_theDevConsole->AddLine(Rgba8::WHITE,
		Stringf("Threads: %d active", (int)m_workerThreads.size()));
	g_theDevConsole->AddLine(Rgba8::YELLOW,
		Stringf("Jobs - Pending: %d, Executing: %d, Completed: %d",
			(int)m_pendingJobs.size(),
			(int)m_executingJobs.size(),
			(int)m_completedJobs.size()));
}

int JobSystem::GetPendingJobCount() const
{
    std::lock_guard<std::mutex> lock(m_jobQueueMutex);
    return (int)m_pendingJobs.size();
}

int JobSystem::GetExecutingJobCount() const
{
    std::lock_guard<std::mutex> lock(m_jobQueueMutex);
    return (int)m_executingJobs.size();
}

int JobSystem::GetCompletedJobCount() const
{
    std::lock_guard<std::mutex> lock(m_jobQueueMutex);
    return (int)m_completedJobs.size();
}

int JobSystem::GetPendingAndExecutingJobCount() const
{
    std::lock_guard<std::mutex> lock(m_jobQueueMutex);
    return (int)(m_pendingJobs.size() + m_executingJobs.size());
}

int JobSystem::GetNumWorkerThreads() const
{
	return m_numWorkerThreads;
}

void JobSystem::SetQuitting(bool isQuitting)
{
    m_isQuitting = isQuitting;
}

bool JobSystem::IsQuitting() const
{
    return m_isQuitting;
}

