#pragma once


#include "pathhistorydb.hpp"

#include <QObject>
#include <QHash>
#include <QList>
#include <QSettings>

class QDir;

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
    Q_PROPERTY(QString lastSessionUrl READ lastSessionUrl WRITE setLastSessionUrl NOTIFY lastSessionUrlChanged FINAL)
    Q_PROPERTY(int videoRotation READ videoRotation WRITE setVideoRotation NOTIFY videoRotationChanged FINAL)
    Q_PROPERTY(bool showMainFileView READ showMainFileView WRITE setShowMainFileView NOTIFY showMainFileViewChanged FINAL)


public:
    explicit Preferences(const QDir &appDataDir
                         , const QString &pathHistoryDBPath
                         , QObject *parent = nullptr);

    bool volumeMuted() const;
    void setVolumeMuted(bool newVolumeMute);

    qreal volume() const;
    void setVolume(qreal newVolume);

    Q_INVOKABLE QByteArray mainSplitviewState();
    Q_INVOKABLE void setMainSplitViewState(const QByteArray &data);

    QStringList recentUrls() const;
    Q_INVOKABLE void pushRecentUrl(const QString &path);

    QString lastSessionUrl() const;
    void setLastSessionUrl(const QString &newLastSessionUrl);

    // index: {row, col}
    Q_INVOKABLE QVector<int> urlLastIndex(const QString &url) const;

    Q_INVOKABLE void setUrlLastIndex(const QString &url
                                     , const QVector<int> &idx);

    int videoRotation() const;
    void setVideoRotation(int newVideoRotation);

    bool showMainFileView() const;
    void setShowMainFileView(bool newShowMainFileView);

signals:
    void volumeMutedChanged();

    void volumeChanged();

    void recentUrlsChanged();

    void lastSessionIndexChanged();

    void lastSessionUrlChanged();

    void videoRotationChanged();

    void showMainFileViewChanged();

private:
    PathHistoryDB m_pathHistory;

    QSettings m_setting;
    SettingEntry<bool> m_volumeMuted;
    SettingEntry<qreal> m_volume;
    SettingEntry<QByteArray> m_mainSplitViewState;
    SettingEntry<bool> m_showMainFileView;
    SettingEntry<QStringList> m_recentUrls;
    SettingEntry<QString> m_lastSessionUrl;
    SettingEntry<int> m_videoRotation;
};

