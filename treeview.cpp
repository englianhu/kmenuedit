/*
 *   Copyright (C) 2000 Matthias Elter <elter@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <iostream.h>

#include <qdir.h>
#include <qheader.h>
#include <qstringlist.h>
#include <qfileinfo.h>
#include <qdragobject.h>
#include <qdatastream.h>
#include <qcstring.h>
#include <qpopupmenu.h>

#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kdesktopfile.h>
#include <kdebug.h>

#include "treeview.h"
#include "treeview.moc"

const char* clipboard_prefix = ".kmenuedit_clipboard/";

TreeItem::TreeItem(QListViewItem *parent, const QString& file)
  :QListViewItem(parent)
{
    _file = file;
}

TreeItem::TreeItem(QListViewItem *parent, QListViewItem *after, const QString& file)
  :QListViewItem(parent, after)
{
    _file = file;
}

TreeItem::TreeItem(QListView *parent, const QString& file)
  : QListViewItem(parent)
{
    _file = file;
}

TreeItem::TreeItem(QListView *parent, QListViewItem *after, const QString& file)
  : QListViewItem(parent, after)
{
    _file = file;
}

TreeView::TreeView( QWidget *parent, const char *name )
  : KListView(parent, name)
{
    setFrameStyle(QFrame::WinPanel | QFrame::Sunken);
    setAllColumnsShowFocus(true);
    setRootIsDecorated(true);
    setSorting(-1);
    setAcceptDrops(true);
    setDropVisualizer(true);
    setDragEnabled(true);
    
    addColumn("");
    header()->hide();

    connect(this, SIGNAL(dropped(QDropEvent*, QListViewItem*)),
	    SLOT(slotDropped(QDropEvent*, QListViewItem*)));
    
    connect(this, SIGNAL(clicked( QListViewItem* )),
	    SLOT(itemSelected( QListViewItem* )));
    
    connect(this, SIGNAL(rightButtonPressed(QListViewItem*, const QPoint&, int)),
	    SLOT(slotRMBPressed(QListViewItem*, const QPoint&)));
    
    fill();
}

void TreeView::fill()
{
    clear();
    fillBranch("", 0);
}

void TreeView::fillBranch(const QString& rPath, TreeItem *parent)
{
    // get rid of leading slash in the relative path
    QString relPath = rPath;
    if(relPath[0] == '/')
	relPath = relPath.mid(1, relPath.length());

    // I don't use findAllResources as subdirectories are not recognised as resources
    // and therefore I already have to iterate by hand to get the subdir list.
    QStringList dirlist = dirList(relPath);
    QStringList filelist = fileList(relPath);

    //for (QStringList::ConstIterator it = dirlist.begin(); it != dirlist.end(); ++it)
    //  cout << (*it).local8Bit() << endl;

    // first add tree items for the desktop files in this directory
    if (!filelist.isEmpty()) {
	QStringList::ConstIterator it = filelist.end();
	do{
	    --it;

	    KDesktopFile df(*it);
	    if(df.readBoolEntry("Hidden") == true)
		continue;

	    TreeItem* item;
	    if (parent == 0) item = new TreeItem(this, *it);
	    else item = new TreeItem(parent, *it);

	    item->setText(0, df.readName());
	    item->setPixmap(0, KGlobal::iconLoader()->
			    loadIcon(df.readIcon(),KIcon::Desktop, KIcon::SizeSmall));
	}
	while (it != filelist.begin());
    }

    // add directories and process sudirs
    if (!dirlist.isEmpty()) {
	QStringList::ConstIterator it = dirlist.end();
	do{
	    --it;

	    QString dirFile = KGlobal::dirs()->findResource("apps", *it + "/.directory");
	    TreeItem* item;

	    if (dirFile.isNull())
		{
		    if (parent == 0)
			item = new TreeItem(this, *it + "/.directory");
		    else
			item = new TreeItem(parent, *it + "/.directory");
		    item->setText(0, *it);
		    item->setPixmap(0, KGlobal::iconLoader()->
				    loadIcon("package",KIcon::Desktop, KIcon::SizeSmall));
		}
	    else
		{
		    KDesktopFile df(dirFile);
		    if(df.readBoolEntry("Hidden") == true)
			continue;

		    if (parent == 0)
			item = new TreeItem(this,  *it + "/.directory");
		    else
			item = new TreeItem(parent, *it + "/.directory");
	
		    item->setText(0, df.readName());
		    item->setPixmap(0, KGlobal::iconLoader()
				    ->loadIcon(df.readIcon(),KIcon::Desktop, KIcon::SizeSmall));
		}
	    fillBranch(*it, item);
	}
	while (it != dirlist.begin());
    }
}

void TreeView::itemSelected(QListViewItem *item)
{
    if(!item) return;
    
    cout << " SELECTED: " << ((TreeItem*)item)->file().local8Bit() << endl;
    emit entrySelected(((TreeItem*)item)->file());
}

void TreeView::currentChanged()
{
    TreeItem *item = (TreeItem*)selectedItem();
    if (item == 0) return;

    KDesktopFile df(item->file());
    item->setText(0, df.readName());
    item->setPixmap(0, KGlobal::iconLoader()
		    ->loadIcon(df.readIcon(),KIcon::Desktop, KIcon::SizeSmall));
}

void TreeView::copyFile(const QString& src, const QString& dest)
{
    // We can't simply copy a .desktop file as several prefixes might
    // contain a version of that file. To make sure to copy all groups
    // and keys we read all groups and keys via KConfig which handles
    // the merging. We then write out the destination .desktop file
    // in a writeable prefix we get using locateLocal().

    if (src == dest) return;

    cout << "copyFile: " << src.local8Bit() << " to " << dest.local8Bit() << endl;		

    // read-only + don't merge in kdeglobals
    KConfig s(locate("apps", src), true, false);
    KSimpleConfig d(locateLocal("apps", dest));

    // don't copy hidden files
    if(s.readBoolEntry("Hidden", false) == true)
	return;

    // loop through all groups
    QStringList groups = s.groupList();
    for (QStringList::ConstIterator it = groups.begin(); it != groups.end(); ++it)
	{
	    if(*it == "<default>")
		continue;

	    if((*it).contains("Desktop Entry"))
		d.setDesktopGroup();
	    else
		d.setGroup(*it);

	    // get a map of keys/value pairs
	    QMap<QString, QString> map = s.entryMap(*it);

	    // iterate through the map and write out key/value pairs to the dest file
	    QMap<QString, QString>::ConstIterator it;
	    for (it = map.begin(); it != map.end(); ++it)
		d.writeEntry(it.key(), it.data());
	}

    // unset "Hidden"
    d.setDesktopGroup();
    d.writeEntry("Hidden", false);

    d.sync();
}

void TreeView::copyDir(const QString& s, const QString& d)
{
    // We create the destination directory in a writeable prefix returned
    // by locateLocal(), copy the .directory and the .desktop files over.
    // Then process the subdirs.

    QString src = s;
    QString dest = d;

    // truncate "/.directory"
    int pos = src.findRev("/.directory");
    if (pos > 0) src.truncate(pos);
    pos = dest.findRev("/.directory");
    if (pos > 0) dest.truncate(pos);

    if (src == dest) return;

    cout << "copyDir: " << src.local8Bit() << " to " << dest.local8Bit() << endl;

    QStringList dirlist = dirList(src);
    QStringList filelist = fileList(src);

    // copy .directory file
    copyFile(src + "/.directory", dest + "/.directory");

    cout << "###" << dest.local8Bit() << endl;
    // copy files
    for (QStringList::ConstIterator it = filelist.begin(); it != filelist.end(); ++it)
	{
	    QString file = (*it).mid((*it).findRev('/'), (*it).length());
	    copyFile(src + "/" + file, dest + "/" + file);
	}

    // process subdirs
    for (QStringList::ConstIterator it = dirlist.begin(); it != dirlist.end(); ++it)
	{
	    QString file = (*it).mid((*it).findRev('/'), (*it).length());
	    copyDir(src + "/" + file, dest + "/" + file);
	}

    // unset hidden flag
    KSimpleConfig c(locateLocal("apps", dest + "/.directory"));
    c.setDesktopGroup();
    c.writeEntry("Hidden", false);
    c.sync();
}

void TreeView::deleteFile(const QString& deskfile)
{
    // We search for the file in all prefixes and remove all writeable
    // ones. If we were not able to remove all (because of lack of permissons)
    // we set the "Hidden" flag in a writeable local file in a path returned
    // by localeLocal().
    bool allremoved = true;

    // search the selected item in all resource dirs
    QStringList resdirs = KGlobal::dirs()->resourceDirs("apps");
    for (QStringList::ConstIterator it = resdirs.begin(); it != resdirs.end(); ++it)
	{
	    QFile f((*it) + "/" + deskfile);

	    // continue if it does not exist in this resource dir
	    if(!f.exists()) continue;

	    // remove all writeable files
	    if(!f.remove())
		allremoved = false;
	}

    // if we did not have the permissions to remove all files we set a hidden flag
    // in the local one.
    if(!allremoved)
	{
	    KSimpleConfig c(locateLocal("apps", deskfile));
	    c.setDesktopGroup();
	    c.writeEntry("Name", "empty");
	    c.writeEntry("Hidden", true);
	    c.sync();
	}
}

void TreeView::deleteDir(const QString& d)
{
    // We delete all .desktop files and then process with the subdirs.
    // Afterwards the .directory file gets deleted from all prefixes
    // and we try to rmdir the directory in all prefixes.
    // If we don't succed in deleting the directory from all prefixes
    // we add a .directory file with the "Hidden" flag set in a local
    // writeable dir return by locateLocal().
    bool allremoved = true;

    QString directory = d;

    // truncate "/.directory"
    int pos = directory.findRev("/.directory");
    if (pos > 0) directory.truncate(pos);

    cout << "deleteDir: " << directory.local8Bit() << endl;

    QStringList dirlist = dirList(directory);
    QStringList filelist = fileList(directory);

    // delete files
    for (QStringList::ConstIterator it = filelist.begin(); it != filelist.end(); ++it)
	deleteFile(*it);

    // process subdirs
    for (QStringList::ConstIterator it = dirlist.begin(); it != dirlist.end(); ++it)
	deleteDir(*it);

    // delete .directory file in all prefixes
    deleteFile(directory + "/.directory");

    // try to rmdir the directory in all prefixes
    QDir dir;
    QStringList dirs = KGlobal::dirs()->findDirs("apps", directory);
    for (QStringList::ConstIterator it = dirs.begin(); it != dirs.end(); ++it)
	{
	    // remove all writeable files
	    if(!dir.rmdir(*it))
		allremoved = false;
	}

    // set a local hidden flag if not all dirs could be deleted
    if(!allremoved)
	{
	    KSimpleConfig c(locateLocal("apps", directory + "/.directory"));
	    c.setDesktopGroup();
	    c.writeEntry("Name", "empty");
	    c.writeEntry("Hidden", true);
	    c.sync();
	}
}

QStringList TreeView::fileList(const QString& rPath)
{
    QString relativePath = rPath;

    // truncate "/.directory"
    int pos = relativePath.findRev("/.directory");
    if (pos > 0) relativePath.truncate(pos);

    QStringList filelist;

    // loop through all resource dirs and build a file list
    QStringList resdirlist = KGlobal::dirs()->resourceDirs("apps");
    for (QStringList::ConstIterator it = resdirlist.begin(); it != resdirlist.end(); ++it)
	{
	    QDir dir((*it) + "/" + relativePath);
	    if(!dir.exists()) continue;

	    dir.setFilter(QDir::Files);
	    dir.setNameFilter("*.desktop");

	    // build a list of files
	    QStringList files = dir.entryList();
	    for (QStringList::ConstIterator it = files.begin(); it != files.end(); ++it)
		{
		    // does not work?!
		    //if (filelist.contains(*it)) continue;

		    if (relativePath == "")
			{
			    filelist.remove(*it); // hack
			    filelist.append(*it);
			}
		    else
			{
			    filelist.remove(relativePath + "/" + *it); //hack
			    filelist.append(relativePath + "/" + *it);
			}
		}
	}
    return filelist;
}

QStringList TreeView::dirList(const QString& rPath)
{
    QString relativePath = rPath;

    // truncate "/.directory"
    int pos = relativePath.findRev("/.directory");
    if (pos > 0) relativePath.truncate(pos);

    QStringList dirlist;

    // loop through all resource dirs and build a subdir list
    QStringList resdirlist = KGlobal::dirs()->resourceDirs("apps");
    for (QStringList::ConstIterator it = resdirlist.begin(); it != resdirlist.end(); ++it)
	{
	    QDir dir((*it) + "/" + relativePath);
	    if(!dir.exists()) continue;
	    dir.setFilter(QDir::Dirs);

	    // build a list of subdirs
	    QStringList subdirs = dir.entryList();
	    for (QStringList::ConstIterator it = subdirs.begin(); it != subdirs.end(); ++it)
		{
		    if ((*it) == "." || (*it) == "..") continue;
		    // does not work?!
		    // if (dirlist.contains(*it)) continue;
	
		    if (relativePath == "")
			{
			    dirlist.remove(*it); //hack
			    dirlist.append(*it);
			}
		    else
			{
			    dirlist.remove(relativePath + "/" + *it); //hack
			    dirlist.append(relativePath + "/" + *it);
			}
		}
	}
    return dirlist;
}

bool TreeView::acceptDrag(QDropEvent* event) const
{
    return (QString(event->format()).contains("text/plain"));
}

void TreeView::slotDropped (QDropEvent * e, QListViewItem *after)
{
    if(!e) return;

    // first move the item in the listview
    TreeItem *item = (TreeItem*)selectedItem();
    QListViewItem* parent = 0;

    if(after){
	if(after->isOpen()) {
	    parent = after;
	    after = 0;
	}
	else
	    parent = after->parent();
    }
    moveItem(item, parent, after);
    setSelected(item, true);

    // get source path from qdropevent
    QByteArray a = e->encodedData("text/plain");
    if (a.isEmpty()) return;
    QString src(a);

    bool isDir = src.find(".directory") > 0;

    kdDebug() << "src: " << src.local8Bit() << endl;

    // get source file
    int pos = src.findRev('/');
    if (isDir)
	pos = src.findRev('/', pos-1);
    QString srcfile;

    if (pos < 0)
	srcfile = src;
    else
	srcfile = src.mid(pos + 1, src.length());

    // get dest path
    QString dest;
    if (item->parent())
	dest = ((TreeItem*)item->parent())->file();

    // truncate file
    pos = dest.findRev('/');
    if (pos > 0) dest.truncate(pos);

    if(dest.isNull())
	dest = srcfile;
    else
	dest += '/' + srcfile;

    kdDebug() << "dest: " << dest.local8Bit() << endl;

    if(isDir)
	{
	    if (src != dest)
		{
		    copyDir(src, dest);
		    deleteDir(src);
		}
	}
    else
	{
	    if (src != dest)
		{
		    copyFile(src, dest);
		    deleteFile(src);
		}
	}

    item->setFile(dest);
}

QDragObject *TreeView::dragObject() const
{
    TreeItem *item = (TreeItem*)selectedItem();
    if(item == 0) return 0;

    QTextDrag *d = new QTextDrag(item->file(), (QWidget*)this);
    d->setPixmap(*item->pixmap(0));
    return d;
}

void TreeView::slotRMBPressed(QListViewItem*, const QPoint& p)
{
    TreeItem *item = (TreeItem*)selectedItem();
    if(item == 0) return;
    
    QPopupMenu mnu(this);
    
    // 42 is the answer to everything
    mnu.insertItem(i18n("C&ut") , 420);
    mnu.insertItem(i18n("&Copy"), 421);
    mnu.insertItem(i18n("&Paste"),422);
    mnu.insertSeparator();
    mnu.insertItem(i18n("&Delete"), 423);
    mnu.insertSeparator();
    mnu.insertItem(i18n("New &Item"), 424);
    mnu.insertItem(i18n("New &Submenu"), 425);
    
    int result = mnu.exec(p);
    
    switch (result)
	{
	case 420:
	    cut();
	    break;
	case 421:
	    copy();
	    break;
	case 422:
	    paste();
	    break;
	case 423:
	    del();
	    break;
	case 424:
	    newitem();
	    break;
	case 425:
	    newsubmenu();
	    break;
	}
}

void TreeView::newsubmenu()
{
    TreeItem *item = (TreeItem*)selectedItem();
    
    QListViewItem* parent = 0;
    QListViewItem* after = 0;
    
    QString sfile;
    
    if(item){
	if(item->childCount() > 0) {
	    parent = item;
	}
	else
	    {
		parent = item->parent();
		after = item;
	    }
	
	sfile = item->file();
    }
    
    QString dir = sfile;
    
    if(sfile.find(".directory") > 0)
	{
	    // truncate "blah/.directory"
	    
	    int pos = dir.findRev('/');
	    int pos2 = dir.findRev('/', pos-1);
	    
	    if (pos2 >= 0)
		pos = pos2;
	
	    if (pos > 0)
		dir.truncate(pos);
	}
    else if (dir.find(".desktop"))
	{
	    // truncate "blah.desktop"
	    int pos = dir.findRev('/');
	    
	    if (pos > 0)
		dir.truncate(pos);
	}
    dir += "/NewSubmenu/.directory";
	    
    TreeItem* newitem;
	
    if (!parent) 
	newitem = new TreeItem(this, after, dir);
    else
	newitem = new TreeItem(parent, after, dir);
    
    newitem->setText(0, "New Submenu");
    newitem->setPixmap(0, KGlobal::iconLoader()->loadIcon("package", KIcon::Desktop, KIcon::SizeSmall));
    
    KSimpleConfig c(locateLocal("apps", dir));
    c.setDesktopGroup();
    c.writeEntry("Name", "New Submenu");
    c.writeEntry("Icon", "package");
    c.sync();
}

void TreeView::newitem()
{
    TreeItem *item = (TreeItem*)selectedItem();
    
    QListViewItem* parent = 0;
    QListViewItem* after = 0;
    
    QString sfile;
    
    if(item){
	if(item->childCount() > 0) {
	    parent = item;
	}	
	else
	    {
		parent = item->parent();
		after = item;
	    }		
	
	sfile = item->file();
    }
    
    QString dir = sfile;
    
    // truncate ".directory" or "blah.desktop"
    
    int pos = dir.findRev('/');
    
    if (pos > 0)
	dir.truncate(pos);

    dir += "/NewFile.desktop";
	    
    TreeItem* newitem;
	
    if (!parent) 
	newitem = new TreeItem(this, after, dir);
    else
	newitem = new TreeItem(parent, after, dir);
    
    newitem->setText(0, "New File");
    newitem->setPixmap(0, KGlobal::iconLoader()->loadIcon("unkown", KIcon::Desktop, KIcon::SizeSmall));
    
    KSimpleConfig c(locateLocal("apps", dir));
    c.setDesktopGroup();
    c.writeEntry("Name", "New File");
    c.writeEntry("Icon", "file");
    c.sync();
}

void TreeView::cut()
{
    copy();
    del();
}

void TreeView::copy()
{
    TreeItem *item = (TreeItem*)selectedItem();

    // nil selected? -> nil to copy
     if (item == 0) return;

    // clean up old stuff
    cleanupClipboard();

    QString file = item->file();

    // is file a .directory or a .desktop file
    if(file.find(".directory") > 0)
	{
	    _clipboard = file;
		
	    // truncate path
	    int pos = _clipboard.findRev('/');
	    int pos2 = _clipboard.findRev('/', pos-1);
	    if (pos2 >= 0)
		pos = pos2+1;
	    else
		pos = 0;
	
	    if (pos > 0)
		_clipboard = _clipboard.mid(pos, _clipboard.length());
	
	    copyDir(file, QString(clipboard_prefix) + _clipboard);
	}
    else if (file.find(".desktop"))
	{
	    _clipboard = file;
		
	    // truncate path
	    int pos = _clipboard.findRev('/');
	    if (pos >= 0)
		_clipboard = _clipboard.mid(pos+1, _clipboard.length());
	
	    copyFile(file, QString(clipboard_prefix) + _clipboard);
	}
}

void TreeView::paste()
{
    TreeItem *item = (TreeItem*)selectedItem();

    // nil selected? -> nil to paste to
    if (item == 0) return;

    // is there content in the clipboard?
    if (_clipboard.isEmpty()) return;

    // get dest
    QString dest = item->file();

    // truncate ".directory"
    int pos = dest.findRev(".directory");
    if (pos > 0) dest.truncate(pos);

    // truncate '/'
    pos = dest.findRev('/');
    if (pos > 0) dest.truncate(pos);

    // truncate ".desktop" and file name
    pos = dest.findRev(".desktop");
    if (pos > 0)
	{
	    dest.truncate(pos);
	
	    // truncate file name
	    pos = dest.findRev('/');
	    if (pos < 0) pos = 0;
	    dest.truncate(pos);
	}

    cout << "### clip: " << _clipboard.local8Bit() << " dest: " << dest.local8Bit() << " ###" << endl;

    // is _clipboard a .directory or a .desktop file
    if(_clipboard.find(".directory") > 0)
	{
	    copyDir(QString(clipboard_prefix) + _clipboard, dest + '/' + _clipboard);
	}
    else if (_clipboard.find(".desktop"))
	{
	    copyFile(QString(clipboard_prefix) + _clipboard, dest + '/' + _clipboard);
	}

    // create the TreeItems:

    QListViewItem* parent = 0;

    if(item){
	if(item->childCount() > 0) {
	    parent = item;
	    item = 0;
	}
	else
	    parent = item->parent();	
    }
	
    TreeItem* newitem;
    if (!parent)
	newitem = new TreeItem(this, item, "");
    else
	newitem = new TreeItem(parent, item, "");
	
    KDesktopFile df(locateLocal("apps", dest + '/' + _clipboard));
	
    newitem->setText(0, df.readName());
    if(!dest.isEmpty())
	newitem->setFile(dest + '/' + _clipboard);
    else
	newitem->setFile(_clipboard);
    newitem->setPixmap(0, KGlobal::iconLoader()->loadIcon(df.readIcon(),KIcon::Desktop, KIcon::SizeSmall));


    fillBranch(newitem->file(), newitem);
}

void TreeView::del()
{	
    TreeItem *item = (TreeItem*)selectedItem();

    // nil selected? -> nil to delete
    if (item == 0) return;

    QString file = item->file();

    // is file a .directory or a .desktop file
    if(file.find(".directory") > 0)
	{
	    deleteDir(file.mid(0, file.find("/.directory")));
	    delete item;
	}
    else if (file.find(".desktop"))
	{
	    deleteFile(file);
	    delete item;
	}
}

void TreeView::cleanupClipboard()
{
    // TODO
}