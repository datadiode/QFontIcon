#include <qfonticon.h>

#include <QApplication>
#include <QAtomicInteger>
#include <QMap>
#include <QMetaEnum>
#include <QPainterPath>
#include <QPainter>
#include <QRawFont>
#include <QStyle>
#include <QTimer>
#include <QWidget>

template<typename T>
class StateMap : public QMap<QPair<QIcon::Mode, QIcon::State>, T>
{
public:
    typedef QPair<QIcon::Mode, QIcon::State> Key;

public:
    using QMap<Key, T>::QMap;
    using QMap<Key, T>::operator=;

    T get(Key k, const T& defaultValue = T()) const
    {
        typename StateMap::const_iterator it;
        while ((it = this->find(k)) == this->end())
        {
            if (k.first != QIcon::Normal)
                k.first = QIcon::Normal;
            else if (k.second != QIcon::Off)
                k.second = QIcon::Off;
            else
                return defaultValue;
        }
        return it.value();
    }

    T get(QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off, const T& defaultValue = T()) const
    {
        return get(Key{mode, state}, defaultValue);
    }

    void set(const T& value, const Key& k) { this->insert(k, value); }

    void set(const T& value, QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off)
    {
        set(value, {mode, state});
    }
};



// =============================================================================



class QFontIconEnginePrivate
{
public:
    ~QFontIconEnginePrivate();

    void setupTimer();

    StateMap<int> icons;
    StateMap<QString> fonts;
    StateMap<qreal> scales;
    StateMap<QColor> colors;
    StateMap<qreal> speeds;
    StateMap<QEasingCurve> curves;

    QWidget* widget = nullptr;

    bool badge = false;

    static QTimer timer;
    static QAtomicInt timerRefCount;
    QMetaObject::Connection timerConnection;
    StateMap<qreal> progress;
    StateMap<qreal> angles;

    static QMap<QString, QPair<QRawFont, QMetaEnum>> availableFonts;
    static QRawFont getFont(const QString& font);
    static QMetaEnum getEnum(const QString& font);
    static void resizeFont(const QSizeF& size, QRawFont& font, qreal scale, quint32 glyphIndex);
    static int keyToValue(const QMetaEnum& metaEnum, const char* key, bool* ok = nullptr);
};

QTimer QFontIconEnginePrivate::timer;
QAtomicInt QFontIconEnginePrivate::timerRefCount;

QFontIconEnginePrivate::~QFontIconEnginePrivate()
{
    if (QObject::disconnect(timerConnection) && --timerRefCount == 0)
        timer.stop();
}

void QFontIconEnginePrivate::setupTimer()
{
    if (QObject::disconnect(timerConnection) && --timerRefCount == 0)
        timer.stop();

    if (!widget)
        return;

    if (std::none_of(speeds.begin(), speeds.end(), [](qreal s){ return s > 0; }))
        return;

    if (timerRefCount++ == 0)
        timer.start(20);

    timerConnection = QObject::connect(&timer, &QTimer::timeout, [this]()
    {
        StateMap<qreal> new_progress;
        StateMap<qreal> new_angles;

        for (auto it = speeds.begin(); it != speeds.end(); ++it)
        {
            if (it.value() == 0)
                continue;

            // Speed is in degrees per seconds
            qreal p = progress.get(it.key(), 0);
            p = fmod(p + it.value() / 1000.0 * timer.interval() / 360.0, 1.0);
            new_progress.set(p, it.key());

            auto c = curves.get(it.key());
            qreal a = c.valueForProgress(p) * 360.0;
            new_angles.set(a, it.key());

            widget->update();
        }

        angles.swap(new_angles);
        progress.swap(new_progress);
    });
}

QMap<QString, QPair<QRawFont, QMetaEnum>> QFontIconEnginePrivate::availableFonts;

QRawFont QFontIconEnginePrivate::getFont(const QString& font)
{
    QRawFont rf;
    if (auto it = availableFonts.find(font); it != availableFonts.end())
        rf = it.value().first;
    return rf;
}

QMetaEnum QFontIconEnginePrivate::getEnum(const QString& font)
{
    QMetaEnum me;
    if (auto it = availableFonts.find(font); it != availableFonts.end())
        me = it.value().second;
    return me;
}

