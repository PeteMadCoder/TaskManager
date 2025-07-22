#include <QSqlDatabase>
#include <QVector>
#include "task.h"
#include "todo.h"
#include "template.h"
#include <QString>

namespace Database {
    bool initialize();
    void shutdown();

    // Task CRUD operations (create, read, update, delete)
    bool createTask(Task& task);
    bool updateTask(Task& task);
    bool deleteTask(int id);
    QVector<Task> getAllTasks();

    // TODOList operations
    bool createTODOList(TODOList& list);
    bool updateTODOList(const TODOList& list);
    bool deleteTODOList(int id);
    QVector<TODOList> getAllTODOLists();

    // TODOItem operations
    bool createTODOItem(TODOItem& item);
    bool updateTODOItem(const TODOItem& item);
    bool deleteTODOItem(int id);
    QVector<TODOItem> getItemsForList(int listId);

    // Template operations
    bool createTemplate(Template& templ);
    bool deleteTemplate(int id);
    QVector<Template> getAllTemplates();

    // Template item operations
    bool createTemplateItem(TemplateItem& item);
    bool deleteTemplateItemsForTemplate(int templateId);
    QVector<TemplateItem> getItemsForTemplate(int templateId);

    // Restore and Backup database
    QString getDatabasePath();
    bool backupDatabase(const QString& backupPath);
    bool restoreDatabase(const QString& backupPath);
    bool exportToSQL(const QString& filePath);
    bool importFromSQL(const QString& filePath);
}