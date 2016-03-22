#include "lunaticdata.h"
#include <QFile>
#include <QDataStream>
#include <QStringList>
#include <QBuffer>

/*
 *
 * This class provides reading of .msstyles files (i.e.: reading of resources inside PE executables)
 * Lots of hacks and bad code here, too
 * Note to the reader: HI
 *
 */
quint32 g_id = 0;
QImage LunaticPEEntry::GetAsBMP()
{
    QFile f(Parent->GetPEName());
    if(!f.open(QIODevice::ReadOnly))
        return QImage();

    f.seek(Offset);
    QByteArray a = f.read(Size);

    QBuffer a_b(&a);
    if(!a_b.open(QIODevice::ReadWrite))
    {
        qDebug("Couldn't create read/write buffer in order to load a resource image.");
        return QImage();
    }

    QDataStream stream(&a_b);
    stream.setByteOrder(QDataStream::LittleEndian);

    // in order to actually read this BMP with any builtin methods, we need to make it valid...
    // in Windows resources, BMP header is stripped as it is "not required after the image is loaded into memory"
    // thus we need to generate it
    // upd: generating BMPv3 header on Qt < 5.3.0, generating 56-byte transparent BMPv3 header on Qt >= 5.3.0

    quint16 bmp_sig = 0x4D42; // BM
    quint32 bmp_size = a.size() + 14;
    quint16 bmp_res1 = 0;
    quint16 bmp_res2 = 0;
    quint32 bmp_offset = 14+40;

    stream.device()->seek(0x0E);
    quint16 colorspace;
    stream >> colorspace;

    if(colorspace <= 8)
    {
        stream.device()->seek(0x20);
        quint32 colortable_size;
        stream >> colortable_size;
        if(!colortable_size)
            colortable_size = 1 << colorspace;
        bmp_offset += colortable_size*4;
    }

    for(int i = 0; i < 14; i++)
        a.insert(0, (char)0); // bad hack. reserve space for the generated header.

    stream.device()->seek(0);
    stream << bmp_sig;
    stream << bmp_size;
    stream << bmp_res1;
    stream << bmp_res2;

    // https://bugreports.qt-project.org/browse/QTBUG-22031
    //  - images in Windows XP .msstyles are using BMPv3 version with extended 56-byte header
    //  - this BMP type is not supported by Qt versions prior to 5.3.0
    //  - it's not working in Qt >5.3.0 as well (tested on 5.3.2, Windows version)

    stream << bmp_offset;

    /*
    QFile f2(QString("test_0%1.bmp").arg(QString::number(g_id)));
    g_id++;
    f2.open(QIODevice::WriteOnly);
    f2.write(a);
    f2.close();
    */

    QImage img;
    if((colorspace != 32) && !img.loadFromData(a, "BMP"))
    {
        qDebug("Couldn't load resource image.");
        return QImage();
    }
    else if(colorspace == 32)
    {
        qint32 bmp_w, bmp_h;
        stream.device()->seek(18);
        stream >> bmp_w >> bmp_h;
        img = QImage(QSize(bmp_w, bmp_h), QImage::Format_ARGB32);
        quint32* a_data = (quint32*)(a.data()+bmp_offset);
        quint32* out_data = (quint32*)(img.bits());
        for(qint32 y = bmp_h-1; y >= 0; y--)
        {
            quint32* pixels = out_data+y*bmp_w;
            for(qint32 x = 0; x < bmp_w; x++)
                *pixels++ = *a_data++;
        }
    }

    return img;
}

QString LunaticPEEntry::GetAsUnicode()
{
    QFile f(Parent->GetPEName());
    if(!f.open(QIODevice::ReadOnly))
        return QString();

    f.seek(Offset);
    QByteArray a = f.read(Size);
    if(a.size() % 2)
    {
        a.append((char)0);
        a.append((char)0);
        a.append((char)0);
    }

    return QString::fromUtf16((quint16*)a.data(), a.size()/2);
}

QString LunaticPEEntry::GetAsASCII()
{
    QFile f(Parent->GetPEName());
    if(!f.open(QIODevice::ReadOnly))
        return QString();
    f.seek(Offset);
    QByteArray a = f.read(Size);
    QString s = "";
    for(int i = 0; i < a.size(); i++)
    {
        if(i % 2)
            continue;
        s += a[i];
    }

    return s;
}

LunaticData::LunaticData()
{
    //

}

// this code is only used during loading of PE
struct tmp_Section
{
    QString name;
    quint32 rva;
    quint32 virtualsize;
    quint32 offset;
    quint32 size;
};

quint32 tmp_ConvertRVAtoPhysical(QVector<tmp_Section>& sections, quint32 rva)
{
    for(int i = 0; i < sections.size(); i++)
    {
        tmp_Section& s = sections[i];
        if(rva >= s.rva && rva < s.rva+s.size)
        {
            qint64 tmp_rva = rva;
            tmp_rva -= s.rva;
            tmp_rva += s.offset;
            if(tmp_rva < 0 || tmp_rva > 0xFFFFFFFF)
                return 0;
            return (quint32)tmp_rva;
        }
    }

    return 0;
}

