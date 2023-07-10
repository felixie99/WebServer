#include <iostream>
#include <unistd.h>
#include "time_wheel.h"

void callback(client_data* data) {
    std::cout << "Timer callback triggered." << std::endl;
}

int main() {
    time_wheel tw;
    tw_timer* timer1 = tw.add_timer(5); // 添加一个定时器，超时时间为5个单位
    timer1->cb_func = callback; // 设置定时器回调函数
    client_data* data1 = new client_data;
    timer1->user_data = data1; // 设置定时器的用户数据

    tw_timer* timer2 = tw.add_timer(10); // 添加另一个定时器，超时时间为10个单位
    timer2->cb_func = callback;
    client_data* data2 = new client_data;
    timer2->user_data = data2;

    tw_timer* timer3 = tw.add_timer(15); // 添加第三个定时器，超时时间为15个单位
    timer3->cb_func = callback;
    client_data* data3 = new client_data;
    timer3->user_data = data3;

    tw_timer* timer4 = tw.add_timer(16); // 添加第三个定时器，超时时间为15个单位
    timer3->cb_func = callback;
    client_data* data4 = new client_data;
    timer4->user_data = data4;

    tw_timer* timer5 = tw.add_timer(18); // 添加第三个定时器，超时时间为15个单位
    timer3->cb_func = callback;
    client_data* data5 = new client_data;
    timer5->user_data = data5;

    tw_timer* timer6 = tw.add_timer(22); // 添加第三个定时器，超时时间为15个单位
    timer3->cb_func = callback;
    client_data* data6 = new client_data;
    timer6->user_data = data6;

    tw_timer* timer7 = tw.add_timer(25); // 添加第三个定时器，超时时间为15个单位
    timer3->cb_func = callback;
    client_data* data7 = new client_data;
    timer7->user_data = data7;

    tw_timer* timer8 = tw.add_timer(27); // 添加第三个定时器，超时时间为15个单位
    timer3->cb_func = callback;
    client_data* data8 = new client_data;
    timer8->user_data = data8;

    std::cout << "Start ticking..." << std::endl;
    for (int i = 0; i < 40; ++i) {
        sleep(2); // 模拟时间流逝，每次休眠1毫秒
        tw.tick(); // 时间轮前进一格
        if(i == 4)
            tw.adjust_timer(timer1);
    }

    delete data1;
    delete data2;
    delete data3;
    delete data4;
    delete data5;
    delete data6;
    delete data7;
    delete data8;

    return 0;
}
