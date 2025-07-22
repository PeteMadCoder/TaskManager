#pragma once
#include <QString>
#include <QDate>

struct TODOList {
    int id = -1;
    QString name;
    QDate date;
};

struct TODOItem {
    int id = -1;
    int listId = -1;
    QString title;
    QString description;
    int priority = 0; // 0=None, 1=Urgent, 2=Important, 3=Both
    int duration = 30;  // in minutes
    bool completed = false;
};