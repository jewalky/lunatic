#ifndef LUNATICSTYLE_H
#define LUNATICSTYLE_H

#include <QProxyStyle>
#include "lunaticdata.h"
#include <QMap>
#include <QVariant>
#include <QMargins>
#include <QPainter>

enum LunaticDefinitionBGType
{
    BG_ImageFile, BG_BorderFill
};

enum LunaticDefinitionSizingType
{
    Sizing_Stretch, Sizing_Tile
};

enum LunaticDefinitionImageLayout
{
    Layout_Horizontal, Layout_Vertical
};

template<class T>
class QCaseInsensitiveMap : public QMap<QString, T>
{
public:
    using QMap<QString, T>::iterator;
    using QMap<QString, T>::begin;
    using QMap<QString, T>::end;
    using QMap<QString, T>::cbegin;
    using QMap<QString, T>::cend;
    using QMap<QString, T>::insert;

    T& operator[](const QString& key)
    {
        for(typename QMap<QString, T>::iterator it = begin(); it != end(); it++)
        {
            if(it.key().toLower() == key.toLower())
                return it.value();
        }

        return insert(cend(), key, undefined_value).value();
    }

    const T& operator[](const QString& key) const
    {
        for(typename QMap<QString, T>::iterator it = begin(); it != end(); it++)
        {
            if(it.key().toLower() == key.toLower())
                return it.value();
        }

        return const_value;
    }

    bool contains(const QString& key) const
    {
        for(typename QMap<QString, T>::const_iterator it = cbegin(); it != cend(); it++)
        {
            if(it.key().toLower() == key.toLower())
                return true;
        }

        return false;
    }

private:
    T undefined_value;
    T const_value;
};

struct LunaticDefinition
{
    LunaticDefinitionBGType BGType;
    QMargins SizingMargins;
    LunaticDefinitionSizingType SizingType;
    QMargins ContentMargins;
    QPixmap ImageFile0;
    QPixmap ImageFile1;
    QPixmap ImageFile2;
    QPixmap ImageFile3;
    QPixmap GlyphImageFile;
    quint32 ImageCount;
    LunaticDefinitionImageLayout ImageLayout;
    QColor TextColor;
    bool MirrorImage;
    bool UniformSizing;
    bool BorderOnly;
    QColor TransparentColor;
    bool Transparent;
    quint32 BorderSize;
    QColor FillColor;
    QColor BorderColor;
    bool ImageGlyph;
    bool TrueSize;
    bool AlignBottom;

    LunaticDefinition()
    {
        MirrorImage = false;
        UniformSizing = false;
        ImageCount = 1;
        BorderOnly = false;
        TransparentColor = QColor(255, 0, 255);
        Transparent = false;
        BorderSize = 1;
        ImageGlyph = false;
        TrueSize = false;
        ImageLayout = Layout_Horizontal;
        AlignBottom = false;
    }

    void MergeImage(QPixmap& pixmap, QString qpath, LunaticData& data)
    {
        QImage ImageFileTmp;

        QString oPath = qpath;
        QString path = "[2]/";
        for(int i = 0; i < oPath.size(); i++)
        {
            char c = oPath.at(i).toLatin1();
            if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                path.append(c);
            else path.append('_');
        }

        LunaticPEEntry* ent = data.GetEntry(path);
        if(ent && ent->Directory && ent->Children.size())
            ImageFileTmp = ent->Children[0].GetAsBMP().convertToFormat(QImage::Format_ARGB32);

        for(int y = 0; y < ImageFileTmp.height(); y++)
        {
            for(int x = 0; x < ImageFileTmp.width(); x++)
            {
                QColor c = ImageFileTmp.pixel(x, y);
                if(c.red() == TransparentColor.red() &&
                        c.green() == TransparentColor.green() &&
                        c.blue() == TransparentColor.blue())
                {
                    c.setRed(0);
                    c.setBlue(0);
                    c.setGreen(0);
                    c.setAlpha(0);
                    ImageFileTmp.setPixel(x, y, c.rgba());
                }
            }
        }

        pixmap = QPixmap::fromImage(ImageFileTmp);
    }

