/* loginwindow.cpp
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
#include "loginwindow.h"
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QStandardPaths>
#include <QScreen>
#include <QProcess>
#include <QFocusEvent>
#include <QWindow>
#include <QLightDM/SessionsModel>
#include "globalv.h"

LoginWindow::LoginWindow(QSharedPointer<GreeterWrapper> greeter, QWidget *parent)
    : QWidget(parent),
      m_sessionsModel(new QLightDM::SessionsModel(QLightDM::SessionsModel::LocalSessions, this)),
      m_greeter(greeter),
      m_config(new QSettings(configFile, QSettings::IniFormat)),
      m_timer(new QTimer(this))
{    
    initUI();
    connect(m_greeter.data(), SIGNAL(showMessage(QString,QLightDM::Greeter::MessageType)),
            this, SLOT(onShowMessage(QString,QLightDM::Greeter::MessageType)));
    connect(m_greeter.data(), SIGNAL(showPrompt(QString,QLightDM::Greeter::PromptType)),
            this, SLOT(onShowPrompt(QString,QLightDM::Greeter::PromptType)));
    connect(m_greeter.data(), SIGNAL(authenticationComplete()),
            this, SLOT(onAuthenticationComplete()));
    connect(m_greeter.data(), SIGNAL(startSessionFailed()), this, SLOT(startAuthentication())); //启动会话失败，重新开始认证

    m_timer->setInterval(100);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updatePixmap()));
}

void LoginWindow::initUI()
{
    if (this->objectName().isEmpty())
        this->setObjectName(QStringLiteral("this"));
    this->resize(520, 135);

    /* 返回按钮 */
    m_backLabel = new QPushButton(this);
    m_backLabel->setObjectName(QStringLiteral("backButton"));
    m_backLabel->setGeometry(QRect(0, 0, 32, 32));
    m_backLabel->setFocusPolicy(Qt::NoFocus);
    connect(m_backLabel, &QPushButton::clicked, this, &LoginWindow::backToUsers);

    /* 头像 */
    m_faceLabel = new QLabel(this);
    m_faceLabel->setObjectName(QStringLiteral("faceLabel"));
    m_faceLabel->setGeometry(QRect(60, 0, 132, 132));
    m_faceLabel->setFocusPolicy(Qt::NoFocus);

    /* session选择按钮 */
    m_sessionLabel = new QPushButton(this);
    m_sessionLabel->setObjectName(QStringLiteral("sessionButton"));
    m_sessionLabel->setGeometry(QRect(width()-26, 0, 26, 26));
    m_sessionLabel->hide();
    connect(m_sessionLabel, &QPushButton::clicked, this, &LoginWindow::onSessionButtonClicked);

    /* 用户名 */
    m_nameLabel = new QLabel(this);
    m_nameLabel->setObjectName(QStringLiteral("nameLabel"));
    QRect nameRect(220, 0, 200, 25);
    m_nameLabel->setGeometry(nameRect);
    m_nameLabel->setFont(QFont("ubuntu", 12));
    m_nameLabel->setFocusPolicy(Qt::NoFocus);

    /* 是否已登录 */
    m_isLoginLabel = new QLabel(this);
    m_isLoginLabel->setObjectName(QStringLiteral("isLoginLabel"));
    QRect loginRect(220, nameRect.bottom()+5, 200, 30);
    m_isLoginLabel->setGeometry(loginRect);
    m_isLoginLabel->setFont(QFont("ubuntu", 10));
    m_isLoginLabel->setFocusPolicy(Qt::NoFocus);

    /* 密码框 */
    m_passwordEdit = new IconEdit(this);
    m_passwordEdit->setObjectName(QStringLiteral("passwordEdit"));
    QRect pwdRect(220, 90, 300, 40);
    m_passwordEdit->setGeometry(pwdRect);
    m_passwordEdit->resize(QSize(300, 40));
    m_passwordEdit->setFocusPolicy(Qt::StrongFocus);
    m_passwordEdit->installEventFilter(this);
    m_passwordEdit->hide(); //收到请求密码的prompt才显示出来
    connect(m_passwordEdit, SIGNAL(clicked(const QString&)), this, SLOT(onLogin(const QString&)));
}

void LoginWindow::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    if(!m_passwordEdit->isHidden())
        m_passwordEdit->setFocus();
}

