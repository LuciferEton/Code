#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

//Any���ͣ����Խ����������ݵ�����
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	template<typename T>
	Any(T data):base_(std::make_unique<Derive<T>>(data))
	{}
	//��������ܰ�Any��������洢��data���ݶ�����ȡ����
	template<typename T>
	T cast_()
	{
	//������ô��base_�ҵ���ָ���Derive���󣬴�������ȡ��data��Ա����
		//����ָ��ת��������ָ��
		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd == nullptr)
		{
			throw"type is unmatch!";
		}
		return pd->data_;
	}
private:
	//��������
	class Base
	{
	public:
		virtual ~Base() = default;
	};
	//����������
	template<typename T>
	class Derive : public Base
	{
	public:
		Derive(T data):data_(data)
		{}
		T data_;
	};
private:
	//����һ�������ָ��
	std::unique_ptr<Base> base_;
};

//ʵ��һ���ź�����
class Semaphore
{
public:
	Semaphore(int limit = 0)
		:resLimit_(limit)
	{}
	~Semaphore() = default;

	//��ȡһ���ź�����Դ
	void wait()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		//�ȴ��ź�������Դ��û����Դ�Ļ�����������ǰ�߳�
		cond_.wait(lock, [&]()->bool {return resLimit_ > 0; });
		resLimit_--;
	}

	//����һ���ź�����Դ
	void post()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		resLimit_++;
		cond_.notify_all();
	}
private:
	int resLimit_;
	std::mutex mtx_;
	std::condition_variable cond_;
};
//Taskǰ������
class Task;
//ʵ�ֽ����ύ���̳߳ص�task����ִ����ɺ�ķ���ֵ����Result
class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;

	//setVal��������ȡ����ִ����ķ���ֵ
	void setVal(Any any);
	//get�������û��������������ȡtask�ķ���ֵ
	Any get();
private:
	Any any_;//�洢���񷵻�ֵ
	Semaphore sem_;//�߳�ͨ���ź���
	std::shared_ptr<Task> task_;//ָ���Ӧ��ȡ����ֵ���������
	std::atomic_bool isValid_;//����ֵ�Ƿ���Ч
};


//����������
//�û������Զ��������������ͣ���Task�̳У���дrun����
//ʵ���Զ���������
class Task
{
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);
	//�û������Զ����������ͣ���Task�̳У���дrun������ʵ���Զ���������
	virtual Any run() = 0;

private:
	Result* result_;//Result������������ڳ���Task��
};

//�̳߳�֧�ֵ�ģʽ
enum class PoolMode
{
	MODE_FIXED,//�̶��������߳�
	MODE_CACHER,//�߳������ɶ�̬����
};

//�߳�����
class Thread
{
public:
	//�̺߳�����������
	using ThreadFunc = std::function<void()>;

	//�̹߳��캯��
	Thread(ThreadFunc func);
	// �߳�����
	~Thread();
	//�����߳�
	void start();
private:
	ThreadFunc func_;
};
/*
* example:
* ThreadPool pool;
* pool.start(4);
* 
* clsaa MyTask : public Task
* {
* public:
* void run(){//�̴߳���}
* }
* 
* pool.submitTask(std::make_shared<Mytask>());
*/
//�̳߳�����
class ThreadPool
{
public:
	//�̳߳ع���
	ThreadPool();

	//�̳߳�����
	~ThreadPool();

	//�����̳߳صĹ���ģʽ
	void setMode(PoolMode mode);

	/*//���ó�ʼ���߳�����
	void setInitThreadSize(int size);*/

	//�����������������ֵ
	void setTaskQueMaxThreshHold(int threshhold);

	//���̳߳��ύ����
	Result submitTask(std::shared_ptr<Task> sp);

	//�����̳߳�
	void start(int initThreadSize = 4);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool operator=(const ThreadPool&) = delete;

private:
	//�����̺߳���
	void threadFunc();

private:
	std::vector<std::unique_ptr<Thread>> threads_;//�߳��б�
	int initThreadSize_;//��ʼ���߳�����
	
	std::queue<std::shared_ptr<Task>> taskQue_;//�������
	std::atomic_int taskSize_;//���������
	int taskQueMaxThreshHold_;//�����������������ֵ

	std::mutex taskQueMtx_;//��֤������е��̰߳�ȫ
	std::condition_variable notFull_;//��ʾ������в���
	std::condition_variable notEmpty_;//��ʾ������в���

	PoolMode poolMode_;//��ǰ�̳߳ع���ģʽ
};

#endif
