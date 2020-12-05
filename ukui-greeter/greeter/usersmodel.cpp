/* usersmodel.cpp
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
#include "usersmodel.h"
#include <QLightDM/UsersModel>
#include <QDebug>
#include "securityuser.h"

UsersModel::UsersModel(bool hideUsers, QObject *parent) :
    ProxyModel(parent),
    secUser(SecurityUser::instance()),
    m_showGuest(false)
{
    if(!hideUsers){
        setSourceModel(new QLightDM::UsersModel(this));
        if(getSecUserCount() == 0){
            setShowManualLogin(true);
        }
    }
    else {
        qDebug() << "hide users, show manual";
        setShowManualLogin(true);
    }
}

void UsersModel::setShowGuest(bool isShowGuest)
{
    if(m_showGuest == isShowGuest)
        return;
    m_showGuest = isShowGuest;
    if(m_showGuest)
    {
        QStandardItem *guest = new QStandardItem(tr("Guest"));
        guest->setData("*guest", QLightDM::UsersModel::NameRole);
        guest->setData(tr("Guest"), QLightDM::UsersModel::RealNameRole);
        extraRowModel()->appendRow(guest);
    }
    else
    {
        QStandardItemModel *model = extraRowModel();
        for(int i = 0; i < model->rowCount(); i++){
            QStandardItem *item = model->item(i, 0);
            if(item->text() == tr("Guest Session")){
                model->removeRow(i);
            }
        }
    }
}

bool UsersModel::showGuest() const
{
    return m_showGuest;
}

void UsersModel::setShowManualLogin(bool isShowManualLogin)
{
    if(m_showManualLogin == isShowManualLogin)
        return;
    m_showManualLogin = isShowManualLogin;
    if(m_showManualLogin){
        QStandardItem *manualLogin = new QStandardItem(tr("Login"));
        manualLogin->setData("*login", QLightDM::UsersModel::NameRole);
        manualLogin->setData(tr("Login"), QLightDM::UsersModel::RealNameRole);
        extraRowModel()->appendRow(manualLogin);
    }
    else{
        QStandardItemModel *model = extraRowModel();
        for(int i = 0; i < model->rowCount(); i++){
            QStandardItem *item = model->item(i, 0);
            if(item->text() == tr("Login")){
                model->removeRow(i);
            }
        }
    }
}

int UsersModel::getSecUserCount()
{
    int count = 0;
    for(int i = 0; i < this->rowCount(); i++){
        QString name = this->index(i).data(QLightDM::UsersModel::NameRole).toString();
        if(secUser->isSecrityUser(name))
            count++;
    }
    return count;
}

bool UsersModel::showManualLogin() const
{
    return m_showManualLogin;
}
