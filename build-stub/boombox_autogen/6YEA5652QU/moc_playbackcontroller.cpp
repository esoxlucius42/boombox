/****************************************************************************
** Meta object code from reading C++ file 'playbackcontroller.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/playbackcontroller.h"
#include <QtCore/qmetatype.h>

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
        "requestLoadFolder",
        "folderPath",
        "requestPlayNext",
        "requestSeek",
        "position",
        "requestPlay",
        "requestPause",
        "requestShutdown",
        "onWorkerTrackChanged",
        "trackCount",
        "onWorkerTrackMetadataLoaded",
        "onWorkerPlaybackError",
        "onWorkerPlaybackSnapshot",
        "playing",
        "duration",
        "onWorkerThreadFinished"
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
        // Signal 'requestLoadFolder'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Signal 'requestPlayNext'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'requestSeek'
        QtMocHelpers::SignalData<void(int)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 },
        }}),
        // Signal 'requestPlay'
        QtMocHelpers::SignalData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'requestPause'
        QtMocHelpers::SignalData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'requestShutdown'
        QtMocHelpers::SignalData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onWorkerTrackChanged'
        QtMocHelpers::SlotData<void(const QString &, int, int)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::Int, 13 }, { QMetaType::Int, 18 },
        }}),
        // Slot 'onWorkerTrackMetadataLoaded'
        QtMocHelpers::SlotData<void(const AudioMetadata &)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 5, 6 },
        }}),
        // Slot 'onWorkerPlaybackError'
        QtMocHelpers::SlotData<void(const QString &)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Slot 'onWorkerPlaybackSnapshot'
        QtMocHelpers::SlotData<void(bool, double, double)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 22 }, { QMetaType::Double, 13 }, { QMetaType::Double, 23 },
        }}),
        // Slot 'onWorkerThreadFinished'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
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
        case 3: _t->requestLoadFolder((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->requestPlayNext(); break;
        case 5: _t->requestSeek((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->requestPlay(); break;
        case 7: _t->requestPause(); break;
        case 8: _t->requestShutdown(); break;
        case 9: _t->onWorkerTrackChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 10: _t->onWorkerTrackMetadataLoaded((*reinterpret_cast<std::add_pointer_t<AudioMetadata>>(_a[1]))); break;
        case 11: _t->onWorkerPlaybackError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->onWorkerPlaybackSnapshot((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[3]))); break;
        case 13: _t->onWorkerThreadFinished(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< AudioMetadata >(); break;
            }
            break;
        case 10:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< AudioMetadata >(); break;
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
        if (QtMocHelpers::indexOfMethod<void (PlaybackController::*)(const QString & )>(_a, &PlaybackController::requestLoadFolder, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (PlaybackController::*)()>(_a, &PlaybackController::requestPlayNext, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (PlaybackController::*)(int )>(_a, &PlaybackController::requestSeek, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (PlaybackController::*)()>(_a, &PlaybackController::requestPlay, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (PlaybackController::*)()>(_a, &PlaybackController::requestPause, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (PlaybackController::*)()>(_a, &PlaybackController::requestShutdown, 8))
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
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
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
void PlaybackController::requestLoadFolder(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void PlaybackController::requestPlayNext()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void PlaybackController::requestSeek(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void PlaybackController::requestPlay()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void PlaybackController::requestPause()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void PlaybackController::requestShutdown()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}
QT_WARNING_POP
