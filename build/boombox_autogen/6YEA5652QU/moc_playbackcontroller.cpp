/****************************************************************************
** Meta object code from reading C++ file 'playbackcontroller.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/playbackcontroller.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'playbackcontroller.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN18PlaybackControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto PlaybackController::qt_create_metaobjectdata<qt_meta_tag_ZN18PlaybackControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "PlaybackController",
        "trackChanged",
        "",
        "filePath",
        "trackMetadataLoaded",
        "AudioMetadata",
        "meta",
        "playbackError",
        "error",
        "spectrumLevelsUpdated",
        "QList<float>",
        "levels",
        "onAudioEventTick",
        "onTrackFinished",
        "onPlaybackError",
        "AudioEngine::ErrorCode",
        "errorCode",
        "std::string",
        "errorMsg"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'trackChanged'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'trackMetadataLoaded'
        QtMocHelpers::SignalData<void(const AudioMetadata &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 6 },
        }}),
        // Signal 'playbackError'
        QtMocHelpers::SignalData<void(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Signal 'spectrumLevelsUpdated'
        QtMocHelpers::SignalData<void(const QVector<float> &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Slot 'onAudioEventTick'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTrackFinished'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPlaybackError'
        QtMocHelpers::SlotData<void(AudioEngine::ErrorCode, const std::string &)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 16 }, { 0x80000000 | 17, 18 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PlaybackController, qt_meta_tag_ZN18PlaybackControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject PlaybackController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18PlaybackControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18PlaybackControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN18PlaybackControllerE_t>.metaTypes,
    nullptr
} };

void PlaybackController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PlaybackController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->trackChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->trackMetadataLoaded((*reinterpret_cast<std::add_pointer_t<AudioMetadata>>(_a[1]))); break;
        case 2: _t->playbackError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->spectrumLevelsUpdated((*reinterpret_cast<std::add_pointer_t<QList<float>>>(_a[1]))); break;
        case 4: _t->onAudioEventTick(); break;
        case 5: _t->onTrackFinished(); break;
        case 6: _t->onPlaybackError((*reinterpret_cast<std::add_pointer_t<AudioEngine::ErrorCode>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<std::string>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<float> >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (PlaybackController::*)(const QString & )>(_a, &PlaybackController::trackChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (PlaybackController::*)(const AudioMetadata & )>(_a, &PlaybackController::trackMetadataLoaded, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (PlaybackController::*)(const QString & )>(_a, &PlaybackController::playbackError, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (PlaybackController::*)(const QVector<float> & )>(_a, &PlaybackController::spectrumLevelsUpdated, 3))
            return;
    }
}

const QMetaObject *PlaybackController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PlaybackController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18PlaybackControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int PlaybackController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void PlaybackController::trackChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void PlaybackController::trackMetadataLoaded(const AudioMetadata & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void PlaybackController::playbackError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void PlaybackController::spectrumLevelsUpdated(const QVector<float> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
