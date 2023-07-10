#include "time_wheel.h"
#include "../http/http_conn.h"
#include <mutex>

#define BUFFER_SIZE 64
//#define TIMESLOT 1

time_wheel::time_wheel() : cur_slot(0)
{
    for(int i = 0; i < N; i++)
    {
        slots[i] = NULL; //初始化每个槽的头结点
    }
}

time_wheel::~time_wheel()
{
    // 遍历每个槽，并销毁其中的定时器
    for(int i = 0; i < N; i++)
    {
        std::lock_guard<std::mutex> lock(mutexes[i]);
        tw_timer* tmp = slots[i];
        while(tmp)
        {
            slots[i] = tmp->next;
            delete tmp;
            tmp = slots[i];
        }
    }
}

/*根据定时值timeout创建一个定时器，并把她插入合适的槽中*/
tw_timer* time_wheel::add_timer(int timeout)
{
    if(timeout < 0)
    {
        return NULL;
    }
    int ticks = 0;
    /*
        根据待插入定时器的超时值计算它将在时间轮转动多少个滴答后被触发，并将该滴答数存储于变量ticks中。
        如果待插入定时器的超时值小于时间轮的槽间隔SI,则将ticks向上折合为1
        否则将ticks向下折合为 timeout / SI
    */
    if(timeout < SI)
    {
        ticks = 1;
    }
    else
    {
        ticks = timeout / SI;
    }

    int rotation = ticks / N; //计算待插入的定时器在时间轮转动多少圈后被触发
    int ts = (cur_slot + (ticks % N) ) % N; //计算待插入的定时器应该被插入哪个槽中
    std::lock_guard<std::mutex> lock(mutexes[ts]);
    
    tw_timer* timer = new tw_timer(rotation, ts);
    /* 如果第ts 个槽中尚无任何定时器，则吧新建的定时器插入其中，并将该定时器设置为该槽的头结点*/
    if(!slots[ts])
    {
        printf("add timer, rotation is %d, ts is %d, cur_slot is %d\n", rotation, ts, cur_slot);
        slots[ts] = timer;
    }
    else
    {
        timer->next = slots[ts];
        slots[ts]->prev = timer;
        slots[ts] = timer;
    }
    return timer;
}

/*根据定时值timeout 和 其他信息 创建一个定时器，并把她插入合适的槽中*/
tw_timer* time_wheel::add_timer(client_data* user_data, int timeout)
{
    if(timeout < 0)
    {
        return NULL;
    }
    int ticks = 0;
    /*
        根据待插入定时器的超时值计算它将在时间轮转动多少个滴答后被触发，并将该滴答数存储于变量ticks中。
        如果待插入定时器的超时值小于时间轮的槽间隔SI,则将ticks向上折合为1
        否则将ticks向下折合为 timeout / SI
    */
    if(timeout < SI)
    {
        ticks = 1;
    }
    else
    {
        ticks = timeout / SI;
    }

    int rotation = ticks / N; //计算待插入的定时器在时间轮转动多少圈后被触发
    int ts = (cur_slot + (ticks % N) ) % N; //计算待插入的定时器应该被插入哪个槽中
    std::lock_guard<std::mutex> lock(mutexes[ts]);

    tw_timer* timer = new tw_timer(rotation, ts);
    timer->user_data = user_data;
    timer->cb_func = cb_func;

    /* 如果第ts 个槽中尚无任何定时器，则吧新建的定时器插入其中，并将该定时器设置为该槽的头结点*/
    if(!slots[ts])
    {
        printf("add timer, rotation is %d, ts is %d, cur_slot is %d\n", rotation, ts, cur_slot);
        slots[ts] = timer;
    }
    else
    {
        timer->next = slots[ts];
        slots[ts]->prev = timer;
        slots[ts] = timer;
    }
    return timer;
}

void time_wheel::del_timer(tw_timer* timer)
{
    if(!timer)
    {
        return ;
    }
    int ts = timer->time_slot;
    std::lock_guard<std::mutex> lock(mutexes[ts]);
    /* slots[ts]是目标定时器所在槽的头结点。如果目标定时器就是该头结点，则需要重置第 ts 个槽的头结点*/
    if(timer == slots[ts])
    {
        slots[ts] = slots[ts]->next;
        if( slots[ts] )
        {
            slots[ts]->prev = NULL;
        }
        delete timer;
    }
    else
    {
        timer->prev->next = timer->next;
        if( timer->next )
        {
            timer->next->prev = timer->prev;
        }
        delete timer;
    }
}

/* SI时间到后， 调用该函数，时间轮向前滚动一个槽的间隔 */
void time_wheel::tick()
{
    tw_timer* tmp = slots[cur_slot];
    std::lock_guard<std::mutex> lock(mutexes[cur_slot]);
    printf("current slot is %d\n", cur_slot);

    while( tmp )
    {
        printf("tick the timer once\n");
        /* 如果定时器的rotation > 0 则它在这一轮不起作用 */
        if(tmp->rotation > 0)
        {
            tmp->rotation--;
            tmp = tmp->next;
        }
        else /* 否则，说明定时器已到期，于是执行定时器任务，然后删除该定时器*/ 
        {
            tmp->cb_func(tmp->user_data);
            if(tmp == slots[cur_slot])
            {
                printf("delete header in cur_slor\n");
                slots[cur_slot] = tmp->next;
                delete tmp;
                if(slots[cur_slot])
                {
                    slots[cur_slot]->prev = NULL;
                }
                tmp = slots[cur_slot];
            }
            else
            {
                tmp->prev->next = tmp->next;
                if(tmp->next)
                {
                    tmp->next->prev = tmp->prev;
                }
                tw_timer* tmp2 = tmp->next;
                delete tmp;
                tmp = tmp2;
            }
        }
    }

    cur_slot = ++cur_slot % N; /* 更新时间轮的当前槽，以反映时间轮的滚动 */
}

void time_wheel::adjust_timer(tw_timer* timer){
    if(!timer)
    {
        return ;
    }
    int ts = timer->time_slot;
    std::unique_lock<std::mutex> lock(mutexes[ts]);
    /* slots[ts]是目标定时器所在槽的头结点。如果目标定时器就是该头结点，则需要重置第 ts 个槽的头结点*/
    if(timer == slots[ts])
    {
        slots[ts] = slots[ts]->next;
        if( slots[ts] )
        {
            slots[ts]->prev = NULL;
        }
        //delete timer;
    }
    else
    {
        timer->prev->next = timer->next;
        if( timer->next )
        {
            timer->next->prev = timer->prev;
        }
        //delete timer;
    }
    lock.unlock();
    int ticks = 30 * m_TIMESLOT;
    int rotation = ticks / N; //计算待插入的定时器在时间轮转动多少圈后被触发
    ts = (cur_slot + (ticks % N) ) % N; //计算待插入的定时器应该被插入哪个槽中
    std::lock_guard<std::mutex> lock1(mutexes[ts]);

    timer->time_slot = ts;
    timer->rotation = rotation;
    //tw_timer* timer = new tw_timer(rotation, ts);
    /* 如果第ts 个槽中尚无任何定时器，则吧新建的定时器插入其中，并将该定时器设置为该槽的头结点*/
    if(!slots[ts])
    {
        printf("add timer, rotation is %d, ts is %d, cur_slot is %d\n", rotation, ts, cur_slot);
        slots[ts] = timer;
    }
    else
    {
        timer->next = slots[ts];
        slots[ts]->prev = timer;
        slots[ts] = timer;
    }
}


void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_time_wheel.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}