QVector<LunaticPEEntry> tmp_TraverseResourceTree(LunaticData* p, QVector<tmp_Section>& sections, QDataStream& stream, quint32 pe_align, quint32 pe_resource_offset, quint32 offset)
{
    QVector<LunaticPEEntry> lst;
    stream.device()->seek(offset + 0x0C);
    quint16 count_named, count_numeric;
    stream >> count_named >> count_numeric;

    // read stuff
    for(quint16 i = 0; i < count_named+count_numeric; i++)
    {
        quint32 toffs = offset + 0x10 + i * 0x08;
        stream.device()->seek(toffs);

        quint32 rva_id;
        quint32 offset;
        stream >> rva_id;
        stream >> offset;

        LunaticPEEntry ent;
        ent.Parent = p;
        ent.Directory = (offset & 0x80000000);
        ent.Offset = (offset & 0x7FFFFFFF);

        if(!(rva_id & 0x80000000)) // numeric resource
        {
            ent.ID = rva_id & 0x7FFFFFFF;
            ent.Name = QString("[%1]").arg(ent.ID, 4);
        }
        else // named resource
        {
            quint32 offs = tmp_ConvertRVAtoPhysical(sections, pe_resource_offset + (rva_id & 0x7FFFFFFF));
            if(!offs)
            {
                qDebug("...: Invalid named resource at physical address %08X.", toffs);
                continue;
            }

            stream.device()->seek(offs);
            quint16 str_size;
            stream >> str_size;

            ent.Name = "";
            while(str_size--)
            {
                // only ASCII for now
                quint16 cc;
                stream >> cc;
                cc &= 0xFF;
                ent.Name.append((QChar)cc);
            }
        }

        ent.Offset = tmp_ConvertRVAtoPhysical(sections, pe_resource_offset + ent.Offset);

        if(ent.Directory)
        {
            if(!ent.Offset)
            {
                qDebug("...: Invalid subdirectory at physical address %08X.", toffs);
                continue;
            }

            ent.Children = tmp_TraverseResourceTree(p, sections, stream, pe_align, pe_resource_offset, ent.Offset);
        }
        else
        {
            // convert offsets
            if(!ent.Offset)
            {
                qDebug("...: Invalid resource at physical address %08X.", toffs);
                continue;
            }

            stream.device()->seek(ent.Offset);
            stream >> ent.Offset;
            ent.Offset = tmp_ConvertRVAtoPhysical(sections, ent.Offset);
            stream >> ent.Size;

            if(!ent.Offset)
            {
                qDebug("...: Invalid resource at physical address %08X.", toffs);
                continue;
            }
        }

        lst.append(ent);
    }

    return lst;
}
// this code is only used during loading of PE ^

bool LunaticData::LoadFromPE(QString filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug("\"%s\" is not readable.", filename.toUtf8().data());
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream.device()->seek(0x3C);
    quint32 pe_header_offset;
    stream >> pe_header_offset;

    stream.device()->seek(pe_header_offset);
    quint32 pe_sig;
    stream >> pe_sig;
    if(pe_sig != 0x00004550)
    {
        qDebug("\"%s\" doesn't appear to be a valid PE executable.", filename.toUtf8().data());
        return false;
    }

    stream.device()->seek(pe_header_offset+0x06);
    quint16 pe_section_count;
    stream >> pe_section_count;

    quint32 pe_section_offset = pe_header_offset + 0xF8; // 0xF8 = PE header size

    qDebug("\"%s\" has %u section(s).", filename.toUtf8().data(), pe_section_count);

    QVector<tmp_Section> sections;

    for(quint16 i = 0; i < pe_section_count; i++)
    {
        char s_name[9];
        s_name[8] = 0;

        stream.device()->seek(pe_section_offset + i * 0x2C); // 0x2C being section definition size
        tmp_Section s;
        stream.readRawData(s_name, 8);
        s.name = s_name;
        stream >> s.virtualsize;
        stream >> s.rva;
        stream >> s.size;
        stream >> s.offset;
        // we don't need the flags field here
        sections.append(s);
    }

    quint32 pe_align;
    stream.device()->seek(pe_header_offset + 0x3C);
    stream >> pe_align;

    quint32 pe_resource_offset;
    stream.device()->seek(pe_header_offset + 0x88);
    stream >> pe_resource_offset;

    quint32 pe_resource_physical = tmp_ConvertRVAtoPhysical(sections, pe_resource_offset);
    if(!pe_resource_physical)
    {
        qDebug("\"%s\" is corrupted (invalid resource RVA).", filename.toUtf8().data());
        return false;
    }

    qDebug("\"%s\": Loading resources...", filename.toUtf8().data());

    PERoot.ID = 0;
    PERoot.Name = "<ROOT>";
    PERoot.Directory = true;
    PERoot.Size = 0;
    PERoot.Offset = 0;
    PERoot.Children = tmp_TraverseResourceTree(this, sections, stream, pe_align, pe_resource_offset, pe_resource_physical);

    PEName = filename;

    return true;
}

LunaticPEEntry* LunaticData::GetEntry(QString spath)
{
    // every path entry is either a complete filename, resource_id ([id]) or wildcard *
    QStringList path = spath.split("/");
    LunaticPEEntry* e = &PERoot;

    for(int i = 0; i < path.size(); i++)
    {
        QString selector = path[i];
        if(!selector.size())
            continue; // skip this part
        LunaticPEEntry* candidate = 0;
        for(int j = 0; j < e->Children.size(); j++)
        {
            // match it
            if(selector[0] == '[' && selector[selector.size()-1] == ']')
            {
                QString sel_int = selector.mid(1, selector.size()-2);
                quint32 real_int = sel_int.toInt();
                if(e->Children[j].ID != real_int)
                    continue;
            }
            else
            {
                if(e->Children[j].Name.toLower() != selector.toLower())
                    continue;
            }

            candidate = &e->Children[j];

            if(path.size()-i == 1)
            {
                return candidate;
            }
            else
            {
                e = candidate;
            }
        }

        if(!candidate)
            return 0;
    }

    return e;
}
