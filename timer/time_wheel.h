#ifndef TIME_WHEEL_
#define TIME_WHEEL_

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <mutex>

#define BUFFER_SIZE 64
//#define TIMESLOT 1

class tw_timer;
struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[ BUFFER_SIZE ];
    tw_timer *timer;
};

class tw_timer
{
public:
    tw_timer(int rot, int ts)
        :next(NULL), prev(NULL), rotation(rot), time_slot(ts){}

public:
    int rotation; //记录定时器在时间轮转多少圈后生效
    int time_slot; //记录定时器属于时间轮上哪个槽
    void (*cb_func)(client_data*);
    client_data* user_data;
    tw_timer* next;
    tw_timer* prev;
};

class time_wheel
{
public:
    time_wheel();
    ~time_wheel();

    /*根据定时值timeout创建一个定时器，并把她插入合适的槽中*/
    tw_timer* add_timer(int timeout);
    tw_timer* add_timer(client_data* user_data, int timeout);
    void del_timer(tw_timer* timer);
    /* SI时间到后， 调用该函数，时间轮向前滚动一个槽的间隔 */
    void tick();

    void adjust_timer(tw_timer* timer);
private:
    static const int N = 60; //时间轮上槽的数目
    static const int SI = 1; //每 1s 时间轮转动一次，即槽间隔为1s
    tw_timer* slots[N]; // 时间轮的槽，其中每个元素指向一个定时器链表，链表无序
    std::mutex mutexes[N];
    int cur_slot; //时间轮的当前槽
    static const int m_TIMESLOT = 1;
};

class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

//对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    time_wheel m_time_wheel;
    static int u_epollfd;
    int m_TIMESLOT;    
};

void cb_func(client_data *user_data);
#endif