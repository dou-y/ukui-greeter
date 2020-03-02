/* usersview.h
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
**/
#ifndef USERSVIEW_H
#define USERSVIEW_H

#include <QWidget>
#include <QTime>
#include "userentry.h"

#define ITEM_WIDTH (CENTER_ENTRY_WIDTH + 31 * scale)
#define ITEM_HEIGHT CENTER_ENTRY_HEIGHT
#define USERSVIEW_WIDTH (ITEM_WIDTH * 5 + 2)
#define USERSVIEW_HEIGHT ITEM_HEIGHT

class QListWidget;
class QPushButton;
class QAbstractListModel;
class UsersView : public QWidget
{
    Q_OBJECT

public:
    explicit UsersView(QWidget *parent = 0);
    ~UsersView();
    void initUI();
    void setModel(QAbstractListModel *);
    void setCurrentUser(const QString &userName, bool entered = false);
    void pageUp();
    void pageDown();

protected:
    void resizeEvent(QResizeEvent *event);
//    void keyReleaseEvent(QKeyEvent *event);
    void showEvent(QShowEvent *event);

private slots:
    void onUserPressed();
    void onUserClicked(int row);
    void onUserAdded(const QModelIndex&, int, int);
    void onUserRemoved(const QModelIndex&, int, int );
    void onUserChanged(const QModelIndex&, const QModelIndex&);
    void setCurrentRow(int row);
    void onGlobalKeyRelease(const QString &key);

signals:
    void userSelected(const QModelIndex&);
    void currentUserChanged(const QModelIndex&);
    void userNotFound(const QString &userName);

private:
    void insertUserEntry(int row);
    void moveUserEntry(int from,int to);
    void moveCurrentToCenter(int row);

private:
    QAbstractListModel *usersModel;

    QListWidget *usersList;

    QPushButton *addButton;
    QPushButton *remButton;
    QPushButton *insertButton;
    QPushButton *zomInButton;
    QPushButton *zomOutButton;
    int lastClickTime;
    QTime time;
};

#endif // USERSVIEW_H