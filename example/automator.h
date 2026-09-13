#pragma once

#include <QTimer>
#include <QProgressBar>

#define AFTER(delay) \
    __LINE__; if constexpr(init) ++count, total += delay; else \
    start(delay); break; case __LINE__: if constexpr(init); else

template<typename T>
class Automator : public QTimer
{
public:
    Automator(QProgressBar* progress, bool singleShot)
        : progress(progress)
    {
        setSingleShot(singleShot);
        do
        {
            static_cast<T*>(this)->template step<true>();
        } while (state != NULL);
        progress->setRange(0, count);
    }

private:
    QProgressBar* const progress;

protected:
    int state = NULL;
    int count = 0; // number of states in the automator cycle
    int total = 0; // total time of the automator cycle

    void timerEvent(QTimerEvent* event) override
    {
        do
        {
            static_cast<T*>(this)->template step<false>();
        } while (!isSingleShot() && state == NULL);
        progress->setValue(progress->value() % count + 1);
        if (state == NULL)
            QTimer::timerEvent(event);
    }
};
