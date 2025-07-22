#include "mainwindow.h"
#include "database.h"
#include "task.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QTextEdit>
#include <QGroupBox>
#include <QLabel>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QFont>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDateTimeEdit>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QMap>
#include "calendardelegate.h"
#include <QListWidget>
#include <QListWidgetItem>
#include <QApplication>
#include <QTimer>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMenu>
#include <QDir>
#include <QFileDialog>

// Priority names for display
const QStringList priorityNames = {"None", "Urgent", "Important", "Urgent & Important"};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupDatabase();
    setupUI();
    setupConnections();
    refreshAllViews();
    refreshTemplateCombo();
}

void MainWindow::setupDatabase() {
    if (!Database::initialize()) {
        QMessageBox::critical(this, "Error", "Failed to initialize database!");
        exit(EXIT_FAILURE);
    }
}

void MainWindow::setupUI() {

    // Create menu bar and File menu
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    QMenu* fileMenu = menuBar->addMenu("File");
    backupAction = new QAction("Backup Database", this);
    restoreAction = new QAction("Restore Database", this);
    exportAction = new QAction("Export to SQL", this);
    importAction = new QAction("Import from SQL", this);
    
    fileMenu->addAction(backupAction);
    fileMenu->addAction(restoreAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exportAction);
    fileMenu->addAction(importAction);

    qApp->setStyleSheet(
        "QDialog {"
        "   background-color: #f8f8f8;"
        "}"
        "QLabel {"
        "   font-size: 12px;"
        "}"
        "QTextEdit {"
        "   background-color: white;"
        "   border: 1px solid #d0d0d0;"
        "   border-radius: 4px;"
        "   padding: 5px;"
        "}"
        "QDialogButtonBox {"
        "   background-color: #f0f0f0;"
        "   border-top: 1px solid #d0d0d0;"
        "   padding: 10px;"
        "}"
    );

    // Create main tabs
    mainTabs = new QTabWidget;
    QWidget* taskTab = new QWidget;
    QWidget* calendarTab = new QWidget;
    QWidget* todoTab = new QWidget;
    todayTab = new QWidget;
    mainTabs->addTab(taskTab, "Tasks");
    mainTabs->addTab(calendarTab, "Calendar");
    mainTabs->addTab(todoTab, "Plan");
    mainTabs->addTab(todayTab, "Today");
    setupTodayTab();

    // Task Tab - Existing UI
    QWidget* taskTabContent = new QWidget(taskTab);
    QHBoxLayout* taskTabLayout = new QHBoxLayout(taskTab);
    taskTabLayout->addWidget(taskTabContent);
    
    // Left panel - Task list with filters
    QWidget* taskListPanel = new QWidget(taskTabContent);
    QVBoxLayout* taskListLayout = new QVBoxLayout(taskListPanel);
    
    // Filter controls
    QWidget* filterPanel = new QWidget(taskListPanel);
    QHBoxLayout* filterLayout = new QHBoxLayout(filterPanel);
    
    searchBox = new QLineEdit(filterPanel);
    searchBox->setPlaceholderText("Search tasks...");
    
    priorityFilter = new QComboBox(filterPanel);
    priorityFilter->addItems(QStringList() << "All Priorities" << priorityNames);
    
    showCompletedCheckbox = new QCheckBox("Show Completed", filterPanel);
    showCompletedCheckbox->setChecked(false); // Hide completed by default
      
    clearFiltersButton = new QPushButton("Clear", filterPanel);
    clearFiltersButton->setToolTip("Clear all filters");
    
    filterLayout->addWidget(searchBox);
    filterLayout->addWidget(priorityFilter);
    filterLayout->addWidget(showCompletedCheckbox);
    filterLayout->addWidget(clearFiltersButton);
    
    // Task table view
    taskView = new QTableView(taskListPanel);
    model = new QStandardItemModel(0, 5, this); // Cols: Completed, Title, Description, Deadline, Priority
    model->setHorizontalHeaderLabels({"", "Title", "Description", "Deadline", "Priority"});
    
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setFilterKeyColumn(-1); // Filter on all columns
    taskView->setModel(proxyModel);
    
    // Customize table appearance
    applyTableStyling();
    
    // Task controls
    QWidget* buttonPanel = new QWidget(taskListPanel);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonPanel);
    
    addButton = new QPushButton("Add Task", buttonPanel);
    editButton = new QPushButton("Edit Task", buttonPanel);
    deleteButton = new QPushButton("Delete Task", buttonPanel);
    completeButton = new QPushButton("Mark Complete", buttonPanel);
    
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addWidget(completeButton);
    
    // Assemble left panel
    taskListLayout->addWidget(filterPanel);
    taskListLayout->addWidget(taskView);
    taskListLayout->addWidget(buttonPanel);
    
    // Right panel - Task details
    detailsGroup = new QGroupBox("Task Details", taskTabContent);
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsGroup);
    
    detailsView = new QTextEdit(detailsGroup);
    detailsView->setReadOnly(true);
    detailsLayout->addWidget(detailsView);
    
    // Task tab layout
    QHBoxLayout* taskContentLayout = new QHBoxLayout(taskTabContent);
    taskContentLayout->addWidget(taskListPanel, 3); // 3/4 width
    taskContentLayout->addWidget(detailsGroup, 1);  // 1/4 width
    
    // Calendar Tab
    QVBoxLayout* calendarLayout = new QVBoxLayout(calendarTab);
    calendarLayout->setContentsMargins(5, 5, 5, 5);  // Add some margins
    calendarLayout->setSpacing(10);  // Add spacing between widgets
    
    // Calendar widget
    calendarWidget = new QCalendarWidget(calendarTab);
    calendarWidget->setMinimumHeight(280);  // Increased from default ~200px
    calendarWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    calendarLayout->addWidget(calendarWidget);
    
    // Create splitter for task list and details
    QSplitter* calendarSplitter = new QSplitter(Qt::Horizontal, calendarTab);

    // Left panel - Task list for selected date
    taskListPanel = new QWidget(calendarSplitter);
    taskListLayout = new QVBoxLayout(taskListPanel);
    taskListLayout->setContentsMargins(0, 0, 0, 0);

    calendarTaskView = new QTableView(taskListPanel);
    calendarModel = new QStandardItemModel(0, 4, this);
    calendarModel->setHorizontalHeaderLabels({"Title", "Priority", "Status", "Time"});
    calendarTaskView->setModel(calendarModel);

    // Configure task list view
    calendarTaskView->setSelectionBehavior(QAbstractItemView::SelectRows);
    calendarTaskView->setSelectionMode(QAbstractItemView::SingleSelection);
    calendarTaskView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    calendarTaskView->setColumnWidth(1, 80);
    calendarTaskView->setColumnWidth(2, 100);
    calendarTaskView->setColumnWidth(3, 80);

    // Add styling
    calendarTaskView->setStyleSheet(
        "QTableView { background-color: white; border: 1px solid #d0d0d0; border-radius: 4px; }"
        "QTableView::item { padding: 4px; }"
        "QTableView::item:selected { background-color: #e0e0ff; color: black; }"
    );

    taskListLayout->addWidget(calendarTaskView);

    // Right panel - Task details
    QWidget* detailsPanel = new QWidget(calendarSplitter);
    detailsLayout = new QVBoxLayout(detailsPanel);
    detailsLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* detailsGroup = new QGroupBox("Task Details", detailsPanel);
    QVBoxLayout* groupLayout = new QVBoxLayout(detailsGroup);

    calendarDetailsView = new QTextEdit(detailsGroup);
    calendarDetailsView->setReadOnly(true);
    calendarDetailsView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    groupLayout->addWidget(calendarDetailsView);

    detailsLayout->addWidget(detailsGroup);

    // Add panels to splitter
    calendarSplitter->addWidget(taskListPanel);
    calendarSplitter->addWidget(detailsPanel);
    calendarSplitter->setSizes({400, 200}); // Adjust initial split ratio

    calendarLayout->addWidget(calendarSplitter, 1); // Take most space

    // Apply styling to calendar
    calendarWidget->setStyleSheet(
        "QCalendarWidget QToolButton {"
        "   height: 20px;"
        "   width: 50px;"
        "   color: white;"
        "   background-color: #3a3a3a;"
        "}"
        "QCalendarWidget QMenu {"
        "   background-color: white;"
        "}"
    );
    
    // Daily Plans Tab (TODO Lists)
    QVBoxLayout* todoLayout = new QVBoxLayout(todoTab);
    todoLayout->setSpacing(10);

    // Template section - Grouped
    QGroupBox* templateGroup = new QGroupBox("Template Operations", todoTab);
    QHBoxLayout* templateLayout = new QHBoxLayout(templateGroup);

    templateCombo = new QComboBox(todoTab);
    templateCombo->setMinimumWidth(200);
    templateCombo->setToolTip("Select a template to create a new plan");
    
    createFromTemplateButton = new QPushButton("Create Plan from Template", todoTab);
    createFromTemplateButton->setIcon(QIcon(":/icons/template.png"));
    
    saveAsTemplateButton = new QPushButton("Save as Template", todoTab);
    saveAsTemplateButton->setIcon(QIcon(":/icons/save_template.png"));
    saveAsTemplateButton->setToolTip("Save current plan as reusable template");
    
    manageTemplatesButton = new QPushButton("Manage", todoTab);
    manageTemplatesButton->setIcon(QIcon(":/icons/manage.png"));
    manageTemplatesButton->setToolTip("View and manage templates");
    
    templateLayout->addWidget(new QLabel("Templates:"));
    templateLayout->addWidget(templateCombo);
    templateLayout->addWidget(createFromTemplateButton);
    templateLayout->addWidget(saveAsTemplateButton);
    templateLayout->addWidget(manageTemplatesButton);
    templateLayout->addStretch();
    
    todoLayout->addWidget(templateGroup);


    // Plan management section
    QGroupBox* planGroup = new QGroupBox("Plan Management", todoTab);
    QVBoxLayout* planLayout = new QVBoxLayout(planGroup);
    
    // Date selection - improved layout
    QHBoxLayout* dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel("Date:"));
    
    todoDateEdit = new QDateEdit(QDate::currentDate(), todoTab);
    QTimer::singleShot(0, this, [this]() {
        loadTODOListsForDate(QDate::currentDate());
    });
    todoDateEdit->setCalendarPopup(true);
    todoDateEdit->setMaximumWidth(150);
    
    addTodoListButton = new QPushButton("Create New Plan", todoTab);
    addTodoListButton->setIcon(QIcon(":/icons/add_plan.png"));
    addTodoListButton->setToolTip("Create an empty plan for the selected date");
    
    dateLayout->addWidget(todoDateEdit);
    dateLayout->addWidget(addTodoListButton);
    dateLayout->addStretch();
    
    planLayout->addLayout(dateLayout);
    
    // Status indicator
    QHBoxLayout* statusLayout = new QHBoxLayout();
    planStatusLabel = new QLabel("No plan selected", todoTab);
    planStatusLabel->setStyleSheet("color: gray; font-style: italic;");
    statusLayout->addWidget(planStatusLabel);
    statusLayout->addStretch();
    planLayout->addLayout(statusLayout);
    
    todoLayout->addWidget(planGroup);

    // Plan content section
    QGroupBox* contentGroup = new QGroupBox("Plan Content", todoTab);
    QHBoxLayout* contentLayout = new QHBoxLayout(contentGroup);
    
    // TODO lists for date
    QSplitter* todoSplitter = new QSplitter(Qt::Horizontal, todoTab);
    todoSplitter->setChildrenCollapsible(false);
    
    // Left panel - Plans list
    QWidget* listPanel = new QWidget(todoSplitter);
    QVBoxLayout* listLayout = new QVBoxLayout(listPanel);
    listLayout->setContentsMargins(5, 5, 5, 5);
    
    listLayout->addWidget(new QLabel("Plans for Date:"));
    
    todoListView = new QListView(listPanel);
    todoListView->setMinimumWidth(150);
    todoListModel = new QStandardItemModel(this);
    todoListView->setModel(todoListModel);
    
    // Add styling to list view
    todoListView->setStyleSheet(
        "QListView { background-color: white; border: 1px solid #d0d0d0; border-radius: 4px; }"
        "QListView::item { padding: 5px; }"
        "QListView::item:selected { background-color: #e0e0ff; color: black; }"
    );
    
    listLayout->addWidget(todoListView, 1); // Take available space

    // Plan buttons
    QHBoxLayout* listButtonLayout = new QHBoxLayout();
    deleteTodoListButton = new QPushButton("Delete", listPanel);
    deleteTodoListButton->setIcon(QIcon(":/icons/delete.png"));
    deleteTodoListButton->setToolTip("Delete selected plan");
    
    listButtonLayout->addWidget(deleteTodoListButton);
    listButtonLayout->addStretch();
    listLayout->addLayout(listButtonLayout);
    
    // Right panel - Plan items
    QWidget* itemPanel = new QWidget(todoSplitter);
    QVBoxLayout* itemLayout = new QVBoxLayout(itemPanel);
    itemLayout->setContentsMargins(5, 5, 5, 5);
    
    itemLayout->addWidget(new QLabel("Plan Items:"));
    
    todoItemView = new QTableView(itemPanel);
    todoItemModel = new QStandardItemModel(0, 5, this);
    todoItemModel->setHorizontalHeaderLabels({"Title", "Priority", "Duration", "Status", ""});
    todoItemView->setModel(todoItemModel);
    todoItemView->setSelectionBehavior(QAbstractItemView::SelectRows);
    todoItemView->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // Improve table appearance
    todoItemView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    todoItemView->setColumnWidth(1, 80);
    todoItemView->setColumnWidth(2, 80);
    todoItemView->setColumnWidth(3, 100);
    todoItemView->setColumnWidth(4, 40);
    
    todoItemView->setStyleSheet(
        "QTableView { background-color: white; border: 1px solid #d0d0d0; border-radius: 4px; }"
        "QTableView::item { padding: 4px; }"
        "QTableView::item:selected { background-color: #e0e0ff; color: black; }"
    );
    
    itemLayout->addWidget(todoItemView, 1); // Take available space

    // Item buttons
    QHBoxLayout* itemButtonLayout = new QHBoxLayout();
    addTodoItemButton = new QPushButton("Add Item", itemPanel);
    addTodoItemButton->setIcon(QIcon(":/icons/add.png"));
    
    editTodoItemButton = new QPushButton("Edit", itemPanel);
    editTodoItemButton->setIcon(QIcon(":/icons/edit.png"));
    
    deleteTodoItemButton = new QPushButton("Delete", itemPanel);
    deleteTodoItemButton->setIcon(QIcon(":/icons/delete.png"));
    
    itemButtonLayout->addWidget(addTodoItemButton);
    itemButtonLayout->addWidget(editTodoItemButton);
    itemButtonLayout->addWidget(deleteTodoItemButton);
    itemButtonLayout->addStretch();
    
    itemLayout->addLayout(itemButtonLayout);
    
    todoSplitter->addWidget(listPanel);
    todoSplitter->addWidget(itemPanel);
    todoSplitter->setSizes({200, 500});
    
    contentLayout->addWidget(todoSplitter);
    todoLayout->addWidget(contentGroup, 1); // Take most space
    
    // Main layout
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(mainTabs);
    setCentralWidget(centralWidget);
    
    // Window settings
    setWindowTitle("Task Manager");
    resize(1200, 700);
    
    // Status bar
    statusBar()->showMessage("Ready");
}

