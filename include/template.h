#pragma once
#include <QString>

struct Template {
    int id = -1;
    QString name;
};

struct TemplateItem {
    int id = -1;
    int templateId = -1;
    QString title;
    QString description;
    int priority = 0;
    int duration = 30;
};