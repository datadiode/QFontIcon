#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QKeyEvent>
#include <QProgressBar>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>

#include "automator.h"
#include <awesome.h>

#define QT_STATICPLUGIN
#include <qfonticon.h>
#ifdef QT_STATICPLUGIN
class QStaticFontIconPlugin : public QFontIconPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QIconEngineFactoryInterface_iid FILE "qfonticon.json");
};
Q_IMPORT_PLUGIN(QStaticFontIconPlugin)
#include "main.moc"
#endif

/*
 * Like QMetaEnum::keyToValue() but uses binary rather than linear search.
 * Key-value pairs must be sorted by key, like produced by generate_fa.py.
 */
static int keyToValue(const QMetaEnum& metaEnum, const char* key, bool* ok = nullptr)
{
    int lower = 0;
    int upper = metaEnum.keyCount();
    while (lower < upper)
    {
        int const match = (upper + lower) >> 1;
        int const cmp = strcmp(metaEnum.key(match), key);
        if (cmp >= 0)
            upper = match;
        if (cmp <= 0)
            lower = match + 1;
    }
    if (bool okay = (ok ? *ok : okay) = lower > upper) // it's cool man
        return metaEnum.value(upper);
    return -1;
}

class KeystrokeAutomator : public Automator<KeystrokeAutomator>
{
public:
    using Automator<KeystrokeAutomator>::Automator;
    template<bool init>
    void step()
    {
        switch (state)
        {
        case NULL:
            state = AFTER(250)
            {
                qApp->postEvent(QApplication::activeWindow(), new QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier));
            }
            state = AFTER(250)
            {
                qApp->postEvent(QApplication::activeWindow(), new QKeyEvent(QEvent::KeyRelease, Qt::Key_Tab, Qt::NoModifier));
            }
            state = AFTER(250)
            {
                qApp->postEvent(QApplication::focusWidget(), new QKeyEvent(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier));
            }
            state = AFTER(250)
            {
                qApp->postEvent(QApplication::focusWidget(), new QKeyEvent(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier));
            }
            state = NULL;
        }
    }
};

#undef NDEBUG
#include <cassert>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Resolve a conflict between a static and a dynamic plugin in favor of the former.
    qt_static_plugin_QStaticFontIconPlugin().instance()->setParent(&app);

    app.setStyleSheet("QPushButton { font-size: 25px; padding: 10px; }");
    app.setApplicationName("QFontIcon");

    for (auto metaEnum : { QMetaEnum::fromType<fa::v5::codepoints>(), QMetaEnum::fromType<fa::v6::codepoints>() })
    {
        for (int i = 0; i < metaEnum.keyCount(); ++i)
        {
            auto key = metaEnum.key(i);
            auto value = metaEnum.value(i);
            assert(metaEnum.keyToValue(key) == keyToValue(metaEnum, key));
        }
    }

#if 1
    // Preferred method to load the font and register the codepoint dictionary when linking against the library.
    QFontIconEngine::loadFont(":/fonts/fa-solid-900.ttf#fa-solid.ttf", QMetaEnum::fromType<fa::v5::codepoints>());
#else
    // Hackish method to load the font and register the codepoint dictionary when loading the plugin dynamically.
    QIconMetaEnum(":/fonts/fa-solid-900.ttf#fa-solid.ttf") = QMetaEnum::fromType<fa::v5::codepoints>();
#endif

    QMainWindow w;
    QStatusBar* status = new QStatusBar();
    QProgressBar* progress = new QProgressBar(status);
    progress->setFixedWidth(300);
    progress->setFormat("Step %v of %m");
    status->addPermanentWidget(progress);
    w.setStatusBar(status);

    KeystrokeAutomator automator(progress, false);
    automator.callOnTimeout([&progress] { progress->hide(); });
    automator.start();

    QVBoxLayout* layout = new QVBoxLayout();

    // a simple animated beer button
    {
        QPushButton* beerButton = new QPushButton("Cheers!");
        beerButton->setFixedHeight(75);
        beerButton->setEnabled(false);

        auto engine = new QFontIconEngine("beer", "fa-solid.ttf");
        engine->setSpeed(180);
        engine->setWidget(beerButton);
        engine->setCurve(QEasingCurve::InOutCubic);

        beerButton->setIcon(QIcon(engine));
        beerButton->setIconSize(QSize(50, 50));

        layout->addWidget(beerButton);
    }

    // a multi species button
    {
        QPushButton* catdogButton = new QPushButton("Meowoof!");
        catdogButton->setFixedHeight(75);
        catdogButton->setCheckable(true);

        catdogButton->setIcon(QIcon("fa-solid?0x11.codepoint=cat&0x44.codepoint=dog&0x01=deepskyblue&0x10=orange&0x04=blue&0x40=yellow#.ttf"));
        catdogButton->setIconSize(QSize(50, 50));
        layout->addWidget(catdogButton);
    }

    // a simple toggle button
    {
        QPushButton* toggleButton = new QPushButton("Toggle Me!");
        toggleButton->setFixedHeight(75);
        toggleButton->setCheckable(true);

        auto engine = new QFontIconEngine();
        engine->setFont("fa-solid.ttf");
        engine->setIcon(fa::v6::toggle_on, QIcon::Normal, QIcon::On);
        engine->setColor(Qt::green, QIcon::Normal, QIcon::On);

        engine->setIcon(fa::v6::toggle_off, QIcon::Normal, QIcon::Off);
        engine->setColor(Qt::red, QIcon::Normal, QIcon::Off);

        toggleButton->setIcon(QIcon(engine));
        toggleButton->setIconSize(QSize(50, 50));

        layout->addWidget(toggleButton);
    }

    // add the samples
    QWidget* samples = new QWidget();
    samples->setLayout(layout);
    w.setContentsMargins(5, 5, 5, 5);
    w.setCentralWidget(samples);

    w.show();

    return app.exec();
}
