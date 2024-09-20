#include "threadpool.h"

#include <functional>
#include <thread>
#include <iostream>
const int TASK_MAX_THRESHHOLD = 1024;

//线程池构造
ThreadPool::ThreadPool()
	: initThreadSize_(0)
	, taskSize_(0)
	, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
{}

//线程池析构
ThreadPool::~ThreadPool()
{}

//设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode) 
{
	poolMode_ = mode;
}

/*//设置初始的线程数量
void ThreadPool::setInitThreadSize(int size)
{
	initThreadSize_ = size;
}*/

//设置任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	taskQueMaxThreshHold_ = threshhold;
}

//给线程池提交任务 用户调用接口传入任务对象，生产任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	//获取锁
	std::unique_lock<std::mutex>lock(taskQueMtx_);

	//线程的通信 等待任务队列有空余
	/*while (taskQue_.size() == taskQueMaxThreshHold_)
	{
		notFull_.wait(lock);
	}*/
	//用户提交任务最长不能阻塞超过1s，否则判断提交任务失败，返回
	if (!notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; })) 
	{
		//表示等待1S，依然没有满足
		std::cerr << "task queue is full,submit task fail." << std::endl;
		//return task->getResult();//Task Result 线程执行完task，task对象就被析构掉了
		return Result(sp,false);
	}

	//如果有空余放入队列
	taskQue_.emplace(sp);
	taskSize_++;

	//因为新放了任务，队列不空，notEmpty_上进行通知
	notEmpty_.notify_all();
	//返回任务的Result对象
	return Result(sp);//默认是true
}

//开启线程池
void ThreadPool::start(int initThreadSize)
{
	//记录初始线程个数
	initThreadSize_ = initThreadSize;

	//创建线程对象
	for (int i = 0; i < initThreadSize_; i++)
	{	//创建thread线程对象的时候，把线程函数给到thread线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		threads_.emplace_back(std::move(ptr));/*项目中遇到的第1个难点，怎么绑定的,绑定器*/
	}

	//启动所有线程 std::vector<Thread*> threads_;
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();
	}
}

//定义线程函数 线程池的所有线程从任务队列消费任务
void ThreadPool::threadFunc()
{
	/*std::cout << "begin threadFunc(开始) tid:" << std::this_thread::get_id()
		<<std::endl;
	
	std::cout << "end threadFunc(结束) tid:" << std::this_thread::get_id()
		<<std::endl;*/
	for (;;)
	{
		std::shared_ptr<Task> task;
		{
			//先获取锁
			std::unique_lock<std::mutex>lock(taskQueMtx_);

			std::cout << "tid;" << std::this_thread::get_id() << "尝试获取任务!" << std::endl;

			//等待notEmpty条件
			notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });

			std::cout << "tid;" << std::this_thread::get_id() << "获取任务成功!" << std::endl;

			//取一个任务出来
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			//如果依然有剩余任务，继续通知其他线程执行任务
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}

			//取出一个任务进行通知可以继续生产任务
			notFull_.notify_all();//有wait就有notify
		}//就应该释放锁
		
		//当前线程负责执行这个任务
		if(task != nullptr)//防止异常情况程序挂掉
		{
			//task->run();//执行任务，把任务返回值通过setVal方法给到Result
			task->exec();
		}
	}
}

/////////////////////////////线程方法实现

//线程构造函数
Thread::Thread(ThreadFunc func) 
	: func_(func)
{}
// 线程析构
Thread::~Thread(){}

//启动线程
void Thread::start()
{
	//创建一个线程来执行一个线程池函数
	std::thread t(func_);//C++11来说 线程对象t 和线程函数func_
	t.detach();//设置分离线程 pthread_detach pthread_t设置成分离线程

}

///////////////////////////Task方法实现
Task::Task()
	:result_(nullptr)
{}

void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setVal(run());//这里发生多态调用}
	}
}
void Task::setResult(Result* res)
{
	result_ = res;
}

////////////////////////////Result方法实现
Result::Result(std::shared_ptr<Task> task, bool isValid)
	:isValid_(isValid)
	,task_(task)
{
	task_->setResult(this);
}
Any Result::get()//用户调用
{
	if (!isValid_)
	{
		return "";
	}
	sem_.wait();//task任务如果没有执行完，这里会阻塞用户的线程
	return std::move(any_);
}

void Result::setVal(Any any)//谁调用的呢
{
	//存储task的返回值
	this->any_ = std::move(any);
	sem_.post();//已经获取的任务的返回值，增加信号量资源
}