    void MergeMap(QCaseInsensitiveMap<QString> map, LunaticData& data)
    {
        if(map.contains("BGType"))
            BGType = (map["BGType"].toLower() == "borderfill") ? BG_BorderFill : BG_ImageFile;
        if(map.contains("SizingMargins"))
            SizingMargins = ReadMarginsFromString(map["SizingMargins"]);
        if(map.contains("SizingType"))
        {
            SizingType = (map["SizingType"].toLower()=="tile") ? Sizing_Tile : Sizing_Stretch;
            TrueSize = (map["SizingType"].toLower()=="truesize");
        }
        if(map.contains("ContentMargins"))
            ContentMargins = ReadMarginsFromString(map["ContentMargins"]);
        if(map.contains("TransparentColor"))
            TransparentColor = ReadColorFromString(map["TransparentColor"]);
        if(map.contains("Transparent"))
            Transparent = (map["Transparent"].toLower()=="true");
        if(map.contains("ImageFile"))
            MergeImage(ImageFile0, map["ImageFile"], data);
        if(map.contains("ImageFile1"))
            MergeImage(ImageFile1, map["ImageFile1"], data);
        if(map.contains("ImageFile2"))
            MergeImage(ImageFile2, map["ImageFile2"], data);
        if(map.contains("ImageFile3"))
            MergeImage(ImageFile3, map["ImageFile3"], data);
        if(map.contains("ImageCount"))
            ImageCount = map["ImageCount"].toInt();
        if(map.contains("ImageLayout"))
            ImageLayout = (map["ImageLayout"].toLower() == "horizontal") ? Layout_Horizontal : Layout_Vertical;
        if(map.contains("TextColor"))
            TextColor = ReadColorFromString(map["TextColor"]);
        if(map.contains("MirrorImage"))
            MirrorImage = (map["MirrorImage"].toLower()=="true");
        if(map.contains("UniformSizing"))
            UniformSizing = (map["UniformSizing"].toLower()=="true");
        if(map.contains("BorderOnly"))
            BorderOnly = (map["BorderOnly"].toLower()=="true");
        if(map.contains("BorderSize"))
            BorderSize = map["BorderSize"].toInt();
        if(map.contains("FillColor"))
            FillColor = ReadColorFromString(map["FillColor"]);
        if(map.contains("BorderColor"))
            BorderColor = ReadColorFromString(map["BorderColor"]);
        if(map.contains("GlyphType"))
            ImageGlyph = (map["GlyphType"].toLower() == "imageglyph");
        if(map.contains("GlyphImageFile"))
            MergeImage(GlyphImageFile, map["GlyphImageFile"], data);
        if(map.contains("VAlign"))
            AlignBottom = (map["VAlign"].toLower() == "bottom");
    }

    QMargins ReadMarginsFromString(QString string)
    {
        QStringList s = string.split(",");
        if(s.length() != 4)
            return QMargins(0, 0, 0, 0);
        QMargins m;
        m.setLeft(s.at(0).trimmed().toInt());
        m.setRight(s.at(1).trimmed().toInt());
        m.setTop(s.at(2).trimmed().toInt());
        m.setBottom(s.at(3).trimmed().toInt());
        return m;
    }

    QColor ReadColorFromString(QString string)
    {
        QStringList s = string.split(" ", QString::SkipEmptyParts);
        if(s.length() != 3)
            return QColor(0, 0, 0, 0);
        QColor c;
        c.setRed(s.at(0).trimmed().toInt());
        c.setGreen(s.at(1).trimmed().toInt());
        c.setBlue(s.at(2).trimmed().toInt());
        return c;
    }