void MainWindow::setupConnections() {
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addTask);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::editTask);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::deleteTask);
    connect(completeButton, &QPushButton::clicked, this, &MainWindow::toggleTaskCompletion);
    connect(clearFiltersButton, &QPushButton::clicked, this, [this]() {
        searchBox->clear();
        priorityFilter->setCurrentIndex(0);
        showCompletedCheckbox->setChecked(false);
        filterTasks();
    });
    connect(searchBox, &QLineEdit::textChanged, this, &MainWindow::filterTasks);
    connect(priorityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::updatePriorityFilter);
    connect(showCompletedCheckbox, &QCheckBox::stateChanged, this, &MainWindow::filterTasks);
    connect(taskView->selectionModel(), &QItemSelectionModel::currentChanged, 
            this, &MainWindow::showTaskDetails);
    connect(calendarWidget, &QCalendarWidget::clicked, this, &MainWindow::updateCalendarTasks);
    connect(calendarWidget, &QCalendarWidget::currentPageChanged, this, &MainWindow::highlightTaskDates);
    connect(todoDateEdit, &QDateEdit::dateChanged,
            this, &MainWindow::loadTODOListsForDate);
    connect(addTodoListButton, &QPushButton::clicked,
            this, &MainWindow::createTODOList);
    connect(todoListView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::loadTODOItemsForList);
    connect(addTodoItemButton, &QPushButton::clicked,
            this, &MainWindow::addTODOItem);
    connect(mainTabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (mainTabs->tabText(index) == "Today") {
            refreshTodayTasks();
            refreshTodayTodoItems();
        }
    });
    connect(deleteTodoListButton, &QPushButton::clicked,
        this, &MainWindow::deleteTODOList);
    connect(deleteTodoItemButton, &QPushButton::clicked,
            this, &MainWindow::deleteTODOItem);
    connect(editTodoItemButton, &QPushButton::clicked, this, &MainWindow::editTODOItem);
    connect(createFromTemplateButton, &QPushButton::clicked,
            this, &MainWindow::createFromTemplate);
    connect(saveAsTemplateButton, &QPushButton::clicked,
            this, &MainWindow::saveAsTemplate);
    connect(manageTemplatesButton, &QPushButton::clicked,
            this, &MainWindow::manageTemplates);
    connect(todoListView->selectionModel(), &QItemSelectionModel::selectionChanged, 
            this, &MainWindow::updatePlanStatus);
    connect(todoDateEdit, &QDateEdit::dateChanged, 
            this, &MainWindow::updatePlanStatus);
    connect(taskView, &QTableView::doubleClicked, 
            this, &MainWindow::onTaskDoubleClicked);
    connect(todoItemView, &QTableView::doubleClicked, 
            this, &MainWindow::onTodoItemDoubleClicked);
    connect(todayTaskView, &QTableView::doubleClicked, 
            this, &MainWindow::onTodayTaskDoubleClicked);
    connect(todayTodoView, &QTableView::doubleClicked, 
            this, &MainWindow::onTodayTodoItemDoubleClicked);
    connect(calendarTaskView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::updateCalendarTaskDetails);
    taskView->installEventFilter(this);
    todayTaskView->installEventFilter(this);
    todayTodoView->installEventFilter(this);
    todoItemView->installEventFilter(this);
    connect(backupAction, &QAction::triggered, this, &MainWindow::backupDatabase);
    connect(restoreAction, &QAction::triggered, this, &MainWindow::restoreDatabase);
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportDatabase);
    connect(importAction, &QAction::triggered, this, &MainWindow::importDatabase);
}

