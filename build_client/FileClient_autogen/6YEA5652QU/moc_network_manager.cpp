/****************************************************************************
** Meta object code from reading C++ file 'network_manager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../Client/include/network_manager.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'network_manager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_NetworkManager_t {
    uint offsetsAndSizes[80];
    char stringdata0[15];
    char stringdata1[17];
    char stringdata2[1];
    char stringdata3[8];
    char stringdata4[4];
    char stringdata5[13];
    char stringdata6[12];
    char stringdata7[16];
    char stringdata8[15];
    char stringdata9[17];
    char stringdata10[5];
    char stringdata11[14];
    char stringdata12[9];
    char stringdata13[15];
    char stringdata14[16];
    char stringdata15[17];
    char stringdata16[12];
    char stringdata17[13];
    char stringdata18[13];
    char stringdata19[14];
    char stringdata20[17];
    char stringdata21[8];
    char stringdata22[6];
    char stringdata23[24];
    char stringdata24[10];
    char stringdata25[20];
    char stringdata26[10];
    char stringdata27[21];
    char stringdata28[11];
    char stringdata29[12];
    char stringdata30[6];
    char stringdata31[19];
    char stringdata32[10];
    char stringdata33[21];
    char stringdata34[18];
    char stringdata35[6];
    char stringdata36[20];
    char stringdata37[11];
    char stringdata38[7];
    char stringdata39[12];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_NetworkManager_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_NetworkManager_t qt_meta_stringdata_NetworkManager = {
    {
        QT_MOC_LITERAL(0, 14),  // "NetworkManager"
        QT_MOC_LITERAL(15, 16),  // "connectionStatus"
        QT_MOC_LITERAL(32, 0),  // ""
        QT_MOC_LITERAL(33, 7),  // "success"
        QT_MOC_LITERAL(41, 3),  // "msg"
        QT_MOC_LITERAL(45, 12),  // "loginSuccess"
        QT_MOC_LITERAL(58, 11),  // "loginFailed"
        QT_MOC_LITERAL(70, 15),  // "registerSuccess"
        QT_MOC_LITERAL(86, 14),  // "registerFailed"
        QT_MOC_LITERAL(101, 16),  // "fileListReceived"
        QT_MOC_LITERAL(118, 4),  // "data"
        QT_MOC_LITERAL(123, 13),  // "uploadStarted"
        QT_MOC_LITERAL(137, 8),  // "filename"
        QT_MOC_LITERAL(146, 14),  // "uploadProgress"
        QT_MOC_LITERAL(161, 15),  // "downloadStarted"
        QT_MOC_LITERAL(177, 16),  // "downloadComplete"
        QT_MOC_LITERAL(194, 11),  // "shareResult"
        QT_MOC_LITERAL(206, 12),  // "deleteResult"
        QT_MOC_LITERAL(219, 12),  // "renameResult"
        QT_MOC_LITERAL(232, 13),  // "logoutSuccess"
        QT_MOC_LITERAL(246, 16),  // "transferProgress"
        QT_MOC_LITERAL(263, 7),  // "current"
        QT_MOC_LITERAL(271, 5),  // "total"
        QT_MOC_LITERAL(277, 23),  // "folderStructureReceived"
        QT_MOC_LITERAL(301, 9),  // "folder_id"
        QT_MOC_LITERAL(311, 19),  // "QList<FileNodeInfo>"
        QT_MOC_LITERAL(331, 9),  // "structure"
        QT_MOC_LITERAL(341, 20),  // "folderShareInitiated"
        QT_MOC_LITERAL(362, 10),  // "session_id"
        QT_MOC_LITERAL(373, 11),  // "total_files"
        QT_MOC_LITERAL(385, 5),  // "files"
        QT_MOC_LITERAL(391, 18),  // "folderFileUploaded"
        QT_MOC_LITERAL(410, 9),  // "completed"
        QT_MOC_LITERAL(420, 20),  // "folderShareCompleted"
        QT_MOC_LITERAL(441, 17),  // "folderShareFailed"
        QT_MOC_LITERAL(459, 5),  // "error"
        QT_MOC_LITERAL(465, 19),  // "folderShareProgress"
        QT_MOC_LITERAL(485, 10),  // "percentage"
        QT_MOC_LITERAL(496, 6),  // "status"
        QT_MOC_LITERAL(503, 11)   // "onReadyRead"
    },
    "NetworkManager",
    "connectionStatus",
    "",
    "success",
    "msg",
    "loginSuccess",
    "loginFailed",
    "registerSuccess",
    "registerFailed",
    "fileListReceived",
    "data",
    "uploadStarted",
    "filename",
    "uploadProgress",
    "downloadStarted",
    "downloadComplete",
    "shareResult",
    "deleteResult",
    "renameResult",
    "logoutSuccess",
    "transferProgress",
    "current",
    "total",
    "folderStructureReceived",
    "folder_id",
    "QList<FileNodeInfo>",
    "structure",
    "folderShareInitiated",
    "session_id",
    "total_files",
    "files",
    "folderFileUploaded",
    "completed",
    "folderShareCompleted",
    "folderShareFailed",
    "error",
    "folderShareProgress",
    "percentage",
    "status",
    "onReadyRead"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_NetworkManager[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      22,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      21,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,  146,    2, 0x06,    1 /* Public */,
       5,    0,  151,    2, 0x06,    4 /* Public */,
       6,    1,  152,    2, 0x06,    5 /* Public */,
       7,    1,  155,    2, 0x06,    7 /* Public */,
       8,    1,  158,    2, 0x06,    9 /* Public */,
       9,    1,  161,    2, 0x06,   11 /* Public */,
      11,    1,  164,    2, 0x06,   13 /* Public */,
      13,    1,  167,    2, 0x06,   15 /* Public */,
      14,    1,  170,    2, 0x06,   17 /* Public */,
      15,    1,  173,    2, 0x06,   19 /* Public */,
      16,    2,  176,    2, 0x06,   21 /* Public */,
      17,    2,  181,    2, 0x06,   24 /* Public */,
      18,    2,  186,    2, 0x06,   27 /* Public */,
      19,    0,  191,    2, 0x06,   30 /* Public */,
      20,    2,  192,    2, 0x06,   31 /* Public */,
      23,    2,  197,    2, 0x06,   34 /* Public */,
      27,    3,  202,    2, 0x06,   37 /* Public */,
      31,    2,  209,    2, 0x06,   41 /* Public */,
      33,    1,  214,    2, 0x06,   44 /* Public */,
      34,    1,  217,    2, 0x06,   46 /* Public */,
      36,    2,  220,    2, 0x06,   48 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      39,    0,  225,    2, 0x08,   51 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    3,    4,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    3,    4,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong,   21,   22,
    QMetaType::Void, QMetaType::LongLong, 0x80000000 | 25,   24,   26,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, 0x80000000 | 25,   28,   29,   30,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   32,   22,
    QMetaType::Void, QMetaType::QString,   28,
    QMetaType::Void, QMetaType::QString,   35,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,   37,   38,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject NetworkManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_NetworkManager.offsetsAndSizes,
    qt_meta_data_NetworkManager,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_NetworkManager_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<NetworkManager, std::true_type>,
        // method 'connectionStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'loginSuccess'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loginFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'registerSuccess'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'registerFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'fileListReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'uploadStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'uploadProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'downloadStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'downloadComplete'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'shareResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'deleteResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'renameResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'logoutSuccess'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'transferProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'folderStructureReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<long long, std::false_type>,
        QtPrivate::TypeAndForceComplete<QList<FileNodeInfo>, std::false_type>,
        // method 'folderShareInitiated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QList<FileNodeInfo> &, std::false_type>,
        // method 'folderFileUploaded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'folderShareCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'folderShareFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'folderShareProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void NetworkManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NetworkManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->connectionStatus((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->loginSuccess(); break;
        case 2: _t->loginFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->registerSuccess((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->registerFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->fileListReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->uploadStarted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->uploadProgress((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->downloadStarted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->downloadComplete((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->shareResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 11: _t->deleteResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 12: _t->renameResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 13: _t->logoutSuccess(); break;
        case 14: _t->transferProgress((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 15: _t->folderStructureReceived((*reinterpret_cast< std::add_pointer_t<qlonglong>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<FileNodeInfo>>>(_a[2]))); break;
        case 16: _t->folderShareInitiated((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QList<FileNodeInfo>>>(_a[3]))); break;
        case 17: _t->folderFileUploaded((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 18: _t->folderShareCompleted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 19: _t->folderShareFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: _t->folderShareProgress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 21: _t->onReadyRead(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (NetworkManager::*)(bool , QString );
            if (_t _q_method = &NetworkManager::connectionStatus; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)();
            if (_t _q_method = &NetworkManager::loginSuccess; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(QString );
            if (_t _q_method = &NetworkManager::loginFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(QString );
            if (_t _q_method = &NetworkManager::registerSuccess; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(QString );
            if (_t _q_method = &NetworkManager::registerFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(QString );
            if (_t _q_method = &NetworkManager::fileListReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(QString );
            if (_t _q_method = &NetworkManager::uploadStarted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(QString );
            if (_t _q_method = &NetworkManager::uploadProgress; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(QString );
            if (_t _q_method = &NetworkManager::downloadStarted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(QString );
            if (_t _q_method = &NetworkManager::downloadComplete; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(bool , QString );
            if (_t _q_method = &NetworkManager::shareResult; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(bool , QString );
            if (_t _q_method = &NetworkManager::deleteResult; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(bool , QString );
            if (_t _q_method = &NetworkManager::renameResult; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)();
            if (_t _q_method = &NetworkManager::logoutSuccess; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(qint64 , qint64 );
            if (_t _q_method = &NetworkManager::transferProgress; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(long long , QList<FileNodeInfo> );
            if (_t _q_method = &NetworkManager::folderStructureReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(const QString & , int , const QList<FileNodeInfo> & );
            if (_t _q_method = &NetworkManager::folderShareInitiated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(int , int );
            if (_t _q_method = &NetworkManager::folderFileUploaded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 17;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(const QString & );
            if (_t _q_method = &NetworkManager::folderShareCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 18;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(const QString & );
            if (_t _q_method = &NetworkManager::folderShareFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 19;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(int , const QString & );
            if (_t _q_method = &NetworkManager::folderShareProgress; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 20;
                return;
            }
        }
    }
}

const QMetaObject *NetworkManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NetworkManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NetworkManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NetworkManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 22)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 22;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 22)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 22;
    }
    return _id;
}

// SIGNAL 0
void NetworkManager::connectionStatus(bool _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void NetworkManager::loginSuccess()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void NetworkManager::loginFailed(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void NetworkManager::registerSuccess(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void NetworkManager::registerFailed(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void NetworkManager::fileListReceived(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void NetworkManager::uploadStarted(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void NetworkManager::uploadProgress(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void NetworkManager::downloadStarted(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void NetworkManager::downloadComplete(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void NetworkManager::shareResult(bool _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void NetworkManager::deleteResult(bool _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void NetworkManager::renameResult(bool _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void NetworkManager::logoutSuccess()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void NetworkManager::transferProgress(qint64 _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void NetworkManager::folderStructureReceived(long long _t1, QList<FileNodeInfo> _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}

// SIGNAL 16
void NetworkManager::folderShareInitiated(const QString & _t1, int _t2, const QList<FileNodeInfo> & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void NetworkManager::folderFileUploaded(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void NetworkManager::folderShareCompleted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 18, _a);
}

// SIGNAL 19
void NetworkManager::folderShareFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 19, _a);
}

// SIGNAL 20
void NetworkManager::folderShareProgress(int _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 20, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
