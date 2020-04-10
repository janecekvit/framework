/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2018 Vit janecek <mailto:janecekvit@gmail.com>.

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

CThreadPool.h
Purpose: header file of static thread pool class

@author: Vit Janecek
@mailto: janecekvit@gmail.com
@version 1.05 01/03/2018
*/

#pragma once
#include <list>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>


class ThreadPool
{
	class Worker
	{
	public:
		Worker(ThreadPool& oParentPool);
		virtual ~Worker();

	private:
		void _Work();

	private:
		ThreadPool& m_oParentPool; //dependency
		std::thread m_oThread;
	};

public:
	ThreadPool(const size_t uiPoolSize);
	virtual ~ThreadPool();
	void AddTask(const std::function<void()> &fn);

	std::mutex m_mxtQueueLock;
	std::queue<std::function<void()>> m_queueTask;
	std::condition_variable m_cvPoolEvent;
	std::atomic<bool> m_bEndFlag = false;

private:
	std::list<Worker> m_ListOfWorkers;
};