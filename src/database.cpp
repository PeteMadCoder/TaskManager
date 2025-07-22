#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QTextStream>
#include <QSqlRecord>

bool Database::initialize() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    // Get user data directory
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        qCritical() << "Could not determine data directory";
        return false;
    }
    
    // Ensure directory exists
    QDir dir(dataDir);
    if (!dir.exists() && !dir.mkpath(".")) {
        qCritical() << "Failed to create data directory:" << dataDir;
        return false;
    }
    
    // Set database path
    QString dbPath = dataDir + "/taskmanager.db";
    qDebug() << "Using database at:" << dbPath;
    db.setDatabaseName(dbPath);
    
    if (!db.open()) {
        qCritical() << "Database error: " << db.lastError();
        return false;
    }

    // Create tables if missing
    QSqlQuery query;
    return query.exec(
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "description TEXT,"
        "deadline DATETIME,"
        "priority INTEGER DEFAULT 0,"
        "is_completed BOOLEAN DEFAULT 0)"
    ) && query.exec(
        "CREATE TABLE IF NOT EXISTS todo_lists ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "date DATE NOT NULL)"
    ) && query.exec(
        "CREATE TABLE IF NOT EXISTS todo_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "list_id INTEGER NOT NULL,"
        "title TEXT NOT NULL,"
        "description TEXT," 
        "priority INTEGER DEFAULT 0,"
        "duration INTEGER DEFAULT 30,"
        "completed BOOLEAN DEFAULT 0,"
        "FOREIGN KEY(list_id) REFERENCES todo_lists(id))"
    ) && query.exec(
        "CREATE TABLE IF NOT EXISTS templates ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL)"
    ) && query.exec(
        "CREATE TABLE IF NOT EXISTS template_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "template_id INTEGER NOT NULL,"
        "title TEXT NOT NULL,"
        "description TEXT,"
        "priority INTEGER DEFAULT 0,"
        "duration INTEGER DEFAULT 30,"
        "FOREIGN KEY(template_id) REFERENCES templates(id))"
    );
}

void Database::shutdown() {
    QSqlDatabase::database().close();
}

// Task Operations
bool Database::createTask(Task& task) {
    QSqlQuery query;
    query.prepare(
        "INSERT INTO tasks ("
        "title, description, deadline, priority, is_completed"
        ") VALUES (?, ?, ?, ?, ?)"
    );
    query.addBindValue(task.title);
    query.addBindValue(task.description);
    query.addBindValue(task.deadline);
    query.addBindValue(task.priority);
    query.addBindValue(task.isCompleted);

    if (!query.exec()) {
        qWarning() << "createTask failed:" << query.lastError().text();
        return false;
    }

    task.id = query.lastInsertId().toInt();
    return true;
}

bool Database::updateTask(Task& task) {
    QSqlQuery query;
    query.prepare(
        "UPDATE tasks SET "
        "title = ?, "
        "description = ?, "
        "deadline = ?, "
        "priority = ?, "
        "is_completed = ? "
        "WHERE id = ?"
    );
    query.addBindValue(task.title);
    query.addBindValue(task.description);
    query.addBindValue(task.deadline);
    query.addBindValue(task.priority);
    query.addBindValue(task.isCompleted);
    query.addBindValue(task.id);

    if (!query.exec()) {
        qWarning() << "updateTask failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool Database::deleteTask(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM tasks WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "deleteTask failed:" << query.lastError().text();
        return false;
    }
    return true;
}

QVector<Task> Database::getAllTasks() {
    QVector<Task> tasks;
    QSqlQuery query("SELECT * FROM tasks");
    
    if (!query.exec()) {
        qWarning() << "getAllTasks failed:" << query.lastError().text();
        return tasks;
    }

    while (query.next()) {
        Task task;
        task.id = query.value("id").toInt();
        task.title = query.value("title").toString();
        task.description = query.value("description").toString();
        task.deadline = query.value("deadline").toDateTime();
        task.priority = query.value("priority").toInt();
        task.isCompleted = query.value("is_completed").toBool();
        tasks.append(task);
    }
    return tasks;
}