    // this differs from QPainter::drawTiledPixmap in that this function only tiles PART of the pixmap (this is used, for instance, in XP progress bars)
    void MyDrawTiledPixmap(QPainter* painter, qint32 x, qint32 y, qint32 w, qint32 h, const QPixmap& pixmap, qint32 s_x, qint32 s_y, qint32 s_w, qint32 s_h) const
    {
        //qDebug("MyDrawTiledPixmap: %6d, %6d, %6d, %6d --- %6d, %6d, %6d, %6d", x, y, w, h, s_x, s_y, s_w, s_h);
        if(s_h <= 0 || s_w <= 0)
            return;
        if(w <= 0 || h <= 0)
            return;
        quint32 h_count = w / s_w;
        if(w % s_w != 0)
            h_count++;
        quint32 v_count = h / s_h;
        if(h % s_h != 0)
            v_count++;
        for(qint32 X = x; X < x+w; X += s_w)
        {
            for(qint32 Y = y; Y < y+h; Y += s_h)
            {
                qint32 W = s_w;
                qint32 H = s_h;
                qint32 sH = s_h;
                qint32 sW = s_w;
                if(X+W > x+w)
                {
                    W = (x+w)-X;
                    sW = W;
                }
                if(Y+H > y+h)
                {
                    H = (y+h)-Y;
                    sH = H;
                }
                if(W+H < 0 || Y+H < 0)
                    continue;
                painter->drawPixmap(X, Y, W, H, pixmap, s_x, s_y, sW, sH);
            }
        }
    }

