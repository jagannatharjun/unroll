#include "preferences.hpp"

namespace
{

const QString VOLUME_MUTE_KEY = "Player/VolumeMute";
const QString VOLUME_KEY = "Player/Volume";
const QString MAIN_SPLITVIEW_STATE = "MainView/SplitviewState";
const QString RECENT_URLS = "RECENT_URLS";


const int MAX_RECENT_PATH = 10;

}

Preferences::Preferences(QObject *parent)
    : QObject{parent}
    , m_setting ("filebrowser")
    , m_volumeMuted (VOLUME_MUTE_KEY, false, m_setting)
    , m_volume (VOLUME_KEY, .5, m_setting)
    , m_mainSplitViewState (MAIN_SPLITVIEW_STATE, {}, m_setting)
    , m_recentUrls(RECENT_URLS, {}, m_setting)
{
}

bool Preferences::volumeMuted() const
{
    return m_volumeMuted;
}

void Preferences::setVolumeMuted(bool newVolumeMuted)
{
    if (m_volumeMuted == newVolumeMuted)
        return;

    m_volumeMuted.set(newVolumeMuted, m_setting);
    emit volumeMutedChanged();
}

qreal Preferences::volume() const
{
    return m_volume;
}

void Preferences::setVolume(qreal newVolume)
{
    if (qFuzzyCompare(m_volume, newVolume))
        return;

    m_volume.set(newVolume, m_setting);
    emit volumeChanged();
}

QByteArray Preferences::mainSplitviewState()
{
    return m_mainSplitViewState.value();
}

void Preferences::setMainSplitViewState(const QByteArray &data)
{
    m_mainSplitViewState.set(data, m_setting);
}

QStringList Preferences::recentUrls() const
{
    return m_recentUrls;
}

void Preferences::pushRecentUrl(const QString &path)
{
    auto recentUrls = m_recentUrls.value();
    if (!recentUrls.empty() && recentUrls.first() == path)
        return;

    recentUrls.removeOne(path);
    recentUrls.insert(0, path);
    if (recentUrls.size() > MAX_RECENT_PATH)
        recentUrls.pop_back();

    m_recentUrls.set(recentUrls, m_setting);
    emit recentUrlsChanged();
}
