// Programs.h - finding the .asm files in CPU/programs and assembling one without crashing.
#ifndef PROGRAMS_H
#define PROGRAMS_H

#include <QString>
#include <QVector>
#include <map>
#include <vector>

#include "Aashish/src/Bits.h"

struct AsmError
{
    int line = 0;           // 0 = no line (e.g. the file could not be opened)
    QString message;
};

struct AsmLoad
{
    bool ok = false;
    std::vector<Byte> bytes;
    QVector<AsmError> errors;
    std::map<Word, int> lineAt;   // instruction address -> source line (1-based)
};

// CPU/programs/<name> from the build-time path, next to the exe, or up to 3 folders above it; "" if none.
QString findProgram(const QString& name);

// Assembles a file. Never throws: a missing file becomes an error with line 0.
AsmLoad assembleProgram(const QString& path);

// "line 12: unknown register" -> {12, "unknown register"} (the Calculator reports errors as text)
AsmError parseErrorText(const QString& text);

#endif