void LoginWindow::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_S && (event->modifiers() & Qt::ControlModifier))
        m_sessionLabel->click();
    else if(event->key() == Qt::Key_Return){
        if(m_sessionLabel->hasFocus())
            m_sessionLabel->click();
    }
    QWidget::keyReleaseEvent(event);
}

/**
 * @brief LoginWindow::recover
 * 复原UI
 */
void LoginWindow::recover()
{
    m_nameLabel->clear();
    m_isLoginLabel->clear();
    m_passwordEdit->clear();
    m_passwordEdit->setType(QLineEdit::Password);
    m_passwordEdit->hide();
    m_passwordEdit->setWaiting(false);
    clearMessage();
}

void LoginWindow::clearMessage()
{
    //清除所有的message label
    for(int i = 0; i < m_messageLabels.size(); i++) {
        QLabel *msgLabel = m_messageLabels[i];
        delete(msgLabel);
    }
    m_messageLabels.clear();
    //恢复密码输入框的位置
    m_passwordEdit->move(m_passwordEdit->geometry().left(), 90);
    //恢复窗口大小
    this->resize(520, 135);
}

void LoginWindow::onSessionButtonClicked()
{
    Q_EMIT selectSession(m_session);
}

/**
 * @brief LoginWindow::backToUsers
 * 返回到用户列表窗口
 */
void LoginWindow::backToUsers()
{
    recover();
    Q_EMIT back();
}

/**
 * @brief LoginWindow::setUserName
 * @param userName 用户名
 *
 * 设置用户名
 */
void LoginWindow::setUserName(const QString& userName)
{
    m_nameLabel->setText(userName);
}

/**
 * @brief LoginWindow::userName
 * @return 当前的用户名
 *
 * 获取当前用户名
 */
QString LoginWindow::getUserName()
{
    if(m_nameLabel->text() == tr("Login"))
        return "Login";
    return m_nameLabel->text();
}

/**
 * @brief LoginWindow::setFace
 * @param faceFile 用户头像文件
 *
 * 设置用户头像
 */
void LoginWindow::setFace(const QString& facePath)
{
    QFile faceFile(facePath);
    QPixmap faceImage;
    //如果头像文件不存在，则使用默认头像
    if(faceFile.exists())
        faceImage = scaledPixmap(128, 128, facePath);
    else
        faceImage = scaledPixmap(128, 128, ":/resource/default_face.png");

    m_faceLabel->setPixmap(faceImage);
}

/**
 * @brief LoginWindow::setLoggedIn
 * @param isLoggedIn
 *
 * 设置当前用户是否已已经登录
 */
void LoginWindow::setLoggedIn(bool isLoggedIn)
{
    m_isLoginLabel->setText(isLoggedIn ? tr("logged in") : "");
}

/**
 * @brief LoginWindow::setPrompt
 * @param text
 *
 * 设置密码框的提示信息
 */
void LoginWindow::setPrompt(const QString& text)
{
    m_passwordEdit->setPrompt(text);
}

/**
 * @brief LoginWindow::password
 * @return
 *
 * 获取密码
 */
QString LoginWindow::getPassword()
{
    return m_passwordEdit->text();
}

/**
 * @brief LoginWindow::setSession
 *
 * 设置session图标
 */
void LoginWindow::setSession(QString session)
{
    QString sessionIcon;

    if(session.isEmpty() || sessionIndex(session) == -1) {
        /* default */
        QString defaultSession = m_greeter->defaultSessionHint();
        if(defaultSession != session && sessionIndex(defaultSession) != -1) {
            session = defaultSession;
        }
        /* first session in session list */
        else if(m_sessionsModel && m_sessionsModel->rowCount() > 0) {
            session = m_sessionsModel->index(0, 0).data(QLightDM::SessionsModel::KeyRole).toString();
        }
        else {
            session = "";
        }
    }
    m_session = session;
    m_greeter->setSession(m_session);

    sessionIcon = IMAGE_DIR + QString("badges/%1_badge.png").arg(session.toLower());
    QFile iconFile(sessionIcon);
    if(!iconFile.exists()){
        sessionIcon = IMAGE_DIR + QString("badges/unknown_badge.png");
    }
    m_sessionLabel->setIcon(QIcon(sessionIcon));
    m_sessionLabel->setIconSize(QSize(22, 22));
}

/**
 * @brief LoginWindow::session
 * @return
 *
 * 获取session标识
 */
