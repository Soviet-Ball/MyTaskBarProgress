#ifndef MYTASKBARPROGRESS_H
#define MYTASKBARPROGRESS_H
#include <QObject>
#include <QWidget>
#include <QEvent>
#include <QShowEvent>
#include <QTimer>
#ifdef Q_OS_WIN
#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
#ifdef Win_Extra_Enabled
#include <QtWinExtras>
#include <QtWin>
#include <qwinfunctions.h>
#endif
#endif
#if (QT_VERSION > QT_VERSION_CHECK(6,0,0))
#endif
#include <Windows.h>
#include <processthreadsapi.h>
#include <qt_windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <Ole2.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <Psapi.h>
#include <fcntl.h>
#include <io.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "DbgHelp.lib")
#endif

class MyTaskBarProgress : QObject
{
    Q_OBJECT
public:
    explicit MyTaskBarProgress(QWidget *parent = nullptr) : QObject(parent)
    {
        if(parent != nullptr && static_cast<QWidget*>(parent) != nullptr)
        {
            wid = static_cast<QWidget*>(parent)->winId();
            parent->installEventFilter(this);
        }
#ifdef Q_OS_WIN
        CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, reinterpret_cast<void**>(&taskbar));
#endif
    }
    bool eventFilter(QObject *watched, QEvent *event)
    {
        if(watched == QWidget::find(wid))
        {
            if(event->type() == QEvent::Show && !this->isHidden())
            {
                this->refresh();
            }
        }
        return QObject::eventFilter(watched,event);
    }
#ifdef Q_OS_WIN
    static bool isAvailable() {return true;}
#else
    static bool isAvailable() {return false;}
#endif
    enum State
    {
        Normal,
        Paused,
        Stopped,
        Busy,
        NoProgress
    };
    void setState(State state)
    {
        emit stateChanged(s,state);
        s = state;
#ifdef Q_OS_WIN
        HWND hwnd = HWND(wid);
        std::function f = [=](){
            if(s == Normal)
                taskbar->SetProgressState(hwnd,TBPF_NORMAL);
            else if(s == Paused)
                taskbar->SetProgressState(hwnd,TBPF_PAUSED);
            else if(s == Stopped)
                taskbar->SetProgressState(hwnd,TBPF_ERROR);
            else if(s == Busy)
                taskbar->SetProgressState(hwnd,TBPF_INDETERMINATE);
            else if(s == NoProgress)
                taskbar->SetProgressState(hwnd,TBPF_NOPROGRESS);
        };
        int i = 0;
        while(i < 10)
        {
            QTimer::singleShot(i*10,f);
            i++;
        }

#endif
        hidden = false;
    }
    State state() {return s;}
    void setWindowHandle(WId w)
    {
        wid = w;
        QWidget* widget = QWidget::find(wid);
        if(widget != nullptr)
        {
            widget->installEventFilter(this);
        }
    }
    WId windowHandle() {return wid;}
    void setMaximum(int max)
    {
        if(max < 0)
            max = 0;
        if(max < minValue)
            max = minValue;
        maxValue = max;
        emit maximumChanged(maxValue);
    }
    int maximum() {return maxValue;}
    void setMinimum(int min)
    {
        if(min < 0)
            min = 0;
        if(min > maxValue)
            min = maxValue;
        minValue = min;
        emit minimumChanged(minValue);
    }
    int minimum() {return minValue;}
    void setRange(int min, int max)
    {
        if(min < 0)
            min = 0;
        if(min > max)
            std::swap<int>(min,max);
        maxValue = max;
        minValue = min;
        emit maximumChanged(maxValue);
        emit minimumChanged(minValue);
    }
    void setValue(int value)
    {
        if(value > maxValue)
            value = maxValue;
        else if(value < minValue)
            value = minValue;
        v = value;
        int length = maxValue - minValue;
        int progress = v - minValue;
#ifdef Q_OS_WIN
        if(!hidden)
            taskbar->SetProgressValue(HWND(wid),progress,length);
#endif
        emit valueChanged(v);
    }
    int value() {return v;}
    void hide()
    {
#ifdef Q_OS_WIN
        taskbar->SetProgressState(HWND(wid),TBPF_NOPROGRESS);
#endif
        hidden = true;
    }
    void show()
    {
        this->refresh();
        hidden = false;
    }
    bool isHidden() {return hidden;}
signals:
    void maximumChanged(int maximum);
    void minimumChanged(int minimum);
    void stateChanged(State oldstate,State newstate);
    void valueChanged(int value);
private slots:
    void refresh()
    {
#ifdef Q_OS_WIN
        taskbar = nullptr;
        CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, reinterpret_cast<void**>(&taskbar));
#endif
        QWidget* widget = QWidget::find(wid);
        if(widget != nullptr)
        {
            this->setState(this->state());
            this->setRange(this->minimum(),this->maximum());
            this->setValue(this->value());
        }
    }
private:
    WId wid = 0;
    State s;
    int minValue = 0;
    int maxValue = 0;
    int v = -1;
    bool hidden = false;
#ifdef Q_OS_WIN
    ITaskbarList3* taskbar = nullptr;
#endif
};

#endif // MYTASKBARPROGRESS_H
