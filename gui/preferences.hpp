#pragma once

#include <QObject>
#include <QSettings>


template <typename T>
class SettingEntry
{
public:
    SettingEntry(const QString &key, const T defaultValue, QSettings &settings)
        : m_key {key}
    {
        if (settings.contains(key))
        {
            const QVariant value = settings.value(key);

            assert(value.canConvert<T>());
            m_value = value.value<T>();
        }
        else
        {
            m_value = defaultValue;
        }
    }

    void set(const T& value, QSettings &setting)
    {
        setting.setValue(m_key, QVariant::fromValue(value));
        m_value = value;
    }

    T value() const { return m_value; }

    operator T() const { return value(); }

private:
    QString m_key;
    T m_value;
};


class Preferences : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool volumeMuted READ volumeMuted WRITE setVolumeMuted NOTIFY volumeMutedChanged FINAL)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged FINAL)
    Q_PROPERTY(QStringList recentUrls READ recentUrls NOTIFY recentUrlsChanged FINAL)

    // row int
    Q_PROPERTY(QVector<int> lastSessionIndex READ lastSessionIndex WRITE setLastSessionIndex NOTIFY lastSessionIndexChanged FINAL)
    Q_PROPERTY(QString lastSessionUrl READ lastSessionUrl WRITE setLastSessionUrl NOTIFY lastSessionUrlChanged FINAL)

public:
    explicit Preferences(QObject *parent = nullptr);

    bool volumeMuted() const;
    void setVolumeMuted(bool newVolumeMute);

    qreal volume() const;
    void setVolume(qreal newVolume);

    Q_INVOKABLE QByteArray mainSplitviewState();
    Q_INVOKABLE void setMainSplitViewState(const QByteArray &data);

    QStringList recentUrls() const;
    Q_INVOKABLE void pushRecentUrl(const QString &path);

    QVector<int> lastSessionIndex() const;
    void setLastSessionIndex(const QVector<int> &newLastSessionIndex);

    QString lastSessionUrl() const;
    void setLastSessionUrl(const QString &newLastSessionUrl);

signals:
    void volumeMutedChanged();

    void volumeChanged();

    void recentUrlsChanged();

    void lastSessionIndexChanged();

    void lastSessionUrlChanged();

private:
    QSettings m_setting;
    SettingEntry<bool> m_volumeMuted;
    SettingEntry<qreal> m_volume;
    SettingEntry<QByteArray> m_mainSplitViewState;
    SettingEntry<QStringList> m_recentUrls;
    SettingEntry<QVector<int>> m_lastSessionIndex;
    SettingEntry<QString> m_lastSessionUrl;
};