void MainWindow::applyTableStyling() {
    // Column widths
    taskView->setColumnWidth(0, 30);  // Checkbox column
    taskView->setColumnWidth(1, 200); // Title
    taskView->setColumnWidth(2, 250); // Description
    taskView->setColumnWidth(3, 150); // Deadline
    taskView->setColumnWidth(4, 120); // Priority
    
    // Selection behavior
    taskView->setSelectionBehavior(QAbstractItemView::SelectRows);
    taskView->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // Header styling
    taskView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    taskView->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "background-color: #3a3a3a;"
        "color: white;"
        "padding: 4px;"
        "border: 1px solid #6c6c6c;"
        "}"
    );
    
    // Alternate row colors
    taskView->setAlternatingRowColors(true);
    taskView->setStyleSheet(
        "QTableView {"
        "alternate-background-color: #f0f0f0;"
        "background-color: white;"
        "}"
        "QTableView::item:selected {"
        "background-color: #a0a0ff;"
        "color: black;"
        "}"
    );
}

void MainWindow::refreshTaskList() {
    model->removeRows(0, model->rowCount());
    QVector<Task> tasks = Database::getAllTasks();

    QDateTime currentDateTime = QDateTime::currentDateTime();
    bool showCompleted = showCompletedCheckbox->isChecked();

    for (const Task& task : tasks) {
        // Skip completed tasks if checkbox isn't checked
        if (!showCompleted && task.isCompleted) continue;

        QList<QStandardItem*> rowItems;
        
        // Completion checkbox
        QStandardItem* completedItem = new QStandardItem;
        completedItem->setCheckable(true);
        completedItem->setCheckState(task.isCompleted ? Qt::Checked : Qt::Unchecked);
        completedItem->setTextAlignment(Qt::AlignCenter);
        rowItems << completedItem;
        
        // Title
        QStandardItem* titleItem = new QStandardItem(task.title);
        if (!task.isCompleted && task.deadline < currentDateTime) {
            titleItem->setForeground(QBrush(Qt::red));
        }
        if (task.isCompleted) {
            QFont font = titleItem->font();
            font.setStrikeOut(true);
            titleItem->setFont(font);
            titleItem->setForeground(QBrush(Qt::gray));
        }
        rowItems << titleItem;
        
        // Description (shortened)
        QString shortDesc = task.description.length() > 50 
                          ? task.description.left(47) + "..." 
                          : task.description;
        rowItems << new QStandardItem(shortDesc);
        
        // Deadline
        rowItems << new QStandardItem(task.deadline.toString("dd/MM/yyyy hh:mm"));
        
        // Priority
        int priority = task.priority;
        QStandardItem* priorityItem = new QStandardItem(priorityNames.value(priority, "None"));
        switch (priority) {
            case 1: priorityItem->setForeground(QBrush(Qt::red)); break;
            case 2: priorityItem->setForeground(QBrush(QColor(255, 165, 0))); break; // Orange
            case 3: priorityItem->setForeground(QBrush(Qt::darkRed)); break;
        }
        rowItems << priorityItem;
        
        // Store ID in first item
        rowItems.first()->setData(task.id);
        
        model->appendRow(rowItems);
    }
    
    statusBar()->showMessage(QString("Showing %1 tasks").arg(tasks.size()));
}

void MainWindow::addTask() {
    createTaskDialog(false);
}

void MainWindow::editTask() {
    QModelIndex currentIndex = taskView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, "No Selection", "Please select a task to edit.");
        return;
    }
    createTaskDialog(true);
}

void MainWindow::createTaskDialog(bool isEditing) {
    QDialog dialog(this);
    dialog.setWindowTitle(isEditing ? "Edit Task" : "Add New Task");
    
    QFormLayout form(&dialog);
    
    // Title
    QLineEdit* titleEdit = new QLineEdit;
    form.addRow("Title:", titleEdit);
    
    // Description
    QTextEdit* descriptionEdit = new QTextEdit;
    form.addRow("Description:", descriptionEdit);
    
    // Deadline
    QDateTimeEdit* deadlineEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    deadlineEdit->setDisplayFormat("dd/MM/yyyy hh:mm");
    deadlineEdit->setCalendarPopup(true);
    form.addRow("Deadline:", deadlineEdit);
    
    // Priority
    QComboBox* priorityCombo = new QComboBox;
    priorityCombo->addItems(priorityNames);
    form.addRow("Priority:", priorityCombo);

    // REMOVED RECURRENCE SECTION

    // Buttons
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                              Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    
    // If editing, populate with existing data
    if (isEditing) {
        QModelIndex proxyIndex = taskView->currentIndex();
        QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
        QStandardItem* idItem = model->item(sourceIndex.row(), 0);
        int taskId = idItem->data().toInt();
        
        Task task;
        QVector<Task> allTasks = Database::getAllTasks();
        for (const Task& t : allTasks) {
            if (t.id == taskId) {
                task = t;
                break;
            }
        }
        
        titleEdit->setText(task.title);
        descriptionEdit->setText(task.description);
        deadlineEdit->setDateTime(task.deadline);
        priorityCombo->setCurrentIndex(task.priority);
    }
    
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        Task task;
        if (isEditing) {
            QModelIndex proxyIndex = taskView->currentIndex();
            QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
            QStandardItem* idItem = model->item(sourceIndex.row(), 0);
            task.id = idItem->data().toInt();
        }
        
        task.title = titleEdit->text();
        task.description = descriptionEdit->toPlainText();
        task.deadline = deadlineEdit->dateTime();
        task.priority = priorityCombo->currentIndex();
        
        bool success = isEditing ? Database::updateTask(task) : Database::createTask(task);
        if (success) {
            refreshAllViews();
            statusBar()->showMessage(isEditing ? "Task updated" : "Task added", 3000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to save task.");
        }
    }
}

