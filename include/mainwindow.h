#include <QMainWindow>
#include <QTableView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include "task.h"
#include <QCalendarWidget>
#include <QTabWidget>
#include "todo.h"
#include <QListView>
#include <QSplitter>
#include <QLabel>
#include <QCheckBox>
#include <QAction>

class QPushButton;
class QCalendarWidget;
class QComboBox;
class QLineEdit;
class QDateEdit;
class QTextEdit;
class QGroupEdit;
class QGroupBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void addTask();
    void editTask();
    void deleteTask();
    void refreshTaskList();
    void filterTasks();
    void showTaskDetails(const QModelIndex &index);
    void toggleTaskCompletion();
    void updatePriorityFilter(int index);
    void highlightTaskDates();
    void updateCalendarTasks(const QDate &date);
    void refreshAllViews();
    void loadTODOListsForDate(const QDate &date);
    void createTODOList();
    void loadTODOItemsForList(const QModelIndex &index);
    void addTODOItem();
    void deleteTODOList();
    void deleteTODOItem();
    void editTODOItem();
    void refreshTemplateCombo();
    void createFromTemplate();
    void saveAsTemplate();
    void manageTemplates();
    void updatePlanStatus();
    void onTaskDoubleClicked(const QModelIndex& index);
    void onTodoItemDoubleClicked(const QModelIndex& index);
    void onTodayTaskDoubleClicked(const QModelIndex& index);
    void onTodayTodoItemDoubleClicked(const QModelIndex& index);
    void updateCalendarTaskDetails(const QModelIndex &current);
    void backupDatabase();
    void restoreDatabase();
    void exportDatabase();
    void importDatabase();

private:
    void setupUI();
    void setupDatabase();
    void setupConnections();
    void createTaskDialog(bool isEditing = false);
    void applyTableStyling();
    QVector<Task> getTasksForDate(const QDate& date);

    // Database
    QVector<Task> getAllFilteredTasks();

    // UI Components
    QTableView* taskView;
    QStandardItemModel* model;
    QSortFilterProxyModel* proxyModel;

    // Filter controls
    QLineEdit* searchBox;
    QComboBox* priorityFilter;
    QCheckBox* showCompletedCheckbox;
    QPushButton* clearFiltersButton;

    // Task 
    QPushButton* addButton;
    QPushButton* editButton;
    QPushButton* deleteButton;
    QPushButton* completeButton;
    
    // Details panel
    QGroupBox* detailsGroup;
    QTextEdit* detailsView;

    // Current selected task
    int currentTaskId = -1;

    // Calendar components
    QTabWidget* mainTabs;
    QCalendarWidget* calendarWidget;
    QStandardItemModel* calendarModel;
    QTableView* calendarTaskView;
    QTextEdit* calendarDetailsView;
    Task getTaskByTitleAndDate(const QString& title, const QDate& date);

    // TODO List components
    QWidget* todoTab;
    QDateEdit* todoDateEdit;
    QListView* todoListView;
    QStandardItemModel* todoListModel;
    QTableView* todoItemView;
    QStandardItemModel* todoItemModel;
    QPushButton* addTodoListButton;
    QPushButton* addTodoItemButton;
    QPushButton* deleteTodoItemButton;
    QPushButton* deleteTodoListButton;
    QPushButton* editTodoItemButton;
    QLabel* planStatusLabel;

    // Today Tad components
    QWidget* todayTab;
    QTableView* todayTaskView;
    QStandardItemModel* todayTaskModel;
    QTableView* todayTodoView;
    QStandardItemModel* todayTodoModel;
    QSplitter* todaySplitter;
    
    // Today Tab methods
    void setupTodayTab();
    void refreshTodayTasks();
    void refreshTodayTodoItems();
    void markTaskComplete(int taskId);
    void toggleTodoItemCompletion(int itemId);
    void onMarkTaskComplete();
    void onMarkTodoComplete();
    void createTodoItemDialog(int listId, int itemId);

    // Template components
    QComboBox* templateCombo;
    QPushButton* createFromTemplateButton;
    QPushButton* saveAsTemplateButton;
    QPushButton* manageTemplatesButton;

    void showTaskDetailsDialog(const Task& task);
    void showTodoItemDetailsDialog(const TODOItem& item);

    // Backup/Restore
    QAction* backupAction;
    QAction* restoreAction;
    QAction* exportAction;
    QAction* importAction;

    QString formatDescription(const QString& description) {
        return description.toHtmlEscaped().replace("\n", "<br>");
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};