    void DrawAt(QPainter* painter, QRect rec, quint32 image_major, quint32 image, quint32 border) const
    {
        if(BGType == BG_BorderFill)
        {
            // just draw a rectangle at the specified coordinates
            // save old pen and brush
            QPen old_pen = painter->pen();
            QBrush old_brush = painter->brush();
            QPen pen(BorderColor);
            pen.setWidth(BorderSize);
            painter->setPen(pen);
            painter->setBrush(FillColor);
            painter->drawRect(rec.left(), rec.top(), rec.right(), rec.bottom());
            painter->setBrush(old_brush);
            painter->setPen(old_pen);
            return;
        }

        if(image > ImageCount)
            return;

        const QPixmap* pix = 0;
        switch(image_major)
        {
        case 1:
            pix = &ImageFile1;
            break;
        case 2:
            pix = &ImageFile2;
            break;
        case 3:
            pix = &ImageFile3;
            break;
        default:
            pix = &ImageFile0;
            break;
        }

        const QPixmap& ImageFile = *pix;

        qint32 img_x;
        qint32 img_y;
        quint32 img_w;
        quint32 img_h;

        QMargins SizingMargins = this->SizingMargins; // overwrite and modify
        SizingMargins.setLeft(SizingMargins.left()-border);
        SizingMargins.setTop(SizingMargins.top()-border);
        SizingMargins.setRight(SizingMargins.right()-border);
        SizingMargins.setBottom(SizingMargins.bottom()-border);

        // do something with the margins if we are resizing the control to very small sizes
        // и это я ещё тут про DPI ничего не написал, а уже пиздец
        bool needResizeH = (SizingMargins.left()+SizingMargins.right() > rec.width());
        bool needResizeV = (SizingMargins.top()+SizingMargins.bottom() > rec.height());

        if(needResizeH)
        {
            qint32 m_hor_left = (float)SizingMargins.left() / (SizingMargins.left()+SizingMargins.right()) * rec.width();
            qint32 m_hor_right = (float)SizingMargins.right() / (SizingMargins.left()+SizingMargins.right()) * rec.width();
            if(SizingMargins.left() > m_hor_left)
                SizingMargins.setLeft(m_hor_left);
            if(SizingMargins.right() > m_hor_right)
                SizingMargins.setRight(m_hor_right);
        }

        if(needResizeV)
        {
            qint32 m_hor_top = (float)SizingMargins.top() / (SizingMargins.top()+SizingMargins.bottom()) * rec.height();
            qint32 m_hor_bottom = (float)SizingMargins.top() / (SizingMargins.top()+SizingMargins.bottom()) * rec.height();
            if(SizingMargins.top() > m_hor_top)
                SizingMargins.setTop(m_hor_top);
            if(SizingMargins.bottom() > m_hor_bottom)
                SizingMargins.setBottom(m_hor_bottom);
        }

        if(ImageLayout == Layout_Horizontal)
        {
            img_w = (ImageFile.width()/ImageCount);
            img_h = ImageFile.height() - border*2;
            img_x = border + img_w*image;
            img_y = border;
            img_w -= border*2;
        }
        else
        {
            img_h = (ImageFile.height()/ImageCount);
            img_w = ImageFile.width() - border*2;
            img_x = border;
            img_y = border + img_h*image;
            img_h -= border*2;
        }

        if(TrueSize)
        {
            if(AlignBottom)
                rec.setY(rec.bottom()-img_h);
            rec.setWidth(img_w);
            rec.setHeight(img_h);
        } // never resize controls with TrueSize set

        if(SizingType == Sizing_Tile)
        {
            if(!BorderOnly)
            {
                // middle
                MyDrawTiledPixmap(painter, rec.left()+SizingMargins.left(), rec.top()+SizingMargins.top(),
                                    rec.width()-(SizingMargins.left()+SizingMargins.right()+1), rec.height()-(SizingMargins.top()+SizingMargins.bottom()+1),
                                    ImageFile,
                                    img_x+SizingMargins.left(), img_y+SizingMargins.top(),
                                    img_w-(SizingMargins.left()+SizingMargins.right()), img_h-(SizingMargins.top()+SizingMargins.bottom()));
            }

            // top border
            MyDrawTiledPixmap(painter, rec.left()+SizingMargins.left(), rec.top(),
                                rec.width()-(SizingMargins.left()+SizingMargins.right()+1), SizingMargins.top(),
                                ImageFile,
                                img_x+SizingMargins.left(), img_y,
                                img_w-(SizingMargins.left()+SizingMargins.right()), SizingMargins.top());
            // bottom border
            MyDrawTiledPixmap(painter, rec.left()+SizingMargins.left(), rec.bottom()-SizingMargins.bottom(),
                                rec.width()-(SizingMargins.left()+SizingMargins.right()+1), SizingMargins.bottom(),
                                ImageFile,
                                img_x+SizingMargins.left(), img_y+img_h-SizingMargins.bottom(),
                                img_w-(SizingMargins.left()+SizingMargins.right()), SizingMargins.bottom());
            // left border
            MyDrawTiledPixmap(painter, rec.left(), rec.top()+SizingMargins.top(),
                                SizingMargins.left(), rec.height()-(SizingMargins.top()+SizingMargins.bottom()+1),
                                ImageFile,
                                img_x, img_y+SizingMargins.top(),
                                SizingMargins.left(), img_h-(SizingMargins.top()+SizingMargins.bottom()));
            // right border
            MyDrawTiledPixmap(painter, rec.right()-SizingMargins.right(), rec.top()+SizingMargins.top(),
                                SizingMargins.right(), rec.height()-(SizingMargins.top()+SizingMargins.bottom()+1),
                                ImageFile,
                                img_x+img_w-SizingMargins.right(), img_y+SizingMargins.top(),
                                SizingMargins.right(), img_h-(SizingMargins.top()+SizingMargins.bottom()));
        }
        else
        {
            if(!BorderOnly)
            {
                // middle
                painter->drawPixmap(rec.left()+SizingMargins.left(), rec.top()+SizingMargins.top(),
                                    rec.width()-(SizingMargins.left()+SizingMargins.right()), rec.height()-(SizingMargins.top()+SizingMargins.bottom()),
                                    ImageFile,
                                    img_x+SizingMargins.left(), img_y+SizingMargins.top(),
                                    img_w-(SizingMargins.left()+SizingMargins.right()), img_h-(SizingMargins.top()+SizingMargins.bottom()));
            }

            // top border
            painter->drawPixmap(rec.left()+SizingMargins.left(), rec.top(),
                                rec.width()-(SizingMargins.left()+SizingMargins.right()), SizingMargins.top(),
                                ImageFile,
                                img_x+SizingMargins.left(), img_y,
                                img_w-(SizingMargins.left()+SizingMargins.right()), SizingMargins.top());
            // bottom border
            painter->drawPixmap(rec.left()+SizingMargins.left(), rec.bottom()-SizingMargins.bottom(),
                                rec.width()-(SizingMargins.left()+SizingMargins.right()), SizingMargins.bottom(),
                                ImageFile,
                                img_x+SizingMargins.left(), img_y+img_h-SizingMargins.bottom(),
                                img_w-(SizingMargins.left()+SizingMargins.right()), SizingMargins.bottom());
            // left border
            painter->drawPixmap(rec.left(), rec.top()+SizingMargins.top(),
                                SizingMargins.left(), rec.height()-(SizingMargins.top()+SizingMargins.bottom()),
                                ImageFile,
                                img_x, img_y+SizingMargins.top(),
                                SizingMargins.left(), img_h-(SizingMargins.top()+SizingMargins.bottom()));
            // right border
            painter->drawPixmap(rec.right()-SizingMargins.right(), rec.top()+SizingMargins.top(),
                                SizingMargins.right(), rec.height()-(SizingMargins.top()+SizingMargins.bottom()),
                                ImageFile,
                                img_x+img_w-SizingMargins.right(), img_y+SizingMargins.top(),
                                SizingMargins.right(), img_h-(SizingMargins.top()+SizingMargins.bottom()));
        }

        // paint the corners. this is always the same.
        painter->drawPixmap(rec.left(), rec.top(),
                            SizingMargins.left(), SizingMargins.top(),
                            ImageFile,
                            img_x, img_y,
                            SizingMargins.left(), SizingMargins.top());
        painter->drawPixmap(rec.right()-SizingMargins.right(), rec.top(),
                            SizingMargins.right(), SizingMargins.top(),
                            ImageFile,
                            img_x+img_w-SizingMargins.right(), img_y,
                            SizingMargins.right(), SizingMargins.top());
        painter->drawPixmap(rec.left(), rec.bottom()-SizingMargins.bottom(),
                            SizingMargins.left(), SizingMargins.bottom(),
                            ImageFile,
                            img_x, img_y+img_h-SizingMargins.bottom(),
                            SizingMargins.left(), SizingMargins.bottom());
        painter->drawPixmap(rec.right()-SizingMargins.right(), rec.bottom()-SizingMargins.bottom(),
                            SizingMargins.right(), SizingMargins.bottom(),
                            ImageFile,
                            img_x+img_w-SizingMargins.right(), img_y+img_h-SizingMargins.bottom(),
                            SizingMargins.right(), SizingMargins.bottom());
        // now either don't draw the remaining part, stretch the remaining part or tile the remaining part

        // largest glyph
        const QPixmap* ImageFileL = 0;
        if(!GlyphImageFile.isNull())
            ImageFileL = &GlyphImageFile;
        else
        {
            if(!ImageFile1.isNull())
                ImageFileL = &ImageFile1;
            if(!ImageFile2.isNull())
                ImageFileL = &ImageFile2;
            if(!ImageFile3.isNull())
                ImageFileL = &ImageFile3;
        }

        //qDebug("ImageGlyph = %s; ImageFileL = %p", ImageGlyph ? "true" : "false", ImageFileL);

        if(ImageGlyph && ImageFileL)
        {
            //painter->drawPixmap(0, 0, ImageFile1);
            qint32 simg_x;
            qint32 simg_y;
            quint32 simg_w;
            quint32 simg_h;

            if(ImageLayout == Layout_Horizontal)
            {
                simg_w = (ImageFileL->width()/ImageCount);
                simg_h = ImageFileL->height();
                simg_x = simg_w*image;
                simg_y = 0;
            }
            else
            {
                simg_h = (ImageFileL->height()/ImageCount);
                simg_w = ImageFileL->width();
                simg_x = 0;
                simg_y = simg_h*image;
            }

            qint32 img_x = rec.x()+rec.width()/2-simg_w/2-1;
            qint32 img_y = rec.y()+rec.height()/2-simg_h/2-1;

            painter->drawPixmap(img_x, img_y, simg_w, simg_h, *ImageFileL, simg_x, simg_y, simg_w, simg_h);
        }
    }

