/****************************************************************************
** window meta object code from reading C++ file 'window.h'
**
** Created: Tue May 17 23:57:14 2011
**      by: The Qt MOC ($Id: qt/moc_yacc.cpp   3.3.8   edited Feb 2 14:59 $)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "window.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.3.8b. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *window::className() const
{
    return "window";
}

QMetaObject *window::metaObj = 0;
static QMetaObjectCleanUp cleanUp_window( "window", &window::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString window::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "window", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString window::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "window", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* window::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QWidget::staticMetaObject();
    static const QUMethod slot_0 = {"get_data1", 0, 0 };
    static const QUMethod slot_1 = {"get_data2", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "get_data1()", &slot_0, QMetaData::Private },
	{ "get_data2()", &slot_1, QMetaData::Private }
    };
    metaObj = QMetaObject::new_metaobject(
	"window", parentObject,
	slot_tbl, 2,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_window.setMetaObject( metaObj );
    return metaObj;
}

void* window::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "window" ) )
	return this;
    return QWidget::qt_cast( clname );
}

bool window::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: get_data1(); break;
    case 1: get_data2(); break;
    default:
	return QWidget::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool window::qt_emit( int _id, QUObject* _o )
{
    return QWidget::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool window::qt_property( int id, int f, QVariant* v)
{
    return QWidget::qt_property( id, f, v);
}

bool window::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