void MainWindow::deleteTask() {
    QModelIndex currentIndex = taskView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, "No Selection", "Please select a task to delete.");
        return;
    }
    
    QModelIndex sourceIndex = proxyModel->mapToSource(currentIndex);
    QStandardItem* idItem = model->item(sourceIndex.row(), 0);
    int taskId = idItem->data().toInt();
    
    int ret = QMessageBox::question(this, "Delete Task", 
                                  "Are you sure you want to delete this task?",
                                  QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        if (Database::deleteTask(taskId)) {
            refreshAllViews();
            statusBar()->showMessage("Task deleted", 3000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to delete task.");
        }
    }
}

void MainWindow::toggleTaskCompletion() {
    QModelIndex currentIndex = taskView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, "No Selection", "Please select a task to mark complete.");
        return;
    }
    
    QModelIndex sourceIndex = proxyModel->mapToSource(currentIndex);
    QStandardItem* idItem = model->item(sourceIndex.row(), 0);
    int taskId = idItem->data().toInt();
    
    Task task;
    QVector<Task> allTasks = Database::getAllTasks();
    for (const Task& t : allTasks) {
        if (t.id == taskId) {
            task = t;
            break;
        }
    }
    
    task.isCompleted = !task.isCompleted;
    
    if (Database::updateTask(task)) {
        refreshAllViews();
        statusBar()->showMessage(task.isCompleted ? "Task marked complete" : "Task marked incomplete", 3000);
    } else {
        QMessageBox::warning(this, "Error", "Failed to update task status.");
    }
}

void MainWindow::showTaskDetails(const QModelIndex &index) {
    if (!index.isValid()) {
        detailsView->clear();
        currentTaskId = -1;
        return;
    }
    
    QModelIndex sourceIndex = proxyModel->mapToSource(index);
    QStandardItem* idItem = model->item(sourceIndex.row(), 0);
    currentTaskId = idItem->data().toInt();
    
    Task task;
    QVector<Task> allTasks = Database::getAllTasks();
    for (const Task& t : allTasks) {
        if (t.id == currentTaskId) {
            task = t;
            break;
        }
    }
    
    QString details = QString("<h2>%1</h2>"
                            "<p><b>Status:</b> %2</p>"
                            "<p><b>Priority:</b> %3</p>"
                            "<p><b>Deadline:</b> %4</p>"
                            "<hr><p>%5</p>")
                    .arg(task.title)
                    .arg(task.isCompleted ? "Completed" : "Pending")
                    .arg(priorityNames.value(task.priority, "None"))
                    .arg(task.deadline.toString("dd/MM/yyyy hh:mm"))
                    .arg(formatDescription(task.description));
    
    detailsView->setHtml(details);
}

void MainWindow::filterTasks() {
    QString searchText = searchBox->text().toLower();   
    proxyModel->setFilterRegExp(QRegExp(searchText, Qt::CaseInsensitive, QRegExp::FixedString));
    refreshAllViews();
}

void MainWindow::updatePriorityFilter(int index) {
    if (index == 0) { // "All Priorities"
        proxyModel->setFilterRegExp("");
    } else {
        proxyModel->setFilterRegExp(QRegExp(priorityNames.at(index-1), 
                                          Qt::CaseInsensitive, 
                                          QRegExp::FixedString));
    }
    refreshAllViews();
}

QVector<Task> MainWindow::getAllFilteredTasks() {
    QVector<Task> allTasks = Database::getAllTasks();
    QVector<Task> filteredTasks;
    
    int priorityFilterIndex = priorityFilter->currentIndex();
    QString searchText = searchBox->text().toLower();
    
    for (const Task& task : allTasks) {
        // Priority filter
        if (priorityFilterIndex > 0 && task.priority != priorityFilterIndex - 1) {
            continue;
        }
        
        // Search text
        if (!searchText.isEmpty()) {
            bool textMatch = task.title.contains(searchText, Qt::CaseInsensitive) ||
                           task.description.contains(searchText, Qt::CaseInsensitive);
            if (!textMatch) continue;
        }
        
        filteredTasks.append(task);
    }
    
    return filteredTasks;
}

void MainWindow::highlightTaskDates() {
    // Clear previous formatting
    calendarWidget->setDateTextFormat(QDate(), QTextCharFormat());
    
    // Get all tasks
    QVector<Task> tasks = Database::getAllTasks();
    int currentYear = calendarWidget->yearShown();
    int currentMonth = calendarWidget->monthShown();   

    QTextCharFormat highlightFormat;
    highlightFormat.setBackground(QBrush(QColor(255, 220, 200)));
    highlightFormat.setFontWeight(QFont::Bold);
    
    // Prepare tasks by date
    QMap<QDate, QStringList> tasksByDate;
    
    // Highlight dates with tasks
    for (const Task& task : tasks) {
        QDate taskDate = task.deadline.date();
        if (taskDate.year() != currentYear || taskDate.month() != currentMonth) {
            continue;
        }
        
        // Don't highlight completed tasks
        if (task.isCompleted) continue;
        
        // Add task title to date
        if (!tasksByDate.contains(taskDate)) {
            tasksByDate[taskDate] = QStringList();
        }
        tasksByDate[taskDate].append(task.title);
        
        calendarWidget->setDateTextFormat(taskDate, highlightFormat);
    }
    
    // Access the calendar's internal view and set delegate
    QTableView* calendarView = calendarWidget->findChild<QTableView*>();
    if (calendarView) {
        calendarView->setItemDelegate(new CalendarDelegate(tasksByDate, this));
    }
}

void MainWindow::updateCalendarTasks(const QDate &date) {
    calendarModel->removeRows(0, calendarModel->rowCount());
    
    QVector<Task> tasks = Database::getAllTasks();
    QDateTime startOfDay(date, QTime(0, 0));
    QDateTime endOfDay(date, QTime(23, 59, 59));
    
    for (const Task& task : tasks) {
        // Skip tasks not on this date
        if (task.deadline < startOfDay || task.deadline > endOfDay) continue;
        
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(task.title);
        
        // Priority
        QStandardItem* priorityItem = new QStandardItem(priorityNames.value(task.priority, "None"));
        switch (task.priority) {
            case 1: priorityItem->setForeground(QBrush(Qt::red)); break;
            case 2: priorityItem->setForeground(QBrush(QColor(255, 165, 0))); break;
            case 3: priorityItem->setForeground(QBrush(Qt::darkRed)); break;
        }
        rowItems << priorityItem;
        
        // Status
        QString status = task.isCompleted ? "Completed" : "Pending";
        if (!task.isCompleted && task.deadline < QDateTime::currentDateTime()) {
            status = "Overdue";
        }
        QStandardItem* statusItem = new QStandardItem(status);
        if (status == "Overdue") statusItem->setForeground(QBrush(Qt::red));
        rowItems << statusItem;
        
        // Time
        rowItems << new QStandardItem(task.deadline.time().toString("hh:mm"));
        
        calendarModel->appendRow(rowItems);
    }
    
    // Clear details when date changes
    calendarDetailsView->clear();
    
    // Update status bar
    statusBar()->showMessage(QString("%1 tasks on %2")
                            .arg(calendarModel->rowCount())
                            .arg(date.toString("dd/MM/yyyy")));
}

void MainWindow::refreshAllViews() {
    refreshTaskList();
    highlightTaskDates();
    updateCalendarTasks(calendarWidget->selectedDate());
}

void MainWindow::loadTODOListsForDate(const QDate &date) {
    todoListModel->clear();
    
    QVector<TODOList> lists = Database::getAllTODOLists();
    for (const TODOList& list : lists) {
        if (list.date != date) continue;
        
        QStandardItem* item = new QStandardItem(list.name);
        item->setData(list.id);
        todoListModel->appendRow(item);
    }
    updatePlanStatus();
}