    // hohohoho
    void PutImageToLeftSide()
    {
        // this is a hack. a bad hack. it's used by vertical header sections as WinXP style doesn't have anything like that.
        QTransform t;
        t.rotate(-90.0);
        t.scale(-1, 1);
        ImageFile0 = ImageFile0.transformed(t);
        ImageFile1 = ImageFile1.transformed(t);
        ImageFile2 = ImageFile2.transformed(t);
        ImageFile3 = ImageFile3.transformed(t);
        QMargins SizingMarginsOld = SizingMargins;
        SizingMargins.setLeft(SizingMarginsOld.top());
        SizingMargins.setRight(SizingMarginsOld.bottom());
        SizingMargins.setTop(SizingMarginsOld.right());
        SizingMargins.setBottom(SizingMarginsOld.left());
        ImageLayout = (ImageLayout == Layout_Horizontal) ? Layout_Vertical : Layout_Horizontal;
    }
};

struct LunaticSysMetrics
{
    quint32 ScrollBarWidth;
    quint32 ScrollBarHeight;

    QColor Window;
    QColor MenuBar;
    QColor Menu;
    QColor Background;
    QColor BtnFace;
    QColor Highlight;

    QColor ActiveCaption;
    QColor CaptionText;
    QColor InactiveCaption;
    QColor InactiveCaptionText;
    QColor GradientActiveCaption;
    QColor GradientInactiveCaption;
    QColor HighlightText;
    QColor MenuHighlight;
    QColor BtnShadow;
    QColor GrayText;
    QColor BtnHighlight;
    QColor DkShadow3D;
    QColor Light3D;