void QFontIconEnginePrivate::resizeFont(const QSizeF& size, QRawFont& font, qreal scale, quint32 glyphIndex)
{
    qreal drawSize = size.width();
    font.setPixelSize(drawSize);
    // Adjust scale to fit the widest glyph in the font to avoid varying sizes for different glyphs
    const qreal maxCharWidth = font.maxCharWidth();
    if (maxCharWidth > drawSize)
        drawSize = drawSize * drawSize / maxCharWidth;
    font.setPixelSize(drawSize * scale);
}

/*
 * Like QMetaEnum::keyToValue() but uses binary rather than linear search.
 * Key-value pairs must be sorted by key, like produced by generate_fa.py.
 */
int QFontIconEnginePrivate::keyToValue(const QMetaEnum& metaEnum, const char* key, bool* ok)
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

// =============================================================================

/**
 * @class QFontIconEngine
 * @brief QIconEngine subclass to handle font glyphs as icons.
 *
 * QFontIconEngine can be used with any fonts. You presumably have embedded your
 * favourite glyph font in a @c .qrc file. The first thing you need to do is
 * loading you font using the loadFont() static function:
 *
 * @code
 * QFontIconEngine::loadFont(":/path/to/my_font.ttf");
 * @endcode
 *
 * Then you can use the icon() static helper function to easily create icons from
 * the font you just loaded:
 * @code
 * QIcon icon = QFontIconEngine::icon(0xf82b) // This is the code point of the glyph
 *
 * // Alternatively, you can make an enumeration and optionally register it with Qt
 * class my_font : public QObject {
 *     v5() = delete;
 *     Q_OBJECT;
 * public: enum codepoints {
 *     super_glyph = 0xf82b
 * };
 * Q_ENUM(codepoints)
 * };
 *
 * QFontIconEngine::loadFont(":/path/to/my_font.ttf", QMetaEnum::fromType<my_font::codepoints>());
 *
 * // And then use names instead:
 * QIcon otherIcon = QFontIconEngine::icon("super-glyph");
 * @endcode
 *
 * QFontIconEngine supports stateful icons. That means you can set different
 * glyphs and various other properties on a per-state basis, depending on
 * @c QIcon::Mode and @c QIcon::State . For example:
 *
 * @code
 * // Assuming you have registered icon names
 * // create the engine and then set properties
 * auto engine = new QFontIconEngine();
 * engine->setIcon("toggle-on",  QIcon::Normal, QIcon::On);
 * engine->setIcon("toggle-off", QIcon::Normal, QIcon::Off);
 *
 * engine->setColor(Qt::green, QIcon::Normal, QIcon::On);
 * engine->setColor(Qt::red,   QIcon::Normal, QIcon::Off);
 *
 * // Pass it to a QIcon. Note that QIcon takes the ownership of the engine so.
 * // Therefore it's unsafe to reuse if afterward.
 * QIcon icon(engine);
 * @endcode
 *
 * The default state is { QIcon::Normal, QIcon::Off }  It is the
 * one set by default when using helper fuctions. If you create and engine from
 * scratch, this state must be set.
 *
 * If a value is not available in a given state { M, S } , it will
 * first try to find a { QIcon::Normal, S } and if still not
 * successful, will ultimately default to { QIcon::Normal, QIcon::Off }.
 *
 * Properties also include a spinning animation. (Yep)
 * @code
 * // Assuming you'll put it on a button
 * auto btn = new QPushButton();
 *
 * auto engine = new QFontIconEngine();
 * engine->setIcon("spinner");
 *
 * engine->setWidget(btn); // You need to tell the engine which widget to update
 * engine->setSpeed(180); // speed is in degree per second. 180 <=> 1 turn in 2s
 * engine->setCurve(QEasingCurve::InOutSine); // You can even set non-linear rotation.
 *
 * // Pass it to a QIcon and profits
 * QIcon icon(engine);
 * @endcode
 *
 * You can load multiple fonts and use them seamlessly.
 * When loading a font you can specify a name in the form of an URL fragment.
 *
 * QFontIconEngine::loadFont(":/path/to/my_font.ttf#primary");
 * QFontIconEngine::loadFont(":/path/to/my_font.ttf#secondary");
 *
 * // Then you can refer to the font through that name:
 * QIcon icon = QFontIconEngine::icon("super-glyph", "secondary");
 *
 * // This one will use the default (nameless) font
 * QIcon icon2 = QFontIconEngine::icon("super-glyph");
 * @endcode
 */

/**
 * Construct an engine with the default font.
 */
QFontIconEngine::QFontIconEngine() :
    d(new QFontIconEnginePrivate)
{
}

/**
 * @brief Copy constructor.
 */
QFontIconEngine::QFontIconEngine(const QFontIconEngine& other) :
    d(new QFontIconEnginePrivate(*other.d))
{
    d->timerConnection = QMetaObject::Connection();
    d->setupTimer();
}

/**
 * @brief Construct and engine using @a icon and @a font.
 *
 * @a icon and @a font will set the { QIcon::Normal, QIcon::Off } state and
 * therefore be used by default.
 *
 * @param icon the code point fo the icon to use
 * @param font the font id to use
 */
QFontIconEngine::QFontIconEngine(int icon, const QString& font) : QFontIconEngine()
{
    setFont(font);
    setIcon(icon);
}

/**
 * @brief Construct and engine using @a icon and @a font.
 *
 * @overload
 *
 * @param icon the glyph name fo the icon to use
 * @param font the font name to use
 */
QFontIconEngine::QFontIconEngine(const QString& icon, const QString& font) : QFontIconEngine()
{
    setFont(font);
    setIcon(icon);
}

QFontIconEngine::QFontIconEngine(const QUrl& url) : QFontIconEngine()
{
    const QUrlQuery query(url);

    if (query.isEmpty())
    {
        const QString path = url.path();
        const QString fragment = url.fragment();
        QFontIconEnginePrivate::availableFonts[fragment].first.loadFromFile(path, 32, QFont::PreferDefaultHinting);
        // set font name so destructor can register a QMetaEnum should one have been assigned in between
        setFont(fragment);
        return;
    }

    const QString fragment = url.path() + url.fragment();

    setFont(fragment);

    for (const auto& pair : query.queryItems())
    {
        char* dot = nullptr;
        QByteArray tok = pair.first.toUtf8();
        if (const uint mapping = strtoul(tok, &dot, 0); dot != nullptr)
        {
            if (*dot == '\0')
            {
                QColor color;
                tok = pair.second.toUtf8();
                if (uint x = static_cast<uint>(strtoul(tok.data(), &dot, 0)); dot == tok.data())
                {
                    color = QColor::fromString(tok);
                }
                else switch (tok.length())
                {
                case std::string_view("0xRGB").size():
                    x = (x & 0xF00) * 0x1100 | (x & 0xF0) * 0x110 | (x & 0xF) * 0x11;
                    [[fallthrough]];
                case std::string_view("0xRRGGBB").size():
                    color = QColor::fromRgb(x);
                    break;
                case std::string_view("0xARGB").size():
                    x = (x & 0xF000) * 0x11000 | (x & 0xF00) * 0x1100 | (x & 0xF0) * 0x110 | (x & 0xF) * 0x11;
                    [[fallthrough]];
                case std::string_view("0xAARRGGBB").size():
                    color = QColor::fromRgba(x);
                    break;
                default:
                    break;
                }
                if (color.isValid())
                {
                    for (uint mode = QIcon::Normal; mode <= QIcon::Selected; ++mode)
                    {
                        if (mapping & (0x10 << mode))
                        {
                            setColor(color, QIcon::Mode(mode), QIcon::On);
                        }
                        if (mapping & (0x01 << mode))
                        {
                            setColor(color, QIcon::Mode(mode), QIcon::Off);
                        }
                    }
                }
            }
            else if (strcmp(dot, ".codepoint") == 0)
            {
                tok = pair.second.toUtf8();
                int codepoint = static_cast<int>(strtoul(tok.data(), &dot, 0));
                bool ok = codepoint >= 0 && codepoint < InvalidIcon;
                if (dot == tok.data())
                {
                    codepoint = QFontIconEnginePrivate::keyToValue(d->getEnum(fragment), tok, &ok);
                }
                if (ok)
                {
                    for (uint mode = QIcon::Normal; mode <= QIcon::Selected; ++mode)
                    {
                        if (mapping & (0x10 << mode))
                        {
                            setIcon(codepoint, QIcon::Mode(mode), QIcon::On);
                        }
                        if (mapping & (0x01 << mode))
                        {
                            setIcon(codepoint, QIcon::Mode(mode), QIcon::Off);
                        }
                    }
                }
            }
        }
    }
}

QFontIconEngine::~QFontIconEngine()
{
    if (QMetaEnum::isValid())
    {
        QFontIconEnginePrivate::availableFonts[font()].second = *this;
    }
}

/**
 * @brief Returns the icon code point set for the given state.
 */
int QFontIconEngine::icon(QIcon::Mode mode, QIcon::State state) const
{
    return d->icons.get(mode, state, InvalidIcon);
}

/**
 * @brief Returns the default icon name.
 */
QString QFontIconEngine::iconName() QFI6_CONST
{
    return iconName(QIcon::Normal, QIcon::Off);
}

/**
 * @brief Returns the icon name set for the given state.
 */
QString QFontIconEngine::iconName(QIcon::Mode mode, QIcon::State state) const
{
    return d->getEnum(font(mode, state)).valueToKey(icon(mode, state));
}

/**
 * @brief Returns the icon code point as a text string for the given state.
 */
QString QFontIconEngine::text(QIcon::Mode mode, QIcon::State state) const
{
    return QChar(icon(mode, state));
}

/**
 * @brief Returns the icon glyph index in the font. (which is different from the codepoint)
 */
quint32 QFontIconEngine::glyphIndex(QIcon::Mode mode, QIcon::State state) const
{
    auto f = QFontIconEnginePrivate::getFont(font(mode, state));
    auto v = f.glyphIndexesForString(text(mode, state));
    return v.count() == 1 ? v.first() : 0;
}

/**
 * @brief Returns the font id set for the given state.
 */
QString QFontIconEngine::font(QIcon::Mode mode, QIcon::State state) const
{
    return d->fonts.get(mode, state);
}

/**
 * @brief Returns the scale factor set for the given state.
 */
qreal QFontIconEngine::scale(QIcon::Mode mode, QIcon::State state) const
{
    return d->scales.get(mode, state, 1.0);
}

/**
 * @brief Returns the color set for the given state.
 */
QColor QFontIconEngine::color(QIcon::Mode mode, QIcon::State state) const
{
    auto c = d->colors.get(mode, state);

    if(!c.isValid())
    {
        auto p = QApplication::style()->standardPalette();

        switch (mode)
        {
        case QIcon::Active:
            c = p.color(QPalette::Active, QPalette::ButtonText);
            break;

        case QIcon::Normal:
            c = p.color(QPalette::Normal, QPalette::ButtonText);
            break;

        case QIcon::Disabled:
            c = p.color(QPalette::Disabled, QPalette::ButtonText);
            break;

        case QIcon::Selected:
            c = p.color(QPalette::Active, QPalette::ButtonText);
            break;
        }
    }

    return c;
}

/**
 * @brief Returns the rotation speed set for the given state.
 */
qreal QFontIconEngine::speed(QIcon::Mode mode, QIcon::State state) const
{
    return d->speeds.get(mode, state, 0);
}

/**
 * @brief Returns the easing curve set for the given state.
 */
QEasingCurve QFontIconEngine::curve(QIcon::Mode mode, QIcon::State state) const
{
    return d->curves.get(mode, state);
}

/**
 * @brief Returns the widget the rotation animation displays on.
 */
QWidget* QFontIconEngine::widget() const
{
    return d->widget;
}

/**
 * @brief Returns whether to display a red badge on the icon
 */
bool QFontIconEngine::badgeEnabled() const
{
    return d->badge;
}

/**
 * @brief Set the icon code point for the given state.
 */
void QFontIconEngine::setIcon(int icon, QIcon::Mode mode, QIcon::State state)
{
    d->icons.set(icon, mode, state);
}

/**
 * @brief Set the icon name for the given state.
 */
void QFontIconEngine::setIcon(const QString& name, QIcon::Mode mode, QIcon::State state)
{
    bool ok = false;
    if (int i = QFontIconEnginePrivate::keyToValue(d->getEnum(font(mode, state)), name.toUtf8(), &ok); ok)
        setIcon(i, mode, state);
    else
        qWarning() << "QFontIcon: Invalid icon name";
}

/**
 * @brief Set the font name for the given state.
 */
void QFontIconEngine::setFont(const QString& font, QIcon::Mode mode, QIcon::State state)
{
    d->fonts.set(font, mode, state);
}

/**
 * @brief Set the scale for the given state.
 */
void QFontIconEngine::setScale(qreal scale, QIcon::Mode mode, QIcon::State state)
{
    d->scales.set(scale, mode, state);
}

/**
 * @brief Set the color for the given state.
 */
void QFontIconEngine::setColor(const QColor& color, QIcon::Mode mode, QIcon::State state)
{
    d->colors.set(color, mode, state);
}

/**
 * @brief Set the rotation speed for the given state.
 */
void QFontIconEngine::setSpeed(qreal speed, QIcon::Mode mode, QIcon::State state)
{
    d->speeds.set(speed, mode, state);
    d->setupTimer();
}

/**
 * @brief Set the easing curve point for the given state.
 */
void QFontIconEngine::setCurve(const QEasingCurve& curve, QIcon::Mode mode, QIcon::State state)
{
    d->curves.set(curve, mode, state);
}

/**
 * @brief Set the widget the rotation animation displays on.
 */
void QFontIconEngine::setWidget(QWidget* widget)
{
    d->widget = widget;
    d->setupTimer();
}

/**
 * @brief Set if the red badge should be enabled
 */
void QFontIconEngine::setBadgeEnabled(bool en)
{
    d->badge = en;
}

QFontIconEngine* QFontIconEngine::QFontIconEngine::clone() const
{
    return new QFontIconEngine(*this);
}

void QFontIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state)
{
    painter->save();

    painter->setRenderHint(QPainter::Antialiasing);

    auto r  = QRectF(rect); // Use floating for more precision
    auto s  = r.size();
    auto g  = glyphIndex(mode, state);
    QString id  = font(mode, state);
    auto f  = QFontIconEnginePrivate::getFont(id);
    auto sf = scale(mode, state);
    auto c  = color(mode, state);

    d->resizeFont(s, f, sf, g);

    auto a = d->angles.get(mode, state);

    if(a != 0)
    {
        auto center = r.center();
        painter->translate(center.x(), center.y());
        painter->rotate(a);
        painter->translate(-center.x(), -center.y());
    }

    auto bounds = f.boundingRect(g);
    auto glyph = f.pathForGlyph(g);

    painter->translate(r.center() - bounds.center());
    painter->setPen(Qt::NoPen);
    painter->setBrush(c);
    painter->drawPath(glyph);

    painter->restore();

    if(d->badge)
    {
        painter->save();

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255, 0, 0, 200));

        auto bs = r.size() / 3.0;

        if(bs.width() < 8 || bs.height() < 8)
            bs = r.size() / 2.0;

        QRectF badgeRect(r.right()-bs.width(), r.top(), bs.width(), bs.height());

        painter->drawEllipse(badgeRect);

        painter->restore();
    }
}

QPixmap QFontIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state)
{
    QPixmap pm(size);
    pm.fill(Qt::transparent);
    {
        QPainter p(&pm);
        paint(&p, QRect(QPoint(0, 0), size), mode, state);
    }
    return pm;
}

/**
 * @brief load the font located at @a filename.
 *
 * You can pass an arbitrary font id and font name if you're using multiple
 * fonts.
 */
bool QFontIconEngine::loadFont(const QString& filename, QMetaEnum codepoints)
{
    QUrl url(filename);

    // Open it
    QRawFont rawFont(url.path(), 32);

    QFontIconEnginePrivate::availableFonts[url.fragment()] = qMakePair(rawFont, codepoints);

    return true;
}

/**
 * @brief Convenience function that returns an icon.
 */
QIcon QFontIconEngine::icon(int icon, const QString& font)
{
    return QIcon(new QFontIconEngine(icon, font));
}

/**
 * @brief Convenience function that returns an icon.
 */
QIcon QFontIconEngine::icon(const QString& icon, const QString& font)
{
    return QIcon(new QFontIconEngine(icon, font));
}

/**
 * @brief Factory function that creates a QFontIconEngine.
 */
QIconEngine* QFontIconPlugin::create(const QString& icon)
{
    // Resolve a conflict between a static and a dynamic plugin in favor of the former.
    static QIconEnginePlugin* that = nullptr;
    if (that == nullptr)
    {
        const char* const className = metaObject()->className();
        for (QIconEnginePlugin* child : qApp->findChildren<QIconEnginePlugin*>())
            if (strcmp(child->metaObject()->superClass()->className(), className) == 0)
                that = child;
    }
    if (that && that != this)
        return that->create(icon);

    if (icon.endsWith(".ttf"))
        return new QFontIconEngine(QUrl(icon));

    return nullptr;
}