void MainWindow::createTODOList() {
    bool ok;
    QString name = QInputDialog::getText(this, "Create Plan", 
                                       "Plan name:", QLineEdit::Normal, 
                                       "Daily Plan", &ok);
    if (!ok || name.isEmpty()) return;
    
    TODOList newList;
    newList.name = name;
    newList.date = todoDateEdit->date();
    
    if (Database::createTODOList(newList)) {
        loadTODOListsForDate(newList.date);
    }
}

void MainWindow::loadTODOItemsForList(const QModelIndex &index) {
    if (!index.isValid()) return;
    
    todoItemModel->removeRows(0, todoItemModel->rowCount());
    int listId = todoListModel->itemFromIndex(index)->data().toInt();
    
    QVector<TODOItem> items = Database::getItemsForList(listId);
    for (const TODOItem& item : items) {
        QList<QStandardItem*> rowItems;
        
        // Title
        QStandardItem* titleItem = new QStandardItem(item.title);
        if (item.completed) {
            QFont font = titleItem->font();
            font.setStrikeOut(true);
            titleItem->setFont(font);
            titleItem->setForeground(QBrush(Qt::gray));
        }
        rowItems << titleItem;
        
        // Description
        QString shortDesc = item.description.length() > 50 
                          ? item.description.left(47) + "..." 
                          : item.description;
        rowItems << new QStandardItem(shortDesc);
        
        // Priority
        QStandardItem* priorityItem = new QStandardItem(priorityNames.value(item.priority, "None"));
        switch (item.priority) {
            case 1: priorityItem->setForeground(QBrush(Qt::red)); break;
            case 2: priorityItem->setForeground(QBrush(QColor(255, 165, 0))); break;
            case 3: priorityItem->setForeground(QBrush(Qt::darkRed)); break;
        }
        rowItems << priorityItem;
        
        // Duration
        rowItems << new QStandardItem(QString("%1 min").arg(item.duration));
        
        // Status
        QString status = item.completed ? "Completed" : "Pending";
        QStandardItem* statusItem = new QStandardItem(status);
        if (item.completed) {
            statusItem->setForeground(QBrush(Qt::darkGreen));
        }
        rowItems << statusItem;
        
        // Store ID in first item
        rowItems.first()->setData(item.id);
        
        todoItemModel->appendRow(rowItems);
    }
    
    //todoItemView->resizeColumnsToContents();
    updatePlanStatus();
}

void MainWindow::addTODOItem() {
    QModelIndex listIndex = todoListView->currentIndex();
    if (!listIndex.isValid()) {
        QMessageBox::warning(this, "No Plan Selected", "Please select a plan first.");
        return;
    }
    
    int listId = todoListModel->itemFromIndex(listIndex)->data().toInt();
    createTodoItemDialog(listId, -1); // -1 for new item
}

void MainWindow::editTODOItem() {
    QModelIndex itemIndex = todoItemView->currentIndex();
    if (!itemIndex.isValid()) {
        QMessageBox::warning(this, "No Item Selected", "Please select an item to edit.");
        return;
    }
    
    int row = itemIndex.row();
    QStandardItem* idItem = todoItemModel->item(row, 0);
    int itemId = idItem->data().toInt();
    
    QModelIndex listIndex = todoListView->currentIndex();
    if (!listIndex.isValid()) {
        QMessageBox::warning(this, "No Plan Selected", "Please select a plan first.");
        return;
    }
    
    int listId = todoListModel->itemFromIndex(listIndex)->data().toInt();
    createTodoItemDialog(listId, itemId);
}

void MainWindow::createTodoItemDialog(int listId, int itemId) {
    QDialog dialog(this);
    dialog.setWindowTitle(itemId >= 0 ? "Edit Plan Item" : "Add Plan Item");
    QFormLayout form(&dialog);
    
    // Title
    QLineEdit* titleEdit = new QLineEdit;
    form.addRow("Title:", titleEdit);
    
    // Description
    QTextEdit* descriptionEdit = new QTextEdit;
    form.addRow("Description:", descriptionEdit);

    titleEdit->setPlaceholderText("Enter item title");
    descriptionEdit->setPlaceholderText("Enter item description");
    
    // Priority
    QComboBox* priorityCombo = new QComboBox;
    priorityCombo->addItems(priorityNames);
    form.addRow("Priority:", priorityCombo);
    
    // Duration
    QSpinBox* durationSpin = new QSpinBox;
    durationSpin->setRange(5, 240);
    durationSpin->setValue(30);
    durationSpin->setSuffix(" minutes");
    form.addRow("Duration:", durationSpin);
    
    // Buttons
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                              Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    
    // If editing, populate with existing data
    TODOItem item;
    if (itemId >= 0) {
        QVector<TODOItem> allItems = Database::getItemsForList(listId);
        for (const TODOItem& i : allItems) {
            if (i.id == itemId) {
                item = i;
                break;
            }
        }
        
        titleEdit->setText(item.title);
        descriptionEdit->setText(item.description);
        priorityCombo->setCurrentIndex(item.priority);
        durationSpin->setValue(item.duration);
    }
    
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        TODOItem newItem;
        newItem.id = itemId;
        newItem.listId = listId;
        newItem.title = titleEdit->text();
        newItem.description = descriptionEdit->toPlainText();
        newItem.priority = priorityCombo->currentIndex();
        newItem.duration = durationSpin->value();
        
        bool success;
        if (itemId >= 0) {
            success = Database::updateTODOItem(newItem);
        } else {
            success = Database::createTODOItem(newItem);
        }
        
        if (success) {
            loadTODOItemsForList(todoListView->currentIndex());
        } else {
            QMessageBox::warning(this, "Error", "Failed to save item.");
        }
    }
}

void MainWindow::setupTodayTab() {
    QVBoxLayout* todayLayout = new QVBoxLayout(todayTab);
    todayLayout->setSpacing(10);
    
    // Header
    QLabel* todayHeader = new QLabel("<h2>Today's Tasks & Plan</h2>", todayTab);
    todayHeader->setAlignment(Qt::AlignCenter);
    todayLayout->addWidget(todayHeader);
    
    // Date display
    QLabel* dateLabel = new QLabel(QDate::currentDate().toString("dddd, MMMM d, yyyy"), todayTab);
    dateLabel->setAlignment(Qt::AlignCenter);
    dateLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50;");
    todayLayout->addWidget(dateLabel);
    
    // Create horizontal splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal, todayTab);
    splitter->setChildrenCollapsible(false);

    // Left panel: Today's Tasks
    QGroupBox* taskGroup = new QGroupBox("Today's Tasks", todayTab);
    QVBoxLayout* taskLayout = new QVBoxLayout(taskGroup);
    
    todayTaskView = new QTableView(taskGroup);
    todayTaskModel = new QStandardItemModel(0, 4, this);
    todayTaskModel->setHorizontalHeaderLabels({"", "Title", "Time", "Priority"});
    todayTaskView->setModel(todayTaskModel);
    todayTaskView->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Configure view
    todayTaskView->setColumnWidth(0, 30);
    todayTaskView->setColumnWidth(2, 80);
    todayTaskView->setColumnWidth(3, 100);
    todayTaskView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    
    // Add styling
    todayTaskView->setStyleSheet(
        "QTableView { background-color: white; border: 1px solid #d0d0d0; border-radius: 4px; }"
        "QTableView::item { padding: 4px; }"
        "QTableView::item:selected { background-color: #e0e0ff; color: black; }"
    );
    
    taskLayout->addWidget(todayTaskView);
    
    // Task controls
    QPushButton* markTaskCompleteButton = new QPushButton("Mark Complete", taskGroup);
    markTaskCompleteButton->setIcon(QIcon(":/icons/complete.png"));
    taskLayout->addWidget(markTaskCompleteButton, 0, Qt::AlignRight);
    
    // Right panel: Today's Plan Items
    QGroupBox* planGroup = new QGroupBox("Today's Plan Items", todayTab);
    QVBoxLayout* planLayout = new QVBoxLayout(planGroup);
    
    todayTodoView = new QTableView(planGroup);
    todayTodoModel = new QStandardItemModel(0, 4, this);
    todayTodoModel->setHorizontalHeaderLabels({"", "Title", "Duration", "Status"});
    todayTodoView->setModel(todayTodoModel);
    todayTodoView->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Configure view
    todayTodoView->setColumnWidth(0, 30);
    todayTodoView->setColumnWidth(2, 80);
    todayTodoView->setColumnWidth(3, 100);
    todayTodoView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    
    // Add styling
    todayTodoView->setStyleSheet(
        "QTableView { background-color: white; border: 1px solid #d0d0d0; border-radius: 4px; }"
        "QTableView::item { padding: 4px; }"
        "QTableView::item:selected { background-color: #e0e0ff; color: black; }"
    );
    
    planLayout->addWidget(todayTodoView);
    
    // Plan controls
    QPushButton* markTodoCompleteButton = new QPushButton("Toggle Completion", planGroup);
    markTodoCompleteButton->setIcon(QIcon(":/icons/toggle.png"));
    planLayout->addWidget(markTodoCompleteButton, 0, Qt::AlignRight);
    
    splitter->addWidget(taskGroup);
    splitter->addWidget(planGroup);
    splitter->setSizes({500, 500}); // Balanced split
    
    todayLayout->addWidget(splitter, 1); // Take available space
    
    // Progress summary
    QLabel* progressLabel = new QLabel("Progress will be shown here", todayTab);
    progressLabel->setAlignment(Qt::AlignCenter);
    progressLabel->setStyleSheet("font-style: italic; color: #7f8c8d;");
    todayLayout->addWidget(progressLabel);
    
    // Connect signals
    connect(markTaskCompleteButton, &QPushButton::clicked, this, &MainWindow::onMarkTaskComplete);
    connect(markTodoCompleteButton, &QPushButton::clicked, this, &MainWindow::onMarkTodoComplete);
}