    LunaticSysMetrics()
    {
        ScrollBarWidth = 17;
        ScrollBarHeight = 17;
    }

    QColor ReadColorFromString(QString string)
    {
        QStringList s = string.split(" ", QString::SkipEmptyParts);
        if(s.length() != 3)
            return QColor(0, 0, 0, 0);
        QColor c;
        c.setRed(s.at(0).trimmed().toInt());
        c.setGreen(s.at(1).trimmed().toInt());
        c.setBlue(s.at(2).trimmed().toInt());
        return c;
    }

    void MergeMetrics(QCaseInsensitiveMap<QString> map)
    {
        if(map.contains("ScrollBarWidth"))
            ScrollBarWidth = map["ScrollBarWidth"].toInt();
        if(map.contains("ScrollBarHeight"))
            ScrollBarHeight = map["ScrollBarHeight"].toInt();
        if(map.contains("Window"))
            Window = ReadColorFromString(map["Window"]);
        if(map.contains("MenuBar"))
            MenuBar = ReadColorFromString(map["MenuBar"]);
        if(map.contains("Menu"))
            Menu = ReadColorFromString(map["Menu"]);
        if(map.contains("Background"))
            Background = ReadColorFromString(map["Background"]);
        if(map.contains("BtnFace"))
            BtnFace = ReadColorFromString(map["BtnFace"]);
        if(map.contains("Highlight"))
            Highlight = ReadColorFromString(map["Highlight"]);
        if(map.contains("ActiveCaption"))
            ActiveCaption = ReadColorFromString(map["ActiveCaption"]);
        if(map.contains("CaptionText"))
            CaptionText = ReadColorFromString(map["CaptionText"]);
        if(map.contains("InactiveCaptionText"))
            InactiveCaptionText = ReadColorFromString(map["InactiveCaptionText"]);
        if(map.contains("GradientActiveCaption"))
            GradientActiveCaption = ReadColorFromString(map["GradientActiveCaption"]);
        if(map.contains("GradientInactiveCaption"))
            GradientInactiveCaption = ReadColorFromString(map["GradientInactiveCaption"]);
        if(map.contains("HighlightText"))
            HighlightText = ReadColorFromString(map["HighlightText"]);
        if(map.contains("MenuHilight"))
            MenuHighlight = ReadColorFromString(map["MenuHilight"]);
        if(map.contains("BtnShadow"))
            BtnShadow = ReadColorFromString(map["BtnShadow"]);
        if(map.contains("GrayText"))
            GrayText = ReadColorFromString(map["GrayText"]);
        if(map.contains("BtnHighlight"))
            BtnHighlight = ReadColorFromString(map["BtnHighlight"]);
        if(map.contains("DkShadow3D"))
            DkShadow3D = ReadColorFromString(map["DkShadow3D"]);
        if(map.contains("Light3D"))
            Light3D = ReadColorFromString(map["Light3D"]);
    }
};

