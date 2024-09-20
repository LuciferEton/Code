#include"threadpool.h"
#include<chrono>
#include<thread>
#include<iostream>

class MyTask : public Task
{
public:
	MyTask(int begin,int end):begin_(begin),end_(end){}
	//问题一:怎么设计run函数的返回值可以表示任意类型
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
	//如何设计这里Result机制呢
	for (int i = 0; i <= 100; i++) 
	{
	Result res = pool.submitTask(std::make_shared<MyTask>(0, 100));
	}
	//std::cout << res.get().cast_<int>() << std::endl;//get返回了一个Any类型，怎么转成具体的类型呢？
	getchar();
}