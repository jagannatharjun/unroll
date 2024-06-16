#include "preferences.hpp"

namespace
{

const QString VOLUME_MUTE_KEY = "Player/VolumeMute";
const QString VOLUME_KEY = "Player/Volume";
const QString MAIN_SPLITVIEW_STATE = "MainView/SplitviewState";

}

Preferences::Preferences(QObject *parent)
    : QObject{parent}
    , m_setting ("filebrowser")
    , m_volumeMuted (VOLUME_MUTE_KEY, false, m_setting)
    , m_volume (VOLUME_KEY, .5, m_setting)
    , m_mainSplitViewState (MAIN_SPLITVIEW_STATE, {}, m_setting)
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
