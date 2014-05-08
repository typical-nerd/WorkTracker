#include "workday.h"
#include "task.h"

#include <QDomDocument>
#include <QTextStream>
#include <QMapIterator>

WorkDay::WorkDay()
    : XmlData(nullptr)
{
}

WorkDay::WorkDay(QDomDocument* dataSource)
    : XmlData(dataSource)
{
    createNode(QDateTime(), QDateTime());
}

WorkDay::WorkDay(QDomDocument* dataSource, QDomElement node)
    : XmlData(dataSource, node)
{
}

WorkDay::WorkDay(QDomDocument* dataSource, QDateTime start)
    : XmlData(dataSource)
{
    createNode(start, QDateTime());
}

WorkDay::WorkDay(QDomDocument* dataSource, QDateTime start, QDateTime stop)
    : XmlData(dataSource)
{
    createNode(start, stop);
}

QDateTime
WorkDay::start() const
{
    return attributeValue("start").toDateTime();
}

void
WorkDay::setStart(QDateTime start)
{
    addAttribute("start", start.toString(Qt::ISODate));
}

QDateTime
WorkDay::stop() const
{
    return attributeValue("stop").toDateTime();
}

void
WorkDay::setStop(QDateTime stop)
{
    addAttribute("stop", stop.toString(Qt::ISODate));
}

void
WorkDay::addTask(WorkTask task)
{
    m_tasks.append(task);
    QDomElement workTask = findTask(task.task().id());
    if (workTask.isNull()) {
        // Create a new work task
        workTask = m_dataSource->createElement("task");
        workTask.setAttribute("id", task.task().id());

        m_node.appendChild(workTask);
    }

    // Append the new times to either the found one or the created one
    QDomElement timeElem = m_dataSource->createElement("time");
    timeElem.setAttribute("start", task.start().toString(Qt::ISODate));
    timeElem.setAttribute("stop",  task.stop().toString(Qt::ISODate));

    workTask.appendChild(timeElem);
}

void
WorkDay::clear()
{
    XmlData::clear();
    m_tasks.clear();
}

void
WorkDay::createNode(QDateTime start, QDateTime stop)
{
    m_node = m_dataSource->createElement("workday");

    addAttribute("start", start.toString(Qt::ISODate));
    addAttribute("stop",  stop.toString(Qt::ISODate));
}

QDomElement
WorkDay::findTask(int id) const
{
    QDomNodeList tasks = m_node.childNodes();
    if (tasks.isEmpty()) {
        return QDomElement();
    }

    int count = tasks.count();
    for (int c = 0; c < count; c++) {
        QDomNode node = tasks.item(c);
        if (!node.isNull() && node.isElement()) {
            QVariant idValue = attributeValueFromNode(node, "id");

            bool ok = false;
            int foundId = idValue.toInt(&ok);
            if (ok && foundId == id) {
                return node.toElement();
            }
        }
    }

    return QDomElement();
}

QString
WorkDay::generateSummary() const
{
    QString value;
    QTextStream html(&value);
    html << "<html>"
         << "<head><title>WorkTracker Summary</title></head>"
         << "<body><ul>";

    QMap<QString, int> durations;
    for (WorkTask task : m_tasks) {
        QMap<QString, int>::iterator found = durations.find(task.task().name());
        if (found == durations.end()) {
            durations.insert(task.task().name(), task.totalTime());
        }
        else {
            int& totalTime = found.value();
            totalTime += task.totalTime();
        }
    }

    QMapIterator<QString, int> iter(durations);
    while (iter.hasNext()) {
        auto item = iter.next();

        float hours = item.value() / 60.0f;

        html << "<li><i>" << item.key() << "</i>: ";
        html << hours << " h (" << item.value() << " min)";
        html << "</li>";
    }

    html << "</ul></body></html>";

    return value;
}

WorkDay
WorkDay::findLastOpen(QDomDocument* dataSource)
{
    QDomElement root = dataSource->documentElement();
    QDomElement days = root.firstChildElement("workdays");

    QDomNodeList children = days.childNodes();

    // Fetch the last item of the list which is the only one that can still be unfinished
    int count = children.count();
    if (count == 0) {
        return WorkDay();
    }

    QDomNode day = children.item(count - 1);
    if (day.isNull() || !day.isElement()) {
        return WorkDay();
    }

    QDomNamedNodeMap attributes = day.attributes();
    QDomAttr stopAttr = attributes.namedItem("stop").toAttr();
    if (stopAttr.isNull()) {
        // No "stop" attribute means that this day is still ongoing
        return fromDomNode(day.toElement(), dataSource);
    }

    return WorkDay();
}

WorkDay
WorkDay::fromDomNode(QDomElement node, QDomDocument* dataSource)
{
    QDomNamedNodeMap attributes = node.attributes();
    QDomAttr dateAttr = attributes.namedItem("start").toAttr();
    if (dateAttr.isNull()) {
        return WorkDay();
    }

    // There's at least a start data so let's treat it as a valid (although maybe
    // incomplete) day object
    QDateTime start = QDateTime::fromString(dateAttr.value(), Qt::ISODate);
    QDateTime stop;

    // If stop is also present then we'll take that as well. If it's missing this
    // probably means the application was shut down before finishing a day.
    dateAttr = attributes.namedItem("stop").toAttr();
    if (!dateAttr.isNull()) {
        stop = QDateTime::fromString(dateAttr.value(), Qt::ISODate);
    }

    WorkDay day(dataSource, node);

    QDomNodeList children = node.childNodes();
    int count = children.count();

    for (int c = 0; c < count; c++) {
        QDomNode taskNode = children.item(c);
        if (!taskNode.isNull()) {
            QList<WorkTask> tasks = WorkTask::fromDomNode(&taskNode, dataSource);
            day.m_tasks.append(tasks);
        }
    }

    return day;
}