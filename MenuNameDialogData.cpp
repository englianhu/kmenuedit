/**********************************************************************

	--- Qt Architect generated file ---

	File: MenuNameDialogData.cpp
	Last generated: Fri Nov 28 14:52:53 1997

	DO NOT EDIT!!!  This file will be automatically
	regenerated by qtarch.  All changes will be lost.

 *********************************************************************/

#include "MenuNameDialogData.h"

#define Inherited QDialog

#include <qlabel.h>

MenuNameDialogData::MenuNameDialogData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	QLabel* dlgedit_Label_1;
	dlgedit_Label_1 = new QLabel( this, "Label_1" );
	dlgedit_Label_1->setGeometry( 16, 16, 328, 24 );
	dlgedit_Label_1->setMinimumSize( 10, 10 );
	dlgedit_Label_1->setMaximumSize( 32767, 32767 );
	dlgedit_Label_1->setText( "Change the name of the menus as displayed in kpanel" );
	dlgedit_Label_1->setAlignment( 289 );
	dlgedit_Label_1->setMargin( -1 );

	QLabel* dlgedit_Label_2;
	dlgedit_Label_2 = new QLabel( this, "Label_2" );
	dlgedit_Label_2->setGeometry( 24, 48, 56, 24 );
	dlgedit_Label_2->setMinimumSize( 10, 10 );
	dlgedit_Label_2->setMaximumSize( 32767, 32767 );
	dlgedit_Label_2->setText( "Personal:" );
	dlgedit_Label_2->setAlignment( 290 );
	dlgedit_Label_2->setMargin( -1 );

	QLabel* dlgedit_Label_3;
	dlgedit_Label_3 = new QLabel( this, "Label_3" );
	dlgedit_Label_3->setGeometry( 24, 88, 56, 16 );
	dlgedit_Label_3->setMinimumSize( 10, 10 );
	dlgedit_Label_3->setMaximumSize( 32767, 32767 );
	dlgedit_Label_3->setText( "Default:" );
	dlgedit_Label_3->setAlignment( 290 );
	dlgedit_Label_3->setMargin( -1 );

	i_personal = new QLineEdit( this, "LineEdit_1" );
	i_personal->setGeometry( 96, 48, 232, 24 );
	i_personal->setMinimumSize( 10, 10 );
	i_personal->setMaximumSize( 32767, 32767 );
	i_personal->setText( "" );
	i_personal->setMaxLength( 32767 );
	i_personal->setEchoMode( QLineEdit::Normal );
	i_personal->setFrame( TRUE );

	i_default = new QLineEdit( this, "LineEdit_2" );
	i_default->setGeometry( 96, 80, 232, 24 );
	i_default->setMinimumSize( 10, 10 );
	i_default->setMaximumSize( 32767, 32767 );
	i_default->setText( "" );
	i_default->setMaxLength( 32767 );
	i_default->setEchoMode( QLineEdit::Normal );
	i_default->setFrame( TRUE );

	b_ok = new QPushButton( this, "PushButton_1" );
	b_ok->setGeometry( 40, 128, 80, 24 );
	b_ok->setMinimumSize( 10, 10 );
	b_ok->setMaximumSize( 32767, 32767 );
	connect( b_ok, SIGNAL(pressed()), SLOT(accept()) );
	b_ok->setText( "Ok" );
	b_ok->setAutoRepeat( FALSE );
	b_ok->setAutoResize( FALSE );

	b_cancel = new QPushButton( this, "PushButton_2" );
	b_cancel->setGeometry( 232, 128, 80, 24 );
	b_cancel->setMinimumSize( 10, 10 );
	b_cancel->setMaximumSize( 32767, 32767 );
	connect( b_cancel, SIGNAL(pressed()), SLOT(reject()) );
	b_cancel->setText( "Cancel" );
	b_cancel->setAutoRepeat( FALSE );
	b_cancel->setAutoResize( FALSE );

	resize( 352,168 );
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
}


MenuNameDialogData::~MenuNameDialogData()
{
}