typedef QCaseInsensitiveMap< QCaseInsensitiveMap< QString > > IniMap;
IniMap ReadIniFromString(QString string);

enum LunaticMask
{
    Mask_PushButton,
    Mask_RadioButton,
    Mask_None
};

struct LunaticSubStyle
{
    IniMap Map;
    QString ReadableName;
    QString Name;
};

class LunaticStyle : public QProxyStyle
{
    Q_OBJECT
public:
    explicit LunaticStyle();
    virtual ~LunaticStyle();

    void resetSubStyle();
    void switchSubStyle(LunaticSubStyle& substyle);

    virtual void polish(QWidget* w);
    virtual void polish(QPalette& p);
    virtual void unpolish(QWidget* w);
    virtual void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = 0) const;
    virtual void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = 0) const;
    virtual bool eventFilter(QObject* object, QEvent* ev);
    virtual int styleHint(StyleHint hint, const QStyleOption* option = 0, const QWidget* widget = 0, QStyleHintReturn* returnData = 0) const;
    virtual int pixelMetric(PixelMetric metric, const QStyleOption* option = 0, const QWidget* widget = 0) const;
    virtual void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget = 0) const;

    QVector<LunaticSubStyle>& getSubStyles() { return SubStyles; } // idk why it's like this

    QRect getToolBarRect(const QWidget* widget) const;

signals:

public slots:

private:
    QVector<LunaticSubStyle> SubStyles;

    bool Active;
    LunaticData Data;

    LunaticSysMetrics SysMetrics;

    LunaticDefinition Button;
    LunaticDefinition ButtonDisabled;

    LunaticDefinition CheckBox;
    LunaticDefinition CheckBoxCheckedDisabled;
    LunaticDefinition CheckBoxUncheckedDisabled;
    LunaticDefinition CheckBoxMixedDisabled;

    LunaticDefinition RadioButton;
    LunaticDefinition RadioButtonCheckedDisabled;
    LunaticDefinition RadioButtonUncheckedDisabled;

    LunaticDefinition GroupBox;

    LunaticDefinition FieldOutline;
    LunaticDefinition FieldOutlineDisabled;
    LunaticDefinition FieldOutlineReadOnly;

    LunaticDefinition ScrollBarButton;
    LunaticDefinition ScrollBarThumbH;
    LunaticDefinition ScrollBarThumbV;
    LunaticDefinition ScrollBarGripperH;
    LunaticDefinition ScrollBarGripperV;
    LunaticDefinition ScrollBarTrackH;
    LunaticDefinition ScrollBarTrackV;

    LunaticDefinition ProgressBarH;
    LunaticDefinition ProgressBarV;
    LunaticDefinition ProgressBarChunkH;
    LunaticDefinition ProgressBarChunkV;

    LunaticDefinition SpinBoxUp;
    LunaticDefinition SpinBoxDown;

    LunaticDefinition SliderTrack;
    LunaticDefinition SliderThumbH;
    LunaticDefinition SliderThumbV;

    LunaticDefinition ComboBox;
    LunaticDefinition ComboBoxHovered;
    LunaticDefinition ComboBoxDisabled;
    LunaticDefinition ComboBoxDropdown;

    LunaticDefinition TabFrame;
    LunaticDefinition TabItemLeft;
    LunaticDefinition TabItemRight;
    LunaticDefinition TabItemBoth;
    LunaticDefinition TabItem;
    LunaticDefinition TabClose;

    LunaticDefinition ListHeader;
    LunaticDefinition ListHeaderV;
    LunaticDefinition ListHeaderItem;
    LunaticDefinition ListHeaderItemV;
    LunaticDefinition ListHeaderArrow;

    LunaticDefinition ToolBarBackground;
    LunaticDefinition ToolBarButton;
    LunaticDefinition ToolBarGripperH;
    LunaticDefinition ToolBarGripperV;

    LunaticDefinition StatusBarBackground;
    LunaticDefinition StatusBarItem;

    LunaticDefinition ResizeGrip;
};

#endif // LUNATICSTYLE_H