// TODOList Operations
bool Database::createTODOList(TODOList& list) {
    QSqlQuery query;
    query.prepare(
        "INSERT INTO todo_lists (name, date) VALUES (?, ?)"
    );
    query.addBindValue(list.name);
    query.addBindValue(list.date);

    if (!query.exec()) {
        qWarning() << "createTODOList failed:" << query.lastError().text();
        return false;
    }

    list.id = query.lastInsertId().toInt();
    return true;
}

bool Database::updateTODOList(const TODOList& list) {
    QSqlQuery query;
    query.prepare(
        "UPDATE todo_lists SET name = ?, date = ? WHERE id = ?"
    );
    query.addBindValue(list.name);
    query.addBindValue(list.date);
    query.addBindValue(list.id);

    if (!query.exec()) {
        qWarning() << "updateTODOList failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool Database::deleteTODOList(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM todo_lists WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "deleteTODOList failed:" << query.lastError().text();
        return false;
    }
    return true;
}

QVector<TODOList> Database::getAllTODOLists() {
    QVector<TODOList> lists;
    QSqlQuery query("SELECT * FROM todo_lists");
    
    if (!query.exec()) {
        qWarning() << "getAllTODOLists failed:" << query.lastError().text();
        return lists;
    }

    while (query.next()) {
        TODOList list;
        list.id = query.value("id").toInt();
        list.name = query.value("name").toString();
        list.date = query.value("date").toDate();
        lists.append(list);
    }
    return lists;
}

// TODOItem Operations
bool Database::createTODOItem(TODOItem& item) {
    QSqlQuery query;
    query.prepare(
        "INSERT INTO todo_items ("
        "list_id, title, description, priority, duration, completed"
        ") VALUES (?, ?, ?, ?, ?, ?)"
    );
    query.addBindValue(item.listId);
    query.addBindValue(item.title);       // NEW
    query.addBindValue(item.description); // NEW
    query.addBindValue(item.priority);    // NEW
    query.addBindValue(item.duration);
    query.addBindValue(item.completed);

    if (!query.exec()) {
        qWarning() << "createTODOItem failed:" << query.lastError().text();
        return false;
    }

    item.id = query.lastInsertId().toInt();
    return true;
}

bool Database::updateTODOItem(const TODOItem& item) {
    QSqlQuery query;
    query.prepare(
        "UPDATE todo_items SET "
        "list_id = ?, "
        "title = ?, "              // NEW
        "description = ?, "         // NEW
        "priority = ?, "            // NEW
        "duration = ?, "
        "completed = ? "
        "WHERE id = ?"
    );
    query.addBindValue(item.listId);
    query.addBindValue(item.title);       // NEW
    query.addBindValue(item.description); // NEW
    query.addBindValue(item.priority);    // NEW
    query.addBindValue(item.duration);
    query.addBindValue(item.completed);
    query.addBindValue(item.id);

    if (!query.exec()) {
        qWarning() << "updateTODOItem failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool Database::deleteTODOItem(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM todo_items WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "deleteTODOItem failed:" << query.lastError().text();
        return false;
    }
    return true;
}

QVector<TODOItem> Database::getItemsForList(int listId) {
    QVector<TODOItem> items;
    QSqlQuery query;
    query.prepare("SELECT * FROM todo_items WHERE list_id = ?");
    query.addBindValue(listId);
    
    if (!query.exec()) {
        qWarning() << "getItemsForList failed:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        TODOItem item;
        item.id = query.value("id").toInt();
        item.listId = query.value("list_id").toInt();
        item.title = query.value("title").toString();         // NEW
        item.description = query.value("description").toString(); // NEW
        item.priority = query.value("priority").toInt();      // NEW
        item.duration = query.value("duration").toInt();
        item.completed = query.value("completed").toBool();
        items.append(item);
    }
    return items;
}

// Template Operations
bool Database::createTemplate(Template& templ) {
    QSqlQuery query;
    query.prepare("INSERT INTO templates (name) VALUES (?)");
    query.addBindValue(templ.name);
    
    if (!query.exec()) {
        qWarning() << "createTemplate failed:" << query.lastError().text();
        return false;
    }
    
    templ.id = query.lastInsertId().toInt();
    return true;
}

bool Database::deleteTemplate(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM templates WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec()) {
        qWarning() << "deleteTemplate failed:" << query.lastError().text();
        return false;
    }
    return true;
}

