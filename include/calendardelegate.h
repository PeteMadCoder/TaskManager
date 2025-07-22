#include <QPainter>
#include <QStyledItemDelegate>
#include <QDate>
#include <QMap>

class CalendarDelegate : public QStyledItemDelegate {
public:
    CalendarDelegate(QMap<QDate, QStringList> tasks, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), m_tasks(tasks) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyledItemDelegate::paint(painter, option, index);

        QDate date = index.data(Qt::EditRole).toDate();
        if (!date.isValid()) return;

        // Draw task indicators
        if (m_tasks.contains(date)) {
            const QStringList& tasks = m_tasks[date];
            
            // Draw background for days with tasks
            painter->save();
            painter->setBrush(QColor(255, 240, 200)); // Light orange background
            painter->setPen(Qt::NoPen);
            painter->drawRect(option.rect);
            painter->restore();
            
            // Draw task titles
            painter->save();
            painter->setFont(QFont("Arial", 8));
            painter->setPen(Qt::black);
            
            QRect contentRect = option.rect.adjusted(2, 2, -2, -2);
            int y = contentRect.top() + 15; // Start below the date number
            
            // Draw up to 3 task titles
            for (int i = 0; i < qMin(3, tasks.size()); i++) {
                QString task = tasks[i];
                if (i == 2 && tasks.size() > 3) {
                    task = QString("+%1 more").arg(tasks.size() - 2);
                }
                
                QTextOption textOption;
                textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
                painter->drawText(contentRect.adjusted(0, y - contentRect.top(), 0, 0), 
                                 task, textOption);
                
                QFontMetrics fm(painter->font());
                y += fm.height();
            }
            painter->restore();
        }
    }

private:
    QMap<QDate, QStringList> m_tasks;
};