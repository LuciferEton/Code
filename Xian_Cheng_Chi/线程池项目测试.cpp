#include"threadpool.h"
#include<chrono>
#include<thread>
#include<iostream>

class MyTask : public Task
{
public:
	MyTask(int begin,int end):begin_(begin),end_(end){}
	//����һ:��ô���run�����ķ���ֵ���Ա�ʾ��������
	Any run()
	{
		std::cout << "tid;" << std::this_thread::get_id() << "begin!" << std::endl;
		//std::this_thread::sleep_for(std::chrono::seconds(2));
		int sum = 0;
		for (int i = begin_; i <= end_; i++) 
		{
			sum += i;
			std::cout << sum << std::endl;
		}
		std::cout << "tid;" << std::this_thread::get_id() << "end!" << std::endl;
		return sum;
	}
private:
	int begin_;
	int end_;
};

int main()
{
	ThreadPool pool;
	pool.start(4);
	//����������Result������
	for (int i = 0; i <= 100; i++) 
	{
	Result res = pool.submitTask(std::make_shared<MyTask>(0, 100));
	}
	//std::cout << res.get().cast_<int>() << std::endl;//get������һ��Any���ͣ���ôת�ɾ���������أ�
	getchar();
}