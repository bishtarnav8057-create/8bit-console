#include "Programs.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QRegularExpression>
#include <exception>

#include "src/Assembler.h"

QString findProgram(const QString& name)
{
    QStringList places;
#ifdef PIXEL8_CPU_DIR
    places << QString(PIXEL8_CPU_DIR) + "/programs/" + name;    // where it was at build time
#endif
    const QString app = QCoreApplication::applicationDirPath();
    places << app + "/CPU/programs/" + name << app + "/../CPU/programs/" + name
           << app + "/../../CPU/programs/" + name << app + "/../../../CPU/programs/" + name
           << "CPU/programs/" + name << "../CPU/programs/" + name;

    for (const QString& place : places)
        if (QFileInfo::exists(place)) return QFileInfo(place).absoluteFilePath();
    return QString();
}

AsmLoad assembleProgram(const QString& path)
{
    AsmLoad out;
    if (!QFileInfo::exists(path))
    {
        out.errors.append({0, "File not found: " + path});
        return out;
    }
    try
    {
        AssembleResult r = Assembler().assembleFile(path.toLocal8Bit().constData());   // throws if unreadable
        for (const OperandCheckResult& e : r.errors) out.errors.append({e.lineNumber, QString::fromStdString(e.message)});
        out.ok = r.ok;
        out.bytes = r.bytes;
        if (!r.ok && out.errors.isEmpty()) out.errors.append({0, "The program could not be assembled."});
    }
    catch (const std::exception& e)
    {
        out.errors.append({0, QString::fromLocal8Bit(e.what())});
    }
    return out;
}

AsmError parseErrorText(const QString& text)
{
    static const QRegularExpression re("^line (\\d+): (.*)$");
    const QRegularExpressionMatch m = re.match(text);
    if (m.hasMatch()) return {m.captured(1).toInt(), m.captured(2)};
    return {0, text};
}
