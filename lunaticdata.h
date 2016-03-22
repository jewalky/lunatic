#ifndef LUNATICDATA_H
#define LUNATICDATA_H

#include <QString>
#include <QVector>
#include <QImage>

class LunaticData;
struct LunaticPEEntry
{
    LunaticData* Parent;

    quint32 ID;
    QString Name;
    bool Directory;

    quint32 Offset;
    quint32 Size;

    QVector<LunaticPEEntry> Children;

    QImage GetAsBMP();
    QString GetAsUnicode();
    QString GetAsASCII();
};

class LunaticData
{
public:
    LunaticData();

    bool LoadFromPE(QString filename);

    LunaticPEEntry* GetEntry(QString path);
    QString GetPEName() { return PEName; }

private:
    QString PEName;
    LunaticPEEntry PERoot;
};

#endif // LUNATICDATA_H