QString LoginWindow::getSession()
{
    QString sessionKey;
    for(int i = 0; i < m_sessionsModel->rowCount(); i++) {
        QString session = m_sessionsModel->index(i, 0).data(Qt::DisplayRole).toString();
        if(m_session == session) {
            sessionKey = m_sessionsModel->index(i, 0).data(Qt::UserRole).toString();
            break;
        }
    }
    return sessionKey;
}

void LoginWindow::setUsersModel(QSharedPointer<QAbstractItemModel> model)
{
    if(model.isNull())
        return;
    m_usersModel = model;

    if(m_usersModel->rowCount() == 1) {
        m_backLabel->hide();
        setUserIndex(m_usersModel->index(0,0));
    }
}

bool LoginWindow::setUserIndex(const QModelIndex& index)
{
    if(!index.isValid()){
        return false;
    }
    //设置用户名
    QString name = index.data(Qt::DisplayRole).toString();
    m_name = index.data(QLightDM::UsersModel::NameRole).toString();
    setUserName(name);

    //设置头像
    QString facePath = index.data(QLightDM::UsersModel::ImagePathRole).toString();
    setFace(facePath);

    //显示是否已经登录
    bool isLoggedIn = index.data(QLightDM::UsersModel::LoggedInRole).toBool();
    setLoggedIn(isLoggedIn);

    //显示session图标
    if(!m_sessionsModel.isNull() && m_sessionsModel->rowCount() > 1) {
        m_sessionLabel->show();
        setSession(index.data(QLightDM::UsersModel::SessionRole).toString());
    }
    m_passwordEdit->hide();
    startAuthentication();

    return true;
}

void LoginWindow::setSessionsModel(QSharedPointer<QAbstractItemModel> model)
{
    if(model.isNull()){
        return ;
    }
    m_sessionsModel = model;
    //如果session有多个，则显示session图标，默认显示用户上次登录的session
    //如果当前还没有设置用户，则默认显示第一个session
    if(m_sessionsModel->rowCount() > 1) {
        if(!m_usersModel.isNull() && !m_nameLabel->text().isEmpty()) {
            for(int i = 0; i < m_usersModel->rowCount(); i++){
                QModelIndex index = m_usersModel->index(i, 0);
                if(index.data(Qt::DisplayRole).toString() == m_nameLabel->text()){
                    setSession(index.data(QLightDM::UsersModel::SessionRole).toString());
                    return;
                }
            }
        }
        setSession(m_sessionsModel->index(0, 0).data().toString());
    }
}

bool LoginWindow::setSessionIndex(const QModelIndex &index)
{
    //显示选择的session（如果有多个session则显示，否则不显示）
    if(!index.isValid()) {
        return false;
    }
    setSession(index.data(Qt::DisplayRole).toString());
    return true;
}

int LoginWindow::sessionIndex(const QString &session)
{
    if(!m_sessionsModel){
        return -1;
    }
    for(int i = 0; i < m_sessionsModel->rowCount(); i++){
        QString sessionName = m_sessionsModel->index(i, 0).data().toString();
        if(session.toLower() == sessionName.toLower()) {
            return i;
        }
    }
    return -1;
}

void LoginWindow::onSessionSelected(const QString &session)
{
    qDebug() << "select session: " << session;
    if(!session.isEmpty() && m_session != session) {
        m_session = session;
        m_greeter->setSession(m_session);
        setSession(m_session);
    }
}

void LoginWindow::startAuthentication()
{
    //用户认证
    if(m_name == "*guest") {                       //游客登录
        qDebug() << "guest login";
        m_passwordEdit->show();
        m_passwordEdit->showIconButton("*");
        m_passwordEdit->setWaiting(true);
        m_passwordEdit->setPrompt(tr("login"));
    }
    else if(m_name == "*login") {                  //手动输入用户名
        m_greeter->authenticate("");
    }
    else {
        qDebug() << "login: " << m_name;
        m_greeter->authenticate(m_name);
    }
}

/**
 * @brief LoginWindow::startWaiting
 *
 * 等待认证结果
 */
void LoginWindow::startWaiting()
{
    m_passwordEdit->setWaiting(true);   //等待认证结果期间不能再输入密码
    m_backLabel->setEnabled(false);
    m_sessionLabel->setEnabled(false);
    m_timer->start();
}

