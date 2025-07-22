#pragma once
#include <QString>
#include <QDateTime>

struct Task {
    int id = -1;
    QString title;
    QString description;
    QDateTime deadline;
    int priority = 0; // 0-3
    bool isCompleted = false;
};

/* Task layout in SQLite

CREATE TABLE IF NOT EXISTS tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    description TEXT,
    deadline DATETIME,
    priority INTEGER DEFAULT 0,  -- 0=None, 1=Urgent, 2=Important, 3=Both
    is_completed BOOLEAN DEFAULT 0
);
*/