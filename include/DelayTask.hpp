//
// Created by cc on 2019/09/04.
//

#ifndef QTALK_V2_NETWORKCHECKTASK_H
#define QTALK_V2_NETWORKCHECKTASK_H

#include <functional>
#include <thread>
#include <mutex>
#include <memory>

/**
 * 延时任务
 * 1 _delay 延时时间
 * 2 _call_fun 回调函数 bool 返回值 true:继续延时_delay false:取消任务
 * 3
 */
class DelayTask {

public:
    explicit DelayTask(int delay, std::string name, std::function<bool()> fun)
        :_call_fun(std::move(fun)), _delay(delay), _name(std::move(name))
    {

    }

    void start()
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);

            if(_run)
                return;

            if(_thread)
                delete _thread;
        }

        update();
        //
        run();
    }

    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _run = false;
        }

        const std::chrono::milliseconds ms(1000);
        std::this_thread::sleep_for(ms);

        delete _thread;
        _thread = nullptr;
    }

    void update()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _execute_time = time(0) * 1000 + _delay;
    }

    void setDelay(int delay)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _delay = delay;
        _execute_time = time(0) * 1000 + _delay;
    }

    bool isRuning()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _run;
    }

protected:
    void run()
    {
//        setThreadName("delayTask thread");
        _run = true;
        _thread = new std::thread([this](){

            while (_run)
            {
                {
                    bool call  = false;
                    {
                        std::lock_guard<std::mutex> lock(_mutex);
                        call = time(0) * 1000 > _execute_time;
                    }
                    if(_run && call)
                    {
                        if(_run && _call_fun())
                            update();
                        else
                        {
                            _run = false;
                            break;
                        }
                    }

                }

                const std::chrono::milliseconds ms(1000);
                std::this_thread::sleep_for(ms);
            }
        });
        _thread->detach();
    }


private:
    mutable std::mutex _mutex;

    std::function<bool()> _call_fun;

    std::thread *_thread = nullptr;

    bool _run = false;
    int  _delay = 0;

    long long _execute_time = 0;

    std::string _name;
};

#endif //QTALK_V2_NETWORKCHECKTASK_H