QVector<Template> Database::getAllTemplates() {
    QVector<Template> templates;
    QSqlQuery query("SELECT * FROM templates");
    
    if (!query.exec()) {
        qWarning() << "getAllTemplates failed:" << query.lastError().text();
        return templates;
    }

    while (query.next()) {
        Template templ;
        templ.id = query.value("id").toInt();
        templ.name = query.value("name").toString();
        templates.append(templ);
    }
    return templates;
}

// Template Item Operations
bool Database::createTemplateItem(TemplateItem& item) {
    QSqlQuery query;
    query.prepare(
        "INSERT INTO template_items ("
        "template_id, title, description, priority, duration"
        ") VALUES (?, ?, ?, ?, ?)"
    );
    query.addBindValue(item.templateId);
    query.addBindValue(item.title);
    query.addBindValue(item.description);
    query.addBindValue(item.priority);
    query.addBindValue(item.duration);

    if (!query.exec()) {
        qWarning() << "createTemplateItem failed:" << query.lastError().text();
        return false;
    }

    item.id = query.lastInsertId().toInt();
    return true;
}

bool Database::deleteTemplateItemsForTemplate(int templateId) {
    QSqlQuery query;
    query.prepare("DELETE FROM template_items WHERE template_id = ?");
    query.addBindValue(templateId);
    
    if (!query.exec()) {
        qWarning() << "deleteTemplateItemsForTemplate failed:" << query.lastError().text();
        return false;
    }
    return true;
}

QVector<TemplateItem> Database::getItemsForTemplate(int templateId) {
    QVector<TemplateItem> items;
    QSqlQuery query;
    query.prepare("SELECT * FROM template_items WHERE template_id = ?");
    query.addBindValue(templateId);
    
    if (!query.exec()) {
        qWarning() << "getItemsForTemplate failed:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        TemplateItem item;
        item.id = query.value("id").toInt();
        item.templateId = query.value("template_id").toInt();
        item.title = query.value("title").toString();
        item.description = query.value("description").toString();
        item.priority = query.value("priority").toInt();
        item.duration = query.value("duration").toInt();
        items.append(item);
    }
    return items;
}

QString Database::getDatabasePath() {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dataDir + "/taskmanager.db";
}

bool Database::backupDatabase(const QString& backupPath) {
    QString dbPath = getDatabasePath();
    return QFile::copy(dbPath, backupPath);
}

bool Database::restoreDatabase(const QString& backupPath) {
    QString dbPath = getDatabasePath();
    QFile::remove(dbPath);
    return QFile::copy(backupPath, dbPath);
}

bool Database::exportToSQL(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    QSqlDatabase db = QSqlDatabase::database();
    
    // Export schema
    out << "PRAGMA foreign_keys=OFF;\n";
    out << "BEGIN TRANSACTION;\n";
    
    // Export tables
    QStringList tables = db.tables();
    for (const QString& table : tables) {
        QSqlQuery query(QString("SELECT sql FROM sqlite_master WHERE name='%1'").arg(table));
        if (query.next()) {
            out << query.value(0).toString() << ";\n";
        }
        
        // Export data
        query.exec(QString("SELECT * FROM %1").arg(table));
        while (query.next()) {
            QStringList values;
            for (int i = 0; i < query.record().count(); ++i) {
                QVariant value = query.value(i);
                if (value.isNull()) {
                    values << "NULL";
                } else if (value.type() == QVariant::String) {
                    values << "'" + value.toString().replace("'", "''") + "'";
                } else {
                    values << value.toString();
                }
            }
            out << QString("INSERT INTO %1 VALUES (%2);\n")
                   .arg(table)
                   .arg(values.join(","));
        }
    }
    
    out << "COMMIT;\n";
    out << "PRAGMA foreign_keys=ON;";
    return true;
}

bool Database::importFromSQL(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    
    QTextStream in(&file);
    QString sql = in.readAll();
    
    // Split into individual commands
    QStringList commands = sql.split(';', Qt::SkipEmptyParts);
    for (QString command : commands) {
        command = command.trimmed();
        if (command.isEmpty()) continue;
        
        QSqlQuery query;
        if (!query.exec(command)) {
            db.rollback();
            return false;
        }
    }
    
    return db.commit();
}