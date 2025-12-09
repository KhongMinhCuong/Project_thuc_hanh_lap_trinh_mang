/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_MainWindow_t {
    uint offsetsAndSizes[46];
    char stringdata0[11];
    char stringdata1[20];
    char stringdata2[1];
    char stringdata3[18];
    char stringdata4[17];
    char stringdata5[16];
    char stringdata6[18];
    char stringdata7[15];
    char stringdata8[16];
    char stringdata9[16];
    char stringdata10[13];
    char stringdata11[6];
    char stringdata12[19];
    char stringdata13[15];
    char stringdata14[5];
    char stringdata15[21];
    char stringdata16[4];
    char stringdata17[23];
    char stringdata18[9];
    char stringdata19[18];
    char stringdata20[8];
    char stringdata21[19];
    char stringdata22[13];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_MainWindow_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
        QT_MOC_LITERAL(0, 10),  // "MainWindow"
        QT_MOC_LITERAL(11, 19),  // "onConnectBtnClicked"
        QT_MOC_LITERAL(31, 0),  // ""
        QT_MOC_LITERAL(32, 17),  // "onLoginBtnClicked"
        QT_MOC_LITERAL(50, 16),  // "onRefreshClicked"
        QT_MOC_LITERAL(67, 15),  // "onUploadClicked"
        QT_MOC_LITERAL(83, 17),  // "onDownloadClicked"
        QT_MOC_LITERAL(101, 14),  // "onShareClicked"
        QT_MOC_LITERAL(116, 15),  // "onDeleteClicked"
        QT_MOC_LITERAL(132, 15),  // "onLogoutClicked"
        QT_MOC_LITERAL(148, 12),  // "onTabChanged"
        QT_MOC_LITERAL(161, 5),  // "index"
        QT_MOC_LITERAL(167, 18),  // "handleLoginSuccess"
        QT_MOC_LITERAL(186, 14),  // "handleFileList"
        QT_MOC_LITERAL(201, 4),  // "data"
        QT_MOC_LITERAL(206, 20),  // "handleUploadProgress"
        QT_MOC_LITERAL(227, 3),  // "msg"
        QT_MOC_LITERAL(231, 22),  // "handleDownloadComplete"
        QT_MOC_LITERAL(254, 8),  // "filename"
        QT_MOC_LITERAL(263, 17),  // "handleShareResult"
        QT_MOC_LITERAL(281, 7),  // "success"
        QT_MOC_LITERAL(289, 18),  // "handleDeleteResult"
        QT_MOC_LITERAL(308, 12)   // "handleLogout"
    },
    "MainWindow",
    "onConnectBtnClicked",
    "",
    "onLoginBtnClicked",
    "onRefreshClicked",
    "onUploadClicked",
    "onDownloadClicked",
    "onShareClicked",
    "onDeleteClicked",
    "onLogoutClicked",
    "onTabChanged",
    "index",
    "handleLoginSuccess",
    "handleFileList",
    "data",
    "handleUploadProgress",
    "msg",
    "handleDownloadComplete",
    "filename",
    "handleShareResult",
    "success",
    "handleDeleteResult",
    "handleLogout"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_MainWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  110,    2, 0x08,    1 /* Private */,
       3,    0,  111,    2, 0x08,    2 /* Private */,
       4,    0,  112,    2, 0x08,    3 /* Private */,
       5,    0,  113,    2, 0x08,    4 /* Private */,
       6,    0,  114,    2, 0x08,    5 /* Private */,
       7,    0,  115,    2, 0x08,    6 /* Private */,
       8,    0,  116,    2, 0x08,    7 /* Private */,
       9,    0,  117,    2, 0x08,    8 /* Private */,
      10,    1,  118,    2, 0x08,    9 /* Private */,
      12,    0,  121,    2, 0x08,   11 /* Private */,
      13,    1,  122,    2, 0x08,   12 /* Private */,
      15,    1,  125,    2, 0x08,   14 /* Private */,
      17,    1,  128,    2, 0x08,   16 /* Private */,
      19,    2,  131,    2, 0x08,   18 /* Private */,
      21,    2,  136,    2, 0x08,   21 /* Private */,
      22,    0,  141,    2, 0x08,   24 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   11,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::QString,   16,
    QMetaType::Void, QMetaType::QString,   18,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   20,   16,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   20,   16,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.offsetsAndSizes,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_MainWindow_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'onConnectBtnClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLoginBtnClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRefreshClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onUploadClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDownloadClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onShareClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDeleteClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLogoutClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTabChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'handleLoginSuccess'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handleFileList'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'handleUploadProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'handleDownloadComplete'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'handleShareResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'handleDeleteResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'handleLogout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onConnectBtnClicked(); break;
        case 1: _t->onLoginBtnClicked(); break;
        case 2: _t->onRefreshClicked(); break;
        case 3: _t->onUploadClicked(); break;
        case 4: _t->onDownloadClicked(); break;
        case 5: _t->onShareClicked(); break;
        case 6: _t->onDeleteClicked(); break;
        case 7: _t->onLogoutClicked(); break;
        case 8: _t->onTabChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->handleLoginSuccess(); break;
        case 10: _t->handleFileList((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->handleUploadProgress((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->handleDownloadComplete((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->handleShareResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 14: _t->handleDeleteResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 15: _t->handleLogout(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 16;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
