#include "worktracker.h"
#include "ui_worktracker.h"
#include "model/worktask.h"
#include "model/task.h"
#include "controller/worktrackercontroller.h"

#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QCompleter>
#include <QPropertyAnimation>
#include <QSize>
#include <QTime>
#include <QDateTime>
#include <QLabel>
#include <QTextEdit>
#include <QDir>

WorkTracker::WorkTracker(WorkTrackerController* controller, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::WorkTracker)
    , m_controller(controller)
    , m_collapsedHeight(0)
{
    ui->setupUi(this);

    m_showAnimation.setTargetObject(this);
    m_showAnimation.setPropertyName("size");
    m_showAnimation.setDuration(200);

    m_hideAnimation.setTargetObject(this);
    m_hideAnimation.setPropertyName("size");
    m_hideAnimation.setDuration(200);

    ui->frame->setVisible(false);
    ui->textEdit->setVisible(false);

    m_statusDay       = new QLabel(tr("Waiting to start a new workday"), this);
    m_statusRecording = new QLabel(tr("Not recording anything yet"), this);
    ui->statusBar->addWidget(m_statusDay, 1);
    ui->statusBar->addWidget(m_statusRecording, 1);

    // Capture the current width which is set by the designer. Then we resize the window
    // to only take up as much space as is needed with the edit field and text edit not
    // being visible. Since this also changes the width we restore it using the value
    // saved earliert. Voila, our UI looks correct.
    int width = this->width();
    this->adjustSize();
    this->resize(width, this->height());
    m_collapsedHeight = this->height();

    QCompleter *completer = new QCompleter(this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    ui->tasksEdit->setCompleter(completer);

    connect(ui->workdayButton,    SIGNAL(clicked()),  m_controller, SLOT(toggleWorkDay()));
    connect(ui->selectTaskButton, SIGNAL(clicked()),  this,         SLOT(taskSelected()));
    connect(ui->taskButton,       SIGNAL(clicked()),  this,         SLOT(showInput()));
    connect(ui->summaryButton,    SIGNAL(clicked()),  this,         SLOT(showSummary()));
    connect(&m_showAnimation,     SIGNAL(finished()), this,         SLOT(showAnimationFinished()));

    connect(m_controller, SIGNAL(workDayStarted(QDateTime)),
            this,         SLOT(workDayStarted(QDateTime)));
    connect(m_controller, SIGNAL(workDayStopped(QDateTime)),
            this,         SLOT(workDayStopped(QDateTime)));
    connect(m_controller, SIGNAL(workTaskStarted(QDateTime, QString)),
            this,         SLOT(workTaskStarted(QDateTime, QString)));
    connect(m_controller, SIGNAL(workTaskStopped(QDateTime, QString)),
            this,         SLOT(workTaskStopped(QDateTime, QString)));
}

WorkTracker::~WorkTracker()
{
    delete ui;
}

void
WorkTracker::setTaskListModel(TaskListModel* model)
{
    ui->tasksEdit->completer()->setModel(model);
    m_taskModel = model;
}

void
WorkTracker::taskSelected()
{
    QString taskName = ui->tasksEdit->text();
    if (taskName.isEmpty()) {
        ui->statusBar->showMessage(tr("You must enter a task description"), 3000);
        return;
    }

    m_controller->toggleTask(taskName);

    ui->taskButton->setEnabled(true);
    ui->summaryButton->setEnabled(true);

    hideInput();
}

void
WorkTracker::workTaskStarted(QDateTime now, QString name)
{
    QString dateString = now.toLocalTime().toString(Qt::TextDate);

    hideSummary();
    ui->workdayButton->setEnabled(false);
    ui->taskButton->setText(tr("Stop &Task"));
    m_statusRecording->setText(tr("Task %1 started at %2")
                               .arg(name)
                               .arg(dateString));
}

void
WorkTracker::workTaskStopped(QDateTime now, QString name)
{
    QString dateString = now.toLocalTime().toString(Qt::TextDate);

    hideSummary();
    ui->workdayButton->setEnabled(true);
    ui->taskButton->setText(tr("Start &Task"));
    m_statusRecording->setText(tr("Task %1 stopped at %2")
                               .arg(name)
                               .arg(dateString));
}

void
WorkTracker::showInput()
{
    ui->textEdit->setVisible(false);
    ui->taskButton->setEnabled(false);

    QSize size = this->size();
    m_showAnimation.setStartValue(size);

    size.setHeight(m_collapsedHeight + ui->frame->height());
    m_showAnimation.setEndValue(size);
    m_showAnimation.start();
    m_animatedWidget = ui->frame;
}

void
WorkTracker::hideInput()
{
    // Hide the frame and shrink the window
    ui->frame->setVisible(false);

    QSize size = this->size();
    m_hideAnimation.setStartValue(size);

    size.setHeight(m_collapsedHeight);
    m_hideAnimation.setEndValue(size);
    m_hideAnimation.start();
}

void
WorkTracker::showAnimationFinished()
{
    // The frame can only be shown once there is enough room for it. Otherwise Qt would
    // just create space for it on its own and the animation would add more space on top
    // of that, resulting in a too large window.
    m_animatedWidget->setVisible(true);
}

void
WorkTracker::workDayStarted(QDateTime now)
{
    hideSummary();

    QString dateString = now.toLocalTime().toString(Qt::TextDate);
    m_statusDay->setText(tr("Working since %1").arg(dateString));

    ui->workdayButton->setText(tr("Stop Workday"));
    ui->taskButton->setEnabled(true);
}

void
WorkTracker::workDayStopped(QDateTime now)
{
    QString dateString = now.toLocalTime().toString(Qt::TextDate);
    m_statusDay->setText(tr("Work finished at %1").arg(dateString));

    ui->workdayButton->setText(tr("Start New Workday"));
    ui->taskButton->setEnabled(false);
    ui->summaryButton->setEnabled(!ui->textEdit->isVisible());
}

void
WorkTracker::showSummary()
{
    ui->summaryButton->setEnabled(false);

    if (!ui->textEdit->isVisible()) {
        ui->frame->setVisible(false);

        QString html = m_controller->generateSummary();
        ui->textEdit->setHtml(html);

        QSize size = this->size();
        m_showAnimation.setStartValue(size);

        size.setHeight(m_collapsedHeight + ui->textEdit->height());
        m_showAnimation.setEndValue(size);
        m_showAnimation.start();
        m_animatedWidget = ui->textEdit;
    }
}

void
WorkTracker::hideSummary()
{
    if (ui->textEdit->isVisible()) {
        ui->textEdit->setVisible(false);

        QSize size = this->size();
        m_hideAnimation.setStartValue(size);

        size.setHeight(m_collapsedHeight);
        m_hideAnimation.setEndValue(size);
        m_hideAnimation.start();
    }
}