void MainWindow::refreshTodayTasks() {
    todayTaskModel->removeRows(0, todayTaskModel->rowCount());
    
    QDate today = QDate::currentDate();
    QVector<Task> tasks = getTasksForDate(today);
    
    for (const Task& task : tasks) {
        QList<QStandardItem*> rowItems;
        
        // Completion checkbox
        QStandardItem* completedItem = new QStandardItem;
        completedItem->setCheckable(true);
        completedItem->setCheckState(task.isCompleted ? Qt::Checked : Qt::Unchecked);
        completedItem->setData(task.id);
        rowItems << completedItem;
        
        // Title
        QStandardItem* titleItem = new QStandardItem(task.title);
        if (task.isCompleted) {
            QFont font = titleItem->font();
            font.setStrikeOut(true);
            titleItem->setFont(font);
            titleItem->setForeground(QBrush(Qt::gray));
        }
        rowItems << titleItem;
        
        // Description
        /*
        QStandardItem* descItem = new QStandardItem(task.description);
        if (task.isCompleted) {
            QFont font = descItem->font();
            font.setStrikeOut(true);
            descItem->setFont(font);
            descItem->setForeground(QBrush(Qt::gray));
        }
        rowItems << descItem;
        //*/
        
        // Deadline time
        rowItems << new QStandardItem(task.deadline.time().toString("hh:mm"));
        
        // Priority
        QStandardItem* priorityItem = new QStandardItem(priorityNames.value(task.priority, "None"));
        switch (task.priority) {
            case 1: priorityItem->setForeground(QBrush(Qt::red)); break;
            case 2: priorityItem->setForeground(QBrush(QColor(255, 165, 0))); break;
            case 3: priorityItem->setForeground(QBrush(Qt::darkRed)); break;
        }
        rowItems << priorityItem;
        
        todayTaskModel->appendRow(rowItems);
    }
    
    //todayTaskView->resizeColumnsToContents();
}

void MainWindow::refreshTodayTodoItems() {
    todayTodoModel->removeRows(0, todayTodoModel->rowCount());
    
    QDate today = QDate::currentDate();
    QVector<TODOList> lists = Database::getAllTODOLists();
    QVector<TODOItem> todayItems;
    
    for (const TODOList& list : lists) {
        if (list.date == today) {
            QVector<TODOItem> items = Database::getItemsForList(list.id);
            for (TODOItem& item : items) {
                todayItems.append(item);
            }
        }
    }
    
    for (const TODOItem& item : todayItems) {
        QList<QStandardItem*> rowItems;
        
        // Completion checkbox
        QStandardItem* completedItem = new QStandardItem;
        completedItem->setCheckable(true);
        completedItem->setCheckState(item.completed ? Qt::Checked : Qt::Unchecked);
        completedItem->setData(item.id);
        rowItems << completedItem;
        
        // Title
        QStandardItem* titleItem = new QStandardItem(item.title);
        if (item.completed) {
            QFont font = titleItem->font();
            font.setStrikeOut(true);
            titleItem->setFont(font);
            titleItem->setForeground(QBrush(Qt::gray));
        }
        rowItems << titleItem;

        // Duration
        rowItems << new QStandardItem(QString("%1 min").arg(item.duration));
        
        // Priority
        QStandardItem* priorityItem = new QStandardItem(priorityNames.value(item.priority, "None"));
        switch (item.priority) {
            case 1: priorityItem->setForeground(QBrush(Qt::red)); break;
            case 2: priorityItem->setForeground(QBrush(QColor(255, 165, 0))); break;
            case 3: priorityItem->setForeground(QBrush(Qt::darkRed)); break;
        }
        rowItems << priorityItem;
                
        todayTodoModel->appendRow(rowItems);
    }
    
    //todayTodoView->resizeColumnsToContents();

    // Calculate progress
    int totalItems = todayItems.size();
    int completedItems = 0;
    
    for (const TODOItem& item : todayItems) {
        if (item.completed) completedItems++;
    }
    
    // Update progress label if available
    if (todayTab->findChild<QLabel*>() && !todayItems.isEmpty()) {
        QString progress = QString("Plan Progress: %1/%2 items completed (%3%)")
                          .arg(completedItems)
                          .arg(totalItems)
                          .arg(totalItems > 0 ? (100 * completedItems / totalItems) : 0);
        
        QLabel* progressLabel = todayTab->findChild<QLabel*>();
        if (progressLabel) {
            progressLabel->setText(progress);
            progressLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
        }
    }
}

void MainWindow::markTaskComplete(int taskId) {
    Task task;
    QVector<Task> allTasks = Database::getAllTasks();
    for (Task& t : allTasks) {
        if (t.id == taskId) {
            task = t;
            break;
        }
    }
    
    if (task.id == -1) return;
    
    task.isCompleted = true;
    if (Database::updateTask(task)) {
        refreshTodayTasks();
        refreshTaskList();  // Update main task list
        refreshAllViews();  // Update calendar if needed
    }
}

QVector<Task> MainWindow::getTasksForDate(const QDate& date) {
    QVector<Task> result;
    QVector<Task> allTasks = Database::getAllTasks();
    
    for (const Task& task : allTasks) {
        // Skip completed tasks
        if (task.isCompleted) continue;
        
        // Only include tasks with matching date
        if (task.deadline.date() == date) {
            result.append(task);
        }
    }
    
    return result;
}

void MainWindow::onMarkTaskComplete() {
    QModelIndex index = todayTaskView->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "No Selection", "Please select a task to mark complete.");
        return;
    }
    
    int row = index.row();
    QStandardItem* idItem = todayTaskModel->item(row, 0);
    int taskId = idItem->data().toInt();
    
    markTaskComplete(taskId);
}

void MainWindow::onMarkTodoComplete() {
    QModelIndex index = todayTodoView->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "No Selection", "Please select a plan item to toggle completion.");
        return;
    }
    
    int row = index.row();
    QStandardItem* idItem = todayTodoModel->item(row, 0);
    int itemId = idItem->data().toInt();
    
    toggleTodoItemCompletion(itemId);
}