void LoginWindow::stopWaiting()
{
    m_timer->stop();
    m_passwordEdit->setWaiting(false);
    m_passwordEdit->setIcon(":/resource/arrow_right.png");
    m_backLabel->setEnabled(true);
    m_sessionLabel->setEnabled(true);
    m_passwordEdit->showIconButton("");
}

void LoginWindow::updatePixmap()
{
    QMatrix matrix;
    matrix.rotate(90.0);
    m_waiting = m_waiting.transformed(matrix, Qt::FastTransformation);
    m_passwordEdit->setIcon(QIcon(m_waiting));
}

void LoginWindow::saveLastLoginUser()
{
    m_greeter->setUserName(m_name);
    m_config->setValue("lastLoginUser", m_name);    //记录的是登录用户的真名
    m_config->sync();
}

void LoginWindow::onLogin(const QString &str)
{
    clearMessage();
//    QString name = m_nameLabel->text();
    qDebug() << str;
    if(m_name == "*guest") {
        m_greeter->authenticateAsGuest();
    }
    else if(m_name == "*login") {   //用户输入用户名
        m_name = str;
        m_nameLabel->setText(str);
        m_passwordEdit->setText("");
        m_passwordEdit->setType(QLineEdit::Password);
        m_greeter->authenticate(str);
        qDebug() << "login: " << str;
    }
    else {  //发送密码
        m_greeter->respond(str);
        m_waiting.load(":/resource/waiting.png");
        m_passwordEdit->showIconButton("*");  //当没有输入密码登录时，也显示等待提示
        m_passwordEdit->setIcon(QIcon(m_waiting));
        startWaiting();
    }
    m_passwordEdit->setText("");
}

void LoginWindow::onShowPrompt(QString text, QLightDM::Greeter::PromptType type)
{
    qDebug()<< "prompt: "<< text;
//    if(text == "") {    //显示生物识别窗口
//        int bioWid;
//        QWindow *bioWindow = QWindow::fromWinId(bioWid);
//        QWidget *bioWidget = QWidget::createWindowContainer(bioWindow, this, Qt::Widget);
//        bioWidget->setGeometry(m_passwordEdit->geometry());
//        bioWidget->show();
//        return;
//    }
    if(m_timer->isActive())
        stopWaiting();
    if(!text.isEmpty())
        m_passwordEdit->show();
    m_passwordEdit->setFocus();
    if(type != QLightDM::Greeter::PromptTypeSecret)
        m_passwordEdit->setType(QLineEdit::Normal);
    else
        m_passwordEdit->setType(QLineEdit::Password);

    if(text == "Password: ")
        text = tr("Password: ");
    if(text == "login:") {
        text = tr("login:");
        m_name = "*login";
        m_nameLabel->setText(tr("login"));
    }
    m_passwordEdit->setPrompt(text);
}

void LoginWindow::onShowMessage(QString text, QLightDM::Greeter::MessageType type)
{
    qDebug()<< "message: "<< text;
    int lineNum = text.count('\n') + 1;
    int height = 20 * lineNum;  //label的高度
    if(m_messageLabels.size() >= 1) {
    //调整窗口大小
        this->resize(this->width(), this->height()+height);
    //移动密码输入框的位置
        m_passwordEdit->move(m_passwordEdit->geometry().left(),
                             m_passwordEdit->geometry().top() + height);
    }
    //每条message添加一个label
    int top = m_passwordEdit->geometry().top() - height - 10;
    QLabel *msgLabel = new QLabel(this);
    msgLabel->setGeometry(QRect(220, top, 300, height));

    if(type == QLightDM::Greeter::MessageTypeError)    {
        msgLabel->setObjectName(QStringLiteral("errorMsgLabel"));
    }
    else if(type == QLightDM::Greeter::MessageTypeInfo)    {
        msgLabel->setObjectName(QStringLiteral("infoMsgLabel"));
    }
    msgLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    msgLabel->setText(text);
    msgLabel->show();
    m_messageLabels.push_back(msgLabel);
}

void LoginWindow::onAuthenticationComplete()
{
    stopWaiting();
    if(m_greeter->isAuthenticated()) {
        qDebug()<< "authentication success";
        saveLastLoginUser();
        m_greeter->startSession();
    } else {
        qDebug() << "authentication failed";
        onShowMessage(tr("Incorrect password, please input again"), QLightDM::Greeter::MessageTypeError);
        m_passwordEdit->clear();
        m_greeter->authenticate(m_name);
    }
}
