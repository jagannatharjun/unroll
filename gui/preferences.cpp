#include "preferences.hpp"

#include <QDir>

namespace
{

const QString VOLUME_MUTE_KEY = "Player/VolumeMute";
const QString VOLUME_KEY = "Player/Volume";
const QString MAIN_SPLITVIEW_STATE = "MainView/SplitviewState";
const QString SHOW_MAIN_FILEVIEW = "MainView/ShowFileView";
const QString RECENT_URLS = "RECENT_URLS";
const QString LAST_SESSION_INDEX = "MainView/LAST_SESSION_INDEX";
const QString LAST_SESSION_URL = "MainView/LAST_SESSION_URL";
const QString PREVIEW_VIDEO_ROTATION = "MainView/PREVIEW_VIDEO_ROTATION";


const int MAX_RECENT_PATH = 10;

}

Preferences::Preferences(const QDir &appDataDir
                         , const QString &pathHistoryDBPath
                         , QObject *parent)
    : QObject{parent}
    , m_pathHistory(pathHistoryDBPath)
    , m_setting (appDataDir.absoluteFilePath("settings.ini"), QSettings::IniFormat)
    , m_volumeMuted (VOLUME_MUTE_KEY, false, m_setting)
    , m_volume (VOLUME_KEY, .5, m_setting)
    , m_mainSplitViewState (MAIN_SPLITVIEW_STATE, {}, m_setting)
    , m_showMainFileView (SHOW_MAIN_FILEVIEW, {}, m_setting)
    , m_recentUrls(RECENT_URLS, {}, m_setting)
    , m_lastSessionUrl(LAST_SESSION_URL, {}, m_setting)
    , m_videoRotation(PREVIEW_VIDEO_ROTATION, {}, m_setting)
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

QString Preferences::lastSessionUrl() const
{
    return m_lastSessionUrl;
}

void Preferences::setLastSessionUrl(const QString &newLastSessionUrl)
{
    if (m_lastSessionUrl == newLastSessionUrl)
        return;

    m_lastSessionUrl.set(newLastSessionUrl, m_setting);
    emit lastSessionUrlChanged();
}

QVector<int> Preferences::urlLastIndex(const QString &url) const
{
    const auto data = m_pathHistory.read(url);
    return {data.row.value_or(0), data.col.value_or(0)};
}

void Preferences::setUrlLastIndex(const QString &url, const QVector<int> &idx)
{
    auto data = m_pathHistory.read(url);
    data.row = idx[0];
    data.col = idx[1];
    m_pathHistory.set(url, data);
}

int Preferences::videoRotation() const
{
    return m_videoRotation;
}

void Preferences::setVideoRotation(int newVideoRotation)
{
    if (m_videoRotation == newVideoRotation)
        return;

    m_videoRotation.set(newVideoRotation, m_setting);
    emit videoRotationChanged();
}

bool Preferences::showMainFileView() const
{
    return m_showMainFileView;
}

void Preferences::setShowMainFileView(bool newShowMainFileView)
{
    if (m_showMainFileView == newShowMainFileView)
        return;

    m_showMainFileView.set(newShowMainFileView, m_setting);
    emit showMainFileViewChanged();
}