void MainWindow::toggleTodoItemCompletion(int itemId) {
    TODOItem item;
    QVector<TODOItem> allItems;
    
    // Find the item
    QVector<TODOList> lists = Database::getAllTODOLists();
    for (const TODOList& list : lists) {
        QVector<TODOItem> items = Database::getItemsForList(list.id);
        for (const TODOItem& i : items) {
            if (i.id == itemId) {
                item = i;
                break;
            }
        }
        if (item.id != -1) break;
    }
    
    if (item.id == -1) return;
    
    // Toggle completion status
    bool newStatus = !item.completed;
    item.completed = newStatus;
    
    if (Database::updateTODOItem(item)) {
        refreshTodayTodoItems();
        
        // Show status message
        QString status = newStatus ? "marked complete" : "marked incomplete";
        QString title = item.title;
        statusBar()->showMessage(title + " " + status, 3000);
    }
}

void MainWindow::deleteTODOList() {
    QModelIndex listIndex = todoListView->currentIndex();
    if (!listIndex.isValid()) {
        QMessageBox::warning(this, "No Selection", "Please select a plan to delete.");
        return;
    }

    int listId = todoListModel->itemFromIndex(listIndex)->data().toInt();
    QString listName = todoListModel->itemFromIndex(listIndex)->text();

    int ret = QMessageBox::question(this, "Delete Plan", 
                                  QString("Are you sure you want to delete the plan '%1'?\nThis will also delete all its items.")
                                  .arg(listName),
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (Database::deleteTODOList(listId)) {
            loadTODOListsForDate(todoDateEdit->date());
            todoItemModel->removeRows(0, todoItemModel->rowCount());
            statusBar()->showMessage("Plan deleted", 3000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to delete plan.");
        }
    }
}

void MainWindow::deleteTODOItem() {
    QModelIndex itemIndex = todoItemView->currentIndex();
    if (!itemIndex.isValid()) {
        QMessageBox::warning(this, "No Selection", "Please select an item to delete.");
        return;
    }

    int row = itemIndex.row();
    QStandardItem* idItem = todoItemModel->item(row, 0);
    int itemId = idItem->data().toInt();

    QString description = todoItemModel->item(row, 1)->text();

    int ret = QMessageBox::question(this, "Delete Item", 
                                  QString("Are you sure you want to delete '%1'?").arg(description),
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (Database::deleteTODOItem(itemId)) {
            loadTODOItemsForList(todoListView->currentIndex());
            statusBar()->showMessage("Item deleted", 3000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to delete item.");
        }
    }
}

void MainWindow::refreshTemplateCombo() {
    templateCombo->clear();
    templateCombo->addItem("-- Select Template --", -1);
    
    QVector<Template> templates = Database::getAllTemplates();
    for (const Template& templ : templates) {
        templateCombo->addItem(templ.name, templ.id);
    }
    templateCombo->setToolTip(QString("%1 templates available").arg(templates.size()));
}

void MainWindow::createFromTemplate() {
    int templateId = templateCombo->currentData().toInt();
    if (templateId == -1) {
        QMessageBox::warning(this, "No Template Selected", "Please select a template.");
        return;
    }
    
    // Create the TODO list
    TODOList newList;
    newList.name = "Daily Plan";
    newList.date = todoDateEdit->date();
    
    if (!Database::createTODOList(newList)) {
        QMessageBox::warning(this, "Error", "Failed to create plan.");
        return;
    }
    
    // Get template items
    QVector<TemplateItem> templateItems = Database::getItemsForTemplate(templateId);
    
    // Create TODO items from template
    for (const TemplateItem& tplItem : templateItems) {
        TODOItem newItem;
        newItem.listId = newList.id;
        newItem.title = tplItem.title;
        newItem.description = tplItem.description;
        newItem.priority = tplItem.priority;
        newItem.duration = tplItem.duration;
        
        Database::createTODOItem(newItem);
    }
    
    // Refresh UI
    loadTODOListsForDate(todoDateEdit->date());
    statusBar()->showMessage("Plan created from template", 3000);
}

void MainWindow::saveAsTemplate() {
    QModelIndex listIndex = todoListView->currentIndex();
    if (!listIndex.isValid()) {
        QMessageBox::warning(this, "No Plan Selected", "Please select a plan to save as template.");
        return;
    }
    
    int listId = todoListModel->itemFromIndex(listIndex)->data().toInt();
    
    // Get plan name for template
    bool ok;
    QString name = QInputDialog::getText(
        this, 
        "Template Name", 
        "Enter template name:", 
        QLineEdit::Normal, 
        "Daily Template", 
        &ok
    );
    
    if (!ok || name.isEmpty()) return;
    
    // Create template
    Template newTemplate;
    newTemplate.name = name;
    if (!Database::createTemplate(newTemplate)) {
        QMessageBox::warning(this, "Error", "Failed to create template.");
        return;
    }
    
    // Get plan items
    QVector<TODOItem> planItems = Database::getItemsForList(listId);
    
    // Create template items
    for (const TODOItem& planItem : planItems) {
        TemplateItem newItem;
        newItem.templateId = newTemplate.id;
        newItem.title = planItem.title;
        newItem.description = planItem.description;
        newItem.priority = planItem.priority;
        newItem.duration = planItem.duration;
        
        Database::createTemplateItem(newItem);
    }
    
    // Refresh template list
    refreshTemplateCombo();
    statusBar()->showMessage("Template saved", 3000);
}

void MainWindow::manageTemplates() {
    QDialog dialog(this);
    dialog.setWindowTitle("Manage Templates");
    QVBoxLayout layout(&dialog);
    
    // Template list
    QListWidget* templateList = new QListWidget(&dialog);
    QVector<Template> templates = Database::getAllTemplates();
    for (const Template& templ : templates) {
        QListWidgetItem* item = new QListWidgetItem(templ.name, templateList);
        item->setData(Qt::UserRole, templ.id);
    }
    
    // Buttons
    QPushButton* deleteButton = new QPushButton("Delete Template", &dialog);
    QDialogButtonBox buttonBox(QDialogButtonBox::Close, &dialog);
    
    layout.addWidget(new QLabel("Saved Templates:"));
    layout.addWidget(templateList);
    layout.addWidget(deleteButton);
    layout.addWidget(&buttonBox);
    
    // Connect signals
    connect(deleteButton, &QPushButton::clicked, [&]() {
        QListWidgetItem* item = templateList->currentItem();
        if (!item) return;
        
        int templateId = item->data(Qt::UserRole).toInt();
        int ret = QMessageBox::question(
            &dialog, 
            "Delete Template", 
            "Delete this template? This cannot be undone.",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (ret == QMessageBox::Yes) {
            Database::deleteTemplate(templateId);
            Database::deleteTemplateItemsForTemplate(templateId);
            delete item;
        }
    });
    
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    dialog.exec();
    refreshTemplateCombo();
}

void MainWindow::updatePlanStatus() {
    QModelIndex listIndex = todoListView->currentIndex();
    if (listIndex.isValid()) {
        QString planName = todoListModel->itemFromIndex(listIndex)->text();
        QDate date = todoDateEdit->date();
        planStatusLabel->setText(QString("Viewing: %1 on %2").arg(planName).arg(date.toString("dd/MM/yyyy")));
        planStatusLabel->setStyleSheet("color: black; font-weight: bold;");
    } else {
        QDate date = todoDateEdit->date();
        planStatusLabel->setText(QString("No plan selected for %1").arg(date.toString("dd/MM/yyyy")));
        planStatusLabel->setStyleSheet("color: gray; font-style: italic;");
    }
}

void MainWindow::showTaskDetailsDialog(const Task& task) {
    QDialog dialog(this);
    dialog.setWindowTitle("Task Details");
    QVBoxLayout layout(&dialog);
    
    QFormLayout form;
    
    QLabel* titleLabel = new QLabel(task.title);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    form.addRow("Title:", titleLabel);
    
    QTextEdit* descriptionEdit = new QTextEdit(formatDescription(task.description));
    descriptionEdit->setReadOnly(true);
    form.addRow("Description:", descriptionEdit);
    
    QLabel* deadlineLabel = new QLabel(task.deadline.toString("dd/MM/yyyy hh:mm"));
    form.addRow("Deadline:", deadlineLabel);
    
    QLabel* priorityLabel = new QLabel(priorityNames.value(task.priority, "None"));
    QString priorityColor;
    switch (task.priority) {
        case 1: priorityColor = "color: red;"; break;
        case 2: priorityColor = "color: orange;"; break;
        case 3: priorityColor = "color: darkred;"; break;
        default: priorityColor = "";
    }
    priorityLabel->setStyleSheet(priorityColor);
    form.addRow("Priority:", priorityLabel);
    
    QLabel* statusLabel = new QLabel(task.isCompleted ? "Completed" : "Pending");
    statusLabel->setStyleSheet(task.isCompleted ? "color: green;" : "color: blue;");
    form.addRow("Status:", statusLabel);
    
    layout.addLayout(&form);
    
    QDialogButtonBox buttonBox(QDialogButtonBox::Close, Qt::Horizontal, &dialog);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout.addWidget(&buttonBox);
    
    dialog.exec();
}

void MainWindow::showTodoItemDetailsDialog(const TODOItem& item) {
    QDialog dialog(this);
    dialog.setWindowTitle("Plan Item Details");
    QVBoxLayout layout(&dialog);
    
    QFormLayout form;
    
    QLabel* titleLabel = new QLabel(item.title);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    form.addRow("Title:", titleLabel);
    
    QTextEdit* descriptionEdit = new QTextEdit(formatDescription(item.description));
    descriptionEdit->setReadOnly(true);
    form.addRow("Description:", descriptionEdit);
    
    QLabel* priorityLabel = new QLabel(priorityNames.value(item.priority, "None"));
    QString priorityColor;
    switch (item.priority) {
        case 1: priorityColor = "color: red;"; break;
        case 2: priorityColor = "color: orange;"; break;
        case 3: priorityColor = "color: darkred;"; break;
        default: priorityColor = "";
    }
    priorityLabel->setStyleSheet(priorityColor);
    form.addRow("Priority:", priorityLabel);
    
    QLabel* durationLabel = new QLabel(QString("%1 minutes").arg(item.duration));
    form.addRow("Duration:", durationLabel);
    
    QLabel* statusLabel = new QLabel(item.completed ? "Completed" : "Pending");
    statusLabel->setStyleSheet(item.completed ? "color: green;" : "color: blue;");
    form.addRow("Status:", statusLabel);
    
    layout.addLayout(&form);
    
    QDialogButtonBox buttonBox(QDialogButtonBox::Close, Qt::Horizontal, &dialog);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout.addWidget(&buttonBox);
    
    dialog.exec();
}

void MainWindow::onTaskDoubleClicked(const QModelIndex& index) {
    QModelIndex sourceIndex = proxyModel->mapToSource(index);
    QStandardItem* idItem = model->item(sourceIndex.row(), 0);
    int taskId = idItem->data().toInt();
    
    QVector<Task> allTasks = Database::getAllTasks();
    for (const Task& task : allTasks) {
        if (task.id == taskId) {
            showTaskDetailsDialog(task);
            break;
        }
    }
}

void MainWindow::onTodoItemDoubleClicked(const QModelIndex& index) {
    int row = index.row();
    QStandardItem* idItem = todoItemModel->item(row, 0);
    int itemId = idItem->data().toInt();
    
    QVector<TODOList> lists = Database::getAllTODOLists();
    for (const TODOList& list : lists) {
        QVector<TODOItem> items = Database::getItemsForList(list.id);
        for (const TODOItem& item : items) {
            if (item.id == itemId) {
                showTodoItemDetailsDialog(item);
                return;
            }
        }
    }
}

void MainWindow::onTodayTaskDoubleClicked(const QModelIndex& index) {
    int row = index.row();
    QStandardItem* idItem = todayTaskModel->item(row, 0);
    int taskId = idItem->data().toInt();
    
    QVector<Task> allTasks = Database::getAllTasks();
    for (const Task& task : allTasks) {
        if (task.id == taskId) {
            showTaskDetailsDialog(task);
            break;
        }
    }
}

void MainWindow::onTodayTodoItemDoubleClicked(const QModelIndex& index) {
    int row = index.row();
    QStandardItem* idItem = todayTodoModel->item(row, 0);
    int itemId = idItem->data().toInt();
    
    QVector<TODOList> lists = Database::getAllTODOLists();
    for (const TODOList& list : lists) {
        QVector<TODOItem> items = Database::getItemsForList(list.id);
        for (const TODOItem& item : items) {
            if (item.id == itemId) {
                showTodoItemDetailsDialog(item);
                return;
            }
        }
    }
}

void MainWindow::updateCalendarTaskDetails(const QModelIndex &current) {
    if (!current.isValid()) {
        calendarDetailsView->clear();
        return;
    }
    
    int row = current.row();
    QString title = calendarModel->item(row, 0)->text();
    QString priority = calendarModel->item(row, 1)->text();
    QString status = calendarModel->item(row, 2)->text();
    QString time = calendarModel->item(row, 3)->text();
    
    // Get full task details (you'll need to implement this)
    Task task = getTaskByTitleAndDate(title, calendarWidget->selectedDate());
    
    QString details = QString("<h2>%1</h2>"
                            "<p><b>Status:</b> %2</p>"
                            "<p><b>Priority:</b> %3</p>"
                            "<p><b>Time:</b> %4</p>"
                            "<hr><p>%5</p>")
                    .arg(title)
                    .arg(status)
                    .arg(priority)
                    .arg(time)
                    .arg(formatDescription(task.description));
    
    calendarDetailsView->setHtml(details);
}

Task MainWindow::getTaskByTitleAndDate(const QString& title, const QDate& date) {
    QVector<Task> tasks = Database::getAllTasks();
    for (const Task& task : tasks) {
        if (task.title == title && task.deadline.date() == date) {
            return task;
        }
    }
    return Task(); // return empty task if not found
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // Tasks tab
            if (obj == taskView) {
                toggleTaskCompletion();
                return true;
            }
            // Today tab - Tasks
            else if (obj == todayTaskView) {
                onMarkTaskComplete();
                return true;
            }
            // Today tab - TODO Items
            else if (obj == todayTodoView) {
                onMarkTodoComplete();
                return true;
            }
            // Plan tab - TODO Items
            else if (obj == todoItemView) {
                editTODOItem();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::backupDatabase() {
    QString defaultPath = QDir::homePath() + "/taskmanager_backup.db";
    QString backupPath = QFileDialog::getSaveFileName(this, "Backup Database", 
                                                   defaultPath, 
                                                   "Database Files (*.db)");
    if (backupPath.isEmpty()) return;
    
    if (Database::backupDatabase(backupPath)) {
        QMessageBox::information(this, "Success", "Database backup created successfully!");
    } else {
        QMessageBox::warning(this, "Error", "Failed to create backup");
    }
}

void MainWindow::restoreDatabase() {
    QString backupPath = QFileDialog::getOpenFileName(this, "Restore Database", 
                                                   QDir::homePath(), 
                                                   "Database Files (*.db)");
    if (backupPath.isEmpty()) return;
    
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this,
        "Confirm Restore",
        "This will replace your current database. Continue?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (confirm != QMessageBox::Yes) return;
    
    if (Database::restoreDatabase(backupPath)) {
        refreshAllViews();
        QMessageBox::information(this, "Success", "Database restored successfully!");
    } else {
        QMessageBox::warning(this, "Error", "Failed to restore database");
    }
}

void MainWindow::exportDatabase() {
    QString defaultPath = QDir::homePath() + "/taskmanager_export.sql";
    QString filePath = QFileDialog::getSaveFileName(this, "Export to SQL", 
                                                  defaultPath, 
                                                  "SQL Files (*.sql)");
    if (filePath.isEmpty()) return;
    
    if (Database::exportToSQL(filePath)) {
        QMessageBox::information(this, "Success", "Database exported to SQL successfully!");
    } else {
        QMessageBox::warning(this, "Error", "Failed to export database");
    }
}

void MainWindow::importDatabase() {
    QString filePath = QFileDialog::getOpenFileName(this, "Import from SQL", 
                                                  QDir::homePath(), 
                                                  "SQL Files (*.sql)");
    if (filePath.isEmpty()) return;
    
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this,
        "Confirm Import",
        "This will replace your current database. Continue?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (confirm != QMessageBox::Yes) return;
    
    if (Database::importFromSQL(filePath)) {
        refreshAllViews();
        QMessageBox::information(this, "Success", "Database imported successfully!");
    } else {
        QMessageBox::warning(this, "Error", "Failed to import database");
    }
}