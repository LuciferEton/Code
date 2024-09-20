#include "threadpool.h"

#include <functional>
#include <thread>
#include <iostream>
const int TASK_MAX_THRESHHOLD = 1024;

//�̳߳ع���
ThreadPool::ThreadPool()
	: initThreadSize_(0)
	, taskSize_(0)
	, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
{}

//�̳߳�����
ThreadPool::~ThreadPool()
{}

//�����̳߳صĹ���ģʽ
void ThreadPool::setMode(PoolMode mode) 
{
	poolMode_ = mode;
}

/*//���ó�ʼ���߳�����
void ThreadPool::setInitThreadSize(int size)
{
	initThreadSize_ = size;
}*/

//�����������������ֵ
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	taskQueMaxThreshHold_ = threshhold;
}

//���̳߳��ύ���� �û����ýӿڴ������������������
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	//��ȡ��
	std::unique_lock<std::mutex>lock(taskQueMtx_);

	//�̵߳�ͨ�� �ȴ���������п���
	/*while (taskQue_.size() == taskQueMaxThreshHold_)
	{
		notFull_.wait(lock);
	}*/
	//�û��ύ�����������������1s�������ж��ύ����ʧ�ܣ�����
	if (!notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; })) 
	{
		//��ʾ�ȴ�1S����Ȼû������
		std::cerr << "task queue is full,submit task fail." << std::endl;
		//return task->getResult();//Task Result �߳�ִ����task��task����ͱ���������
		return Result(sp,false);
	}

	//����п���������
	taskQue_.emplace(sp);
	taskSize_++;

	//��Ϊ�·������񣬶��в��գ�notEmpty_�Ͻ���֪ͨ
	notEmpty_.notify_all();
	//���������Result����
	return Result(sp);//Ĭ����true
}

//�����̳߳�
void ThreadPool::start(int initThreadSize)
{
	//��¼��ʼ�̸߳���
	initThreadSize_ = initThreadSize;

	//�����̶߳���
	for (int i = 0; i < initThreadSize_; i++)
	{	//����thread�̶߳����ʱ�򣬰��̺߳�������thread�̶߳���
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		threads_.emplace_back(std::move(ptr));/*��Ŀ�������ĵ�1���ѵ㣬��ô�󶨵�,����*/
	}

	//���������߳� std::vector<Thread*> threads_;
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();
	}
}

//�����̺߳��� �̳߳ص������̴߳����������������
void ThreadPool::threadFunc()
{
	/*std::cout << "begin threadFunc(��ʼ) tid:" << std::this_thread::get_id()
		<<std::endl;
	
	std::cout << "end threadFunc(����) tid:" << std::this_thread::get_id()
		<<std::endl;*/
	for (;;)
	{
		std::shared_ptr<Task> task;
		{
			//�Ȼ�ȡ��
			std::unique_lock<std::mutex>lock(taskQueMtx_);

			std::cout << "tid;" << std::this_thread::get_id() << "���Ի�ȡ����!" << std::endl;

			//�ȴ�notEmpty����
			notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });

			std::cout << "tid;" << std::this_thread::get_id() << "��ȡ����ɹ�!" << std::endl;

			//ȡһ���������
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			//�����Ȼ��ʣ�����񣬼���֪ͨ�����߳�ִ������
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}

			//ȡ��һ���������֪ͨ���Լ�����������
			notFull_.notify_all();//��wait����notify
		}//��Ӧ���ͷ���
		
		//��ǰ�̸߳���ִ���������
		if(task != nullptr)//��ֹ�쳣�������ҵ�
		{
			//task->run();//ִ�����񣬰����񷵻�ֵͨ��setVal��������Result
			task->exec();
		}
	}
}

/////////////////////////////�̷߳���ʵ��

//�̹߳��캯��
Thread::Thread(ThreadFunc func) 
	: func_(func)
{}
// �߳�����
Thread::~Thread(){}

//�����߳�
void Thread::start()
{
	//����һ���߳���ִ��һ���̳߳غ���
	std::thread t(func_);//C++11��˵ �̶߳���t ���̺߳���func_
	t.detach();//���÷����߳� pthread_detach pthread_t���óɷ����߳�

}

///////////////////////////Task����ʵ��
Task::Task()
	:result_(nullptr)
{}

void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setVal(run());//���﷢����̬����}
	}
}
void Task::setResult(Result* res)
{
	result_ = res;
}

////////////////////////////Result����ʵ��
Result::Result(std::shared_ptr<Task> task, bool isValid)
	:isValid_(isValid)
	,task_(task)
{
	task_->setResult(this);
}
Any Result::get()//�û�����
{
	if (!isValid_)
	{
		return "";
	}
	sem_.wait();//task�������û��ִ���꣬����������û����߳�
	return std::move(any_);
}

void Result::setVal(Any any)//˭���õ���
{
	//�洢task�ķ���ֵ
	this->any_ = std::move(any);
	sem_.post();//�Ѿ���ȡ������ķ���ֵ�������ź�����Դ
}