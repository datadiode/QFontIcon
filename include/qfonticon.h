#ifndef QFONTICON_H
#define QFONTICON_H

#include <QEasingCurve>
#include <QIconEngine>
#include <QMetaEnum>
#include <QUrlQuery>
#include <QVariant>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define QFI6_CONST const
#else
#define QFI6_CONST
#endif

class QFontIconEnginePrivate;
class QFONTICON_EXPORT QFontIconEngine : public QIconEngine, public QMetaEnum
{
public:
    enum { InvalidIcon = 0xffff };

public:
    QFontIconEngine();
    QFontIconEngine(const QFontIconEngine& other);
    QFontIconEngine(int icon, const QString& font = QString());
    QFontIconEngine(const QString& icon, const QString& font = QString());
    QFontIconEngine(const QUrl& url);

    ~QFontIconEngine() override;

    // ======

    int icon(QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off) const;
    QString iconName() QFI6_CONST override;
    QString iconName(QIcon::Mode mode, QIcon::State state) const;
    QString text(QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off) const;
    quint32 glyphIndex(QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off) const;
    QString font(QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off) const;
    qreal scale(QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off) const;
    QColor color(QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off) const;
    qreal speed(QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off) const;
    QEasingCurve curve(QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off) const;
    QWidget* widget() const;
    bool badgeEnabled() const;

    void setIcon(int icon, QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off);
    void setIcon(const QString& name, QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off);
    void setFont(const QString& font, QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off);
    void setScale(qreal scale, QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off);
    void setColor(const QColor& color, QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off);
    void setSpeed(qreal speed, QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off);
    void setCurve(const QEasingCurve& curve, QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off);
    void setWidget(QWidget* widget);
    void setBadgeEnabled(bool en);

    // ======

    QFontIconEngine* clone() const override;

    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;

public:
    static bool loadFont(const QString& filename, QMetaEnum codepoints = QMetaEnum());
    static QIcon icon(int icon, const QString& font = QString());
    static QIcon icon(const QString& icon, const QString& font = QString());

protected:
    QScopedPointer<QFontIconEnginePrivate> d;
};

/*
 * Hackish way to accociate a QMetaEnum with a font when the hosting application
 * does not link against the QFontIcon library but rather loads it as a plugin.
 */
class QIconMetaEnum : public QIcon, public QMetaEnum
{
public:
    using QIcon::QIcon;
    using QMetaEnum::operator=;
    ~QIconMetaEnum()
    {
        // Daringly assume the first member of QIconPrivate is the QIconEngine*.
        if (QIconEngine** p = reinterpret_cast<QIconEngine**>(data_ptr()))
            if (QMetaEnum* q = dynamic_cast<QMetaEnum*>(*p))
                *q = *this;
    }
};

#include <QtPlugin>
#include <QIconEnginePlugin>

class QFONTICON_EXPORT QFontIconPlugin : public QIconEnginePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QIconEngineFactoryInterface_iid FILE "qfonticon.json");
public:
	using QIconEnginePlugin::QIconEnginePlugin;
    QIconEngine* create(const QString& icon) override;
};

#endif // QFONTICON_H
