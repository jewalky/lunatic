#include "lunaticstyle.h"
#include <QStyleFactory>
#include <QImage>
#include <QSettings>

#include <QLabel>
#include <QEvent>
#include <QAbstractButton>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStyleOptionSlider>
#include <QAbstractSlider>
#include <QStyleOptionComboBox>
#include <QStyleOptionTab>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QMenuBar>
#include <QMainWindow>
#include <QComboBox>
#include <QHeaderView>
#include <QScrollBar>

IniMap ReadIniFromString(QString string)
{
    IniMap map;
    QStringList spl = string.split("\n");
    QString section = "";
    for(int i = 0; i < spl.size(); i++)
    {
        QString line = spl[i].trimmed();
        int line_comment = line.indexOf(";");
        if(line_comment >= 0)
            line = line.mid(0, line_comment).trimmed();
        if(!line.size())
            continue;

        if(line[0] == '[' || line[line.size()-1] == ']')
        {
            section = line.mid(1, line.size()-2);
            continue;
        }

        if(!line.contains("="))
            continue; // bad value

        if(!section.size())
            continue;

        int sep = line.indexOf("=");
        QString i_k = line.mid(0, sep).trimmed();
        QString i_v = QString(line.mid(sep+1).trimmed());

        map[section][i_k] = i_v;
    }

    return map;
}

LunaticStyle::LunaticStyle()
{
    Active = false;
    this->setBaseStyle(QStyleFactory::create("windows"));
    //Active = Data.LoadFromPE("luna.msstyles"); // we will just see the default style in case something terrible happens
    Active = Data.LoadFromPE("Human.msstyles"); // we will just see the default style in case something terrible happens

    // MS style consists of three parts
    //  - TEXTFILE/THEMES_INI/<lang> = main INI (themes list)
    //  - TEXTFILE/<PART_NAME>_INIT/<lang> = theme INI
    //  - [2]/<SIMPLE_FILE_NAME>/<lang> with SIMPLE_FILE_NAME being a file name from .INI with non-ASCII characters replaced to underscores = theme picture

    // let's load the main INI
    if(Active)
    {
        LunaticPEEntry* themes_ini = Data.GetEntry("textfile/themes_ini");
        if(!themes_ini || !themes_ini->Directory || !themes_ini->Children.size())
        {
            Active = false;
            return;
        }

        //QString activeStyle = "NormalMetallic";
        QString activeStyle = "NormalBlue";
        //QString activeStyle = "NormalHomestead";
        //QString activeStyle = "NormalRadiance";

        QString themes_ini_s = themes_ini->Children[0].GetAsASCII();
        IniMap map = ReadIniFromString(themes_ini_s);

        for(IniMap::iterator it = map.begin(); it != map.end(); ++it)
        {
            //qDebug("key = %s", it.key().toUtf8().data());
            QString s = it.key().toLower();
            QString s_r = it.key();

            if(s.indexOf("file.") == 0)
            {
                if(!map[s].contains("Sizes") || map[s]["Sizes"].toLower() != "normalsize")
                    continue;

                QString file = s.mid(5);
                //if(file != activeStyle.toLower())
                //    continue;

                QString file_s = "textfile/"+file+"_ini";
                //qDebug("file = %s", file_s.toUtf8().data());

                LunaticPEEntry* theme_ini = Data.GetEntry(file_s);
                if(!theme_ini || !theme_ini->Directory || !theme_ini->Children.size())
                {
                    Active = false;
                    return;
                }

                QString theme_ini_s = theme_ini->Children[0].GetAsASCII();
                IniMap theme = ReadIniFromString(theme_ini_s);
                LunaticSubStyle t;
                t.Name = file;
                t.ReadableName = s_r.mid(5);
                t.Map = theme;
                SubStyles.append(t);
            }
        }

        Active = false;
    }
}

void LunaticStyle::resetSubStyle()
{
    Active = false;
}

void LunaticStyle::switchSubStyle(LunaticSubStyle& substyle)
{
    IniMap& theme = substyle.Map;

    if(!theme.size())
    {
        Active = false;
        return;
    }

    Active = true;

    SysMetrics = LunaticSysMetrics();
    SysMetrics.MergeMetrics(theme["sysmetrics"]);

    Button = LunaticDefinition();
    Button.MergeMap(theme["button.pushbutton"], Data);
    ButtonDisabled = Button;
    ButtonDisabled.MergeMap(theme["button.pushbutton(disabled)"], Data);

    CheckBox = LunaticDefinition();
    CheckBox.MergeMap(theme["button.checkbox"], Data);
    CheckBoxMixedDisabled = CheckBoxUncheckedDisabled = CheckBoxCheckedDisabled = CheckBox;
    CheckBoxCheckedDisabled.MergeMap(theme["button.checkbox(checkeddisabled)"], Data);
    CheckBoxUncheckedDisabled.MergeMap(theme["button.checkbox(uncheckeddisabled)"], Data);
    CheckBoxMixedDisabled.MergeMap(theme["button.checkbox(mixeddisabled)"], Data);

    RadioButton = LunaticDefinition();
    RadioButton.MergeMap(theme["button.radiobutton"], Data);
    RadioButtonCheckedDisabled = RadioButtonUncheckedDisabled = RadioButton;
    RadioButtonCheckedDisabled.MergeMap(theme["button.radiobutton(checkeddisabled)"], Data);
    RadioButtonUncheckedDisabled.MergeMap(theme["button.radiobutton(uncheckeddisabled)"], Data);

    GroupBox = LunaticDefinition();
    GroupBox.MergeMap(theme["button.groupbox"], Data); // question, Microsoft: why the fuck is group box a button?!

    FieldOutline = LunaticDefinition();
    FieldOutline.MergeMap(theme["edit"], Data); // beware: we're using spinbox outline for all elements, might cause unexpected behavior with custom themes
    FieldOutlineDisabled = FieldOutlineReadOnly = FieldOutline;
    FieldOutlineDisabled.MergeMap(theme["edit.edittext(disabled)"], Data);
    FieldOutlineReadOnly.MergeMap(theme["edit.edittext(readonly)"], Data);

    ScrollBarButton = LunaticDefinition();
    ScrollBarThumbH = ScrollBarGripperH = LunaticDefinition();
    ScrollBarThumbV = ScrollBarGripperV = LunaticDefinition();
    ScrollBarTrackH = ScrollBarTrackV = LunaticDefinition();
    ScrollBarButton.MergeMap(theme["scrollbar.arrowbtn"], Data);
    ScrollBarThumbH.MergeMap(theme["scrollbar.thumbbtnhorz"], Data);
    ScrollBarThumbV.MergeMap(theme["scrollbar.thumbbtnvert"], Data);
    ScrollBarGripperH.MergeMap(theme["scrollbar.gripperhorz"], Data);
    ScrollBarGripperV.MergeMap(theme["scrollbar.grippervert"], Data);
    ScrollBarTrackH.MergeMap(theme["scrollbar.uppertrackhorz"], Data); // I don't know what's the difference between UpperTrack# and LowerTrack# so we're just using Upper version
    ScrollBarTrackV.MergeMap(theme["scrollbar.uppertrackvert"], Data);

    ProgressBarH = ProgressBarChunkH = LunaticDefinition();
    ProgressBarV = ProgressBarChunkV = LunaticDefinition();
    ProgressBarH.MergeMap(theme["progress.bar"], Data);
    ProgressBarV.MergeMap(theme["progress.barvert"], Data);
    ProgressBarChunkH.MergeMap(theme["progress.chunk"], Data);
    ProgressBarChunkV.MergeMap(theme["progress.chunkvert"], Data);

    SpinBoxUp = SpinBoxDown = LunaticDefinition();
    SpinBoxUp.MergeMap(theme["spin.up"], Data);
    SpinBoxDown.MergeMap(theme["spin.down"], Data);

    SliderTrack = SliderThumbH = SliderThumbV = LunaticDefinition();
    SliderTrack.MergeMap(theme["trackbar.track"], Data);
    SliderThumbH.MergeMap(theme["trackbar.thumb"], Data);
    SliderThumbV.MergeMap(theme["trackbar.thumbvert"], Data);

    ComboBox = LunaticDefinition();
    ComboBoxDropdown = LunaticDefinition();
    ComboBox.MergeMap(theme["combobox"], Data);
    ComboBoxHovered = ComboBoxDisabled = ComboBox;
    ComboBoxHovered.MergeMap(theme["combobox(hot)"], Data);
    ComboBoxDisabled.MergeMap(theme["combobox(disabled)"], Data);
    ComboBoxDropdown.MergeMap(theme["combobox.dropdownbutton"], Data);

    TabFrame = LunaticDefinition();
    TabFrame.MergeMap(theme["tab.pane"], Data);
    TabItem = TabItemLeft = TabItemRight = TabItemBoth = LunaticDefinition();
    TabItem.MergeMap(theme["tab.tabitem"], Data);
    TabItemLeft.MergeMap(theme["tab.tabitemleftedge"], Data);
    TabItemRight.MergeMap(theme["tab.tabitemrightedge"], Data);
    TabItemBoth.MergeMap(theme["tab.tabitembothedge"], Data);
    TabClose = LunaticDefinition();
    TabClose.MergeMap(theme["tooltip.close"], Data);

    ListHeader = LunaticDefinition();
    ListHeaderItem = LunaticDefinition();
    ListHeader.MergeMap(theme["header"], Data);
    ListHeaderItem.MergeMap(theme["header.headeritem"], Data);
    ListHeaderItemV = ListHeaderItem;
    ListHeaderItemV.PutImageToLeftSide();
    ListHeaderV = ListHeader;
    ListHeaderV.PutImageToLeftSide();
    ListHeaderArrow = ScrollBarButton;
    ListHeaderArrow.ImageFile0 = QPixmap(); // note that this is a hack as well... I'm too lazy to redesign the painter function so it can only paint PARTS of a compound element

    ToolBarBackground = LunaticDefinition();
    ToolBarBackground.MergeMap(theme["rebar"], Data);
    ToolBarButton = LunaticDefinition();
    ToolBarButton.MergeMap(theme["toolbar.button"], Data);
    ToolBarGripperH = ToolBarGripperV = LunaticDefinition();
    ToolBarGripperH.MergeMap(theme["rebar.gripper"], Data);
    ToolBarGripperV.MergeMap(theme["rebar.grippervert"], Data);

    //LunaticDefinition StatusBarBackground;
    //LunaticDefinition StatusBarItem;

    StatusBarBackground = StatusBarItem = LunaticDefinition();
    StatusBarBackground.MergeMap(theme["status"], Data);
    StatusBarItem.MergeMap(theme["status.pane"], Data);

    ResizeGrip = LunaticDefinition();
    ResizeGrip.MergeMap(theme["status.gripper"], Data);
}

LunaticStyle::~LunaticStyle()
{
    this->setBaseStyle(0);
}

void LunaticStyle::drawControl(QStyle::ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    if(!Active)
    {
        QProxyStyle::drawControl(element, option, painter, widget);
        return;
    }

    switch(element)
    {
    case QStyle::CE_RadioButton:
    {
        // here we need to draw everything manually because Win9x's radio indicator is too small by default (12 vs 13 in XP)
        QStyleOption o2 = *option;
        o2.rect.setLeft(o2.rect.left()+18);
        drawControl(QStyle::CE_RadioButtonLabel, &o2, painter, widget);
        QStyleOption o3 = *option;
        QRect r3;
        r3.setX(o3.rect.x());
        r3.setY(o3.rect.y()+o3.rect.height()/2-6);
        r3.setWidth(13);
        r3.setHeight(13);
        o3.rect = r3;
        drawPrimitive(QStyle::PE_IndicatorRadioButton, &o3, painter, widget);
        return;
    }
    case QStyle::CE_RadioButtonLabel:
    {
        const QAbstractButton* l = dynamic_cast<const QAbstractButton*>(widget);
        if(!l) break;

        QTextOption opts;
        opts.setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

        QColor c;
        if(!(option->state & QStyle::State_Enabled))
        {
            if(option->state & QStyle::State_On)
                c = RadioButtonCheckedDisabled.TextColor;
            else c = RadioButtonUncheckedDisabled.TextColor;
        }
        else c = RadioButton.TextColor;

        painter->setPen(c);
        painter->drawText(option->rect, l->text(), opts);

        return;
    }
    case QStyle::CE_CheckBoxLabel:
    {
        const QAbstractButton* l = dynamic_cast<const QAbstractButton*>(widget);
        if(!l) break;

        QTextOption opts;
        opts.setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

        QColor c;
        if(!(option->state & QStyle::State_Enabled))
        {
            if(option->state & QStyle::State_NoChange)
                c = CheckBoxMixedDisabled.TextColor;
            else if(option->state & QStyle::State_On)
                c = CheckBoxCheckedDisabled.TextColor;
            else c = CheckBoxUncheckedDisabled.TextColor;
        }
        else c = CheckBox.TextColor;

        painter->setPen(c);
        painter->drawText(option->rect, l->text(), opts);

        return;
    }
    case QStyle::CE_PushButtonLabel:
    {
        const QAbstractButton* l = dynamic_cast<const QAbstractButton*>(widget);
        if(!l) break;

        QTextOption opts;
        opts.setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

        QColor c;
        if(!(option->state & QStyle::State_Enabled))
            c = ButtonDisabled.TextColor;
        else c = Button.TextColor;

        painter->setPen(c);

        if(option->state & QStyle::State_Sunken)
        {
            QRect rr = option->rect;
            rr.moveTo(rr.x()+1, rr.y()+1);
            painter->drawText(rr, l->text(), opts);
        }
        else
        {
            painter->drawText(option->rect, l->text(), opts);
        }
        return;
    }
    case QStyle::CE_ScrollBarSubLine:
    case QStyle::CE_ScrollBarAddLine:
    {
        QRect xRect = option->rect;

        quint32 base = 0;
        if(option->state & QStyle::State_Horizontal)
        {
            xRect.setRight(xRect.right()+1);
            xRect.setBottom(xRect.bottom()+1);
            if(element == QStyle::CE_ScrollBarAddLine)
                base = 12;
            else base = 8;
            ScrollBarTrackH.DrawAt(painter, option->rect, 0, 0, 0);
        }
        else
        {
            xRect.setRight(xRect.right()+1);
            xRect.setBottom(xRect.bottom()+1);
            if(element == QStyle::CE_ScrollBarAddLine)
                base = 4;
            else base = 0;
            ScrollBarTrackV.DrawAt(painter, option->rect, 0, 0, 0);
        }


        if(!(option->state & QStyle::State_Enabled))
            ScrollBarButton.DrawAt(painter, xRect, 0, base+3, 0);
        else if(option->state & QStyle::State_Sunken)
            ScrollBarButton.DrawAt(painter, xRect, 0, base+2, 0);
        else if(option->state & QStyle::State_MouseOver)
            ScrollBarButton.DrawAt(painter, xRect, 0, base+1, 0);
        else ScrollBarButton.DrawAt(painter, xRect, 0, base, 0);

        return;
    }
    case QStyle::CE_ScrollBarSlider:
    {
        QRect xRect = option->rect;
        QRect gripperRect;
        gripperRect.setX(option->rect.x()+option->rect.width()/2-4);
        gripperRect.setY(option->rect.y()+option->rect.height()/2-4);
        gripperRect.setWidth(8);
        gripperRect.setHeight(8);

        if(option->state & QStyle::State_Horizontal)
        {
            xRect.setRight(xRect.right()+1);
            xRect.setBottom(xRect.bottom()+1);
            if(!(option->state & QStyle::State_Enabled))
                ScrollBarThumbH.DrawAt(painter, xRect, 0, 3, 0);
            else if(option->state & QStyle::State_Sunken)
                ScrollBarThumbH.DrawAt(painter, xRect, 0, 2, 0);
            else if(option->state & QStyle::State_MouseOver)
                ScrollBarThumbH.DrawAt(painter, xRect, 0, 1, 0);
            else ScrollBarThumbH.DrawAt(painter, xRect, 0, 0, 0);
            if(xRect.width() > 16) // if scrollbar is wide enough
            {
                if(!(option->state & QStyle::State_Enabled))
                    ScrollBarGripperH.DrawAt(painter, gripperRect, 0, 3, 0);
                else if(option->state & QStyle::State_Sunken)
                    ScrollBarGripperH.DrawAt(painter, gripperRect, 0, 2, 0);
                else if(option->state & QStyle::State_MouseOver)
                    ScrollBarGripperH.DrawAt(painter, gripperRect, 0, 1, 0);
                else ScrollBarGripperH.DrawAt(painter, gripperRect, 0, 0, 0);
            }
        }
        else
        {
            xRect.setRight(xRect.right()+1);
            xRect.setBottom(xRect.bottom()+1);
            if(!(option->state & QStyle::State_Enabled))
                ScrollBarThumbV.DrawAt(painter, xRect, 0, 3, 0);
            else if(option->state & QStyle::State_Sunken)
                ScrollBarThumbV.DrawAt(painter, xRect, 0, 2, 0);
            else if(option->state & QStyle::State_MouseOver)
                ScrollBarThumbV.DrawAt(painter, xRect, 0, 1, 0);
            else ScrollBarThumbV.DrawAt(painter, xRect, 0, 0, 0);
            if(xRect.height() > 16) // if scrollbar is wide enough
            {
                if(!(option->state & QStyle::State_Enabled))
                    ScrollBarGripperV.DrawAt(painter, gripperRect, 0, 3, 0);
                else if(option->state & QStyle::State_Sunken)
                    ScrollBarGripperV.DrawAt(painter, gripperRect, 0, 2, 0);
                else if(option->state & QStyle::State_MouseOver)
                    ScrollBarGripperV.DrawAt(painter, gripperRect, 0, 1, 0);
                else ScrollBarGripperV.DrawAt(painter, gripperRect, 0, 0, 0);
            }
        }
        return;
    }
    case QStyle::CE_ProgressBar:
    {
        const QStyleOptionProgressBar* p = qstyleoption_cast<const QStyleOptionProgressBar*>(option);
        if(!p) break;
        drawControl(QStyle::CE_ProgressBarGroove, option, painter, widget);
        QStyleOptionProgressBar g = *p;
        if(p->state & QStyle::State_Horizontal)
        {
            g.rect.setLeft(g.rect.left()+ProgressBarH.SizingMargins.left());
            g.rect.setRight(g.rect.right()-+ProgressBarH.SizingMargins.right());
            g.rect.setTop(g.rect.top()+ProgressBarH.SizingMargins.top());
            g.rect.setBottom(g.rect.bottom()-ProgressBarH.SizingMargins.bottom()-1);
        }
        else
        {
            g.rect.setLeft(g.rect.left()+ProgressBarV.SizingMargins.left());
            g.rect.setRight(g.rect.right()-+ProgressBarV.SizingMargins.right()-1);
            g.rect.setTop(g.rect.top()+ProgressBarV.SizingMargins.top());
            g.rect.setBottom(g.rect.bottom()-ProgressBarV.SizingMargins.bottom()-1);
        }
        drawControl(QStyle::CE_ProgressBarContents, &g, painter, widget);
        return;
    }
    case QStyle::CE_ProgressBarGroove:
    {
        if(option->state & QStyle::State_Horizontal)
            ProgressBarH.DrawAt(painter, option->rect, 0, 0, 0);
        else ProgressBarV.DrawAt(painter, option->rect, 0, 0, 0);
        return;
    }
    case QStyle::CE_ProgressBarContents:
    {
        QRect r2 = option->rect;
        // one chunk is...idk 10 pixels wide?
        const QStyleOptionProgressBar* p = qstyleoption_cast<const QStyleOptionProgressBar*>(option);
        if(!p) break;
        float progress = (float)(p->progress - p->minimum) / (float)(p->maximum - p->minimum);
        painter->setClipRect(option->rect);
        if(option->state & QStyle::State_Horizontal)
        {
            quint32 prog_pixels = (int)(((float)r2.width() * progress) / 10) * 10; // clamp to nearest 10
            r2.setWidth(prog_pixels);
            ProgressBarChunkH.DrawAt(painter, r2, 0, 0, 0);
        }
        else
        {
            quint32 prog_pixels = (int)(((float)r2.height() * progress) / 10) * 10;
            r2.setTop(option->rect.bottom()-prog_pixels+1);
            r2.setHeight(prog_pixels);
            ProgressBarChunkV.DrawAt(painter, r2, 0, 0, 0);
        }
        painter->setClipping(false);
        return;
    }
    case QStyle::CE_TabBarTabShape:
    {
        const QStyleOptionTab* opt_t = qstyleoption_cast<const QStyleOptionTab*>(option);
        QRect rec_tab = option->rect;

        bool active = (option->state & QStyle::State_Selected);

        if(!active || !(option->state & QStyle::State_Enabled))
        {
            rec_tab.setBottom(rec_tab.bottom()-1);
            rec_tab.setTop(rec_tab.top()+1);
        }
        else rec_tab.setBottom(rec_tab.bottom()+1);
        if(opt_t->position == QStyleOptionTab::Beginning)
            rec_tab.setLeft(rec_tab.left()+1);
        else if(opt_t->position == QStyleOptionTab::End)
            rec_tab.setRight(rec_tab.right()-1);

        if(!(option->state & QStyle::State_Enabled))
            TabItem.DrawAt(painter, rec_tab, 0, 3, 0);
        else if(option->state & QStyle::State_Selected)
            TabItem.DrawAt(painter, rec_tab, 0, 2, 0);
        else if(option->state & QStyle::State_MouseOver)
            TabItem.DrawAt(painter, rec_tab, 0, 1, 0);
        else TabItem.DrawAt(painter, rec_tab, 0, 0, 0);

        return;
    }
    case QStyle::CE_HeaderEmptyArea:
    {
        QRect r2 = option->rect;
        r2.setRight(r2.right()+1);
        r2.setBottom(r2.bottom()+1);
        if(option->state & QStyle::State_Horizontal)
            ListHeader.DrawAt(painter, r2, 0, 0, 0);
        else ListHeaderV.DrawAt(painter, r2, 0, 0, 0);
        return;
    }
    case QStyle::CE_HeaderSection:
    {
        QRect r2 = option->rect;
        //r2.setLeft(r2.left()+1);
        r2.setRight(r2.right()+1);
        r2.setBottom(r2.bottom()+1);
        if(option->state & QStyle::State_Horizontal)
        {
            ListHeader.DrawAt(painter, r2, 0, 0, 0);
            if(!(option->state & QStyle::State_Enabled))
                ListHeaderItem.DrawAt(painter, r2, 0, 3, 0);
            else if(option->state & QStyle::State_Sunken)
                ListHeaderItem.DrawAt(painter, r2, 0, 2, 0);
            else if(option->state & QStyle::State_MouseOver)
                ListHeaderItem.DrawAt(painter, r2, 0, 1, 0);
            else ListHeaderItem.DrawAt(painter, r2, 0, 0, 0);
        }
        else
        {
            ListHeaderV.DrawAt(painter, r2, 0, 0, 0);
            if(!(option->state & QStyle::State_Enabled))
                ListHeaderItemV.DrawAt(painter, r2, 0, 3, 0);
            else if(option->state & QStyle::State_Sunken)
                ListHeaderItemV.DrawAt(painter, r2, 0, 2, 0);
            else if(option->state & QStyle::State_MouseOver)
                ListHeaderItemV.DrawAt(painter, r2, 0, 1, 0);
            else ListHeaderItemV.DrawAt(painter, r2, 0, 0, 0);
        }
        return;
    }
    case QStyle::CE_MenuBarEmptyArea:
    {
        QRect tbRect = getToolBarRect(widget);

        // okay, this is _VERY_ evil
        // why the fuck does QMenuBar draw background AFTER the actions?
        const QMenuBar* mb = qobject_cast<const QMenuBar*>(widget); // also just saying, this...feature makes the theme more laggy (because lots of clipping everywhere)
        if(!mb) break;

        QPainterPath p;
        QList<QAction*> mb_actions = mb->actions();
        for(int i = 0; i < mb_actions.size(); i++)
            p.addRect(mb->actionGeometry(mb_actions.at(i)));
        QPainterPath p2;
        p2.addRect(option->rect);
        painter->setClipPath(p2.subtracted(p));
        ToolBarBackground.DrawAt(painter, tbRect, 0, 0, 0);
        painter->setClipping(false);
        return;
    }
    case QStyle::CE_MenuBarItem:
    {
        const QStyleOptionMenuItem* opt_m = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
        if(!opt_m) break;
        QTextOption o;
        o.setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

        QPen old_pen = painter->pen();

        if(option->state & QStyle::State_Enabled)
        {
            //if(option->state & QStyle::State_Sunken|QStyle::State_MouseOver)
            if(option->state & QStyle::State_Selected)//|QStyle::State_Sunken)
            {
                painter->fillRect(option->rect, SysMetrics.MenuHighlight);
                painter->setPen(SysMetrics.HighlightText);
            }
            else
            {
                QRect tbRect = getToolBarRect(widget);
                painter->setClipRect(option->rect);
                ToolBarBackground.DrawAt(painter, tbRect, 0, 0, 0);
                painter->setClipping(false);
                painter->setPen(Button.TextColor);
            }
        }
        else
        {
            painter->setPen(ButtonDisabled.TextColor);
        }

        painter->drawText(option->rect, opt_m->text, o);
        painter->setPen(old_pen);
        return;
    }
    case QStyle::CE_MenuItem:
    {
        const QStyleOptionMenuItem* opt_m = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
        if(!opt_m) break;
        if(opt_m->menuItemType == QStyleOptionMenuItem::Separator)
        {
            qint32 mO = option->rect.y()+option->rect.height()/2;
            QPen old_pen = painter->pen();
            painter->fillRect(option->rect, SysMetrics.Menu);
            painter->setPen(SysMetrics.DkShadow3D);
            painter->drawLine(option->rect.left(), mO, option->rect.right(), mO);
            painter->setPen(old_pen);
            return;
        }

        QStyleOptionMenuItem opt_m2 = *opt_m;
        opt_m2.rect.setRight(opt_m2.rect.right()+1);
        QProxyStyle::drawControl(element, &opt_m2, painter, widget);

        return;
    }
    case QStyle::CE_MenuEmptyArea:
        painter->fillRect(option->rect, SysMetrics.Menu);
        return;
    case QStyle::CE_ToolBar:
    {
        QRect tbRect = getToolBarRect(widget);
        painter->setClipRect(option->rect);
        ToolBarBackground.DrawAt(painter, tbRect, 0, 0, 0);
        painter->setClipping(false);
        return;
    }
    case QStyle::CE_SizeGrip:
    {
        ResizeGrip.DrawAt(painter, option->rect, 0, 0, 0);
        return;
    }
    default:
        break;
    }

    QProxyStyle::drawControl(element, option, painter, widget);
}

QRect __getGlobalRect(const QWidget* widget)
{
    QRect absRect = widget->geometry();
    const QWidget* p = widget->parentWidget();
    if(!p) return absRect;
    absRect.moveTopLeft(p->mapToGlobal(absRect.topLeft()));
    return absRect;
}

QRect __getLocalRect(const QWidget* widget, const QRect& rec)
{
    QRect absRect = rec;
    absRect.moveTopLeft(widget->mapFromGlobal(absRect.topLeft()));
    return absRect;
}

QRect LunaticStyle::getToolBarRect(const QWidget* widget) const
{
    if(!widget) return QRect();
    // ok this is a bit tricky..heh
    // we need to guess the topmost attached toolbar
    QWidget* p = widget->parentWidget();
    if(!p || !p->isWindow()) return QRect();
    //qDebug("PARENT IS WINDOW");
    // ok now we can search for other toolbars there
    QRect wAbsRect = __getGlobalRect(widget);
    QRect wndAbsRect = __getGlobalRect(p);

    QRect outRect;
    outRect.setLeft(wndAbsRect.left());
    outRect.setRight(wndAbsRect.right());
    outRect.setTop(wndAbsRect.top());
    outRect.setBottom(wndAbsRect.top());

    const QObjectList& l = p->children();
    for(int i = 0; i < l.size(); i++)
    {
        const QWidget* cw = qobject_cast<const QWidget*>(l.at(i));
        const QMenuBar* mb = qobject_cast<const QMenuBar*>(l.at(i));
        const QToolBar* tb = qobject_cast<const QToolBar*>(l.at(i));

        if(!mb && !tb)
            continue;

        QRect cAbsRect = __getGlobalRect(cw);
        if(cAbsRect.bottom() > outRect.bottom())
            outRect.setBottom(cAbsRect.bottom());
    }

    //outRect = __getLocalRect(p, outRect);
    outRect = __getLocalRect(widget, outRect);

    //qDebug("outRect = [%d,%d,%d,%d]", outRect.left(), outRect.right(), outRect.top(), outRect.bottom());

    return outRect;
}

void LunaticStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    if(!Active)
    {
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        return;
    }

    switch(element)
    {
    case QStyle::PE_FrameDefaultButton:
        return; // don't draw this
    case QStyle::PE_FrameFocusRect:
        if(dynamic_cast<const QAbstractButton*>(widget))
            return; // don't draw this on buttons
        break;
    case QStyle::PE_PanelButtonCommand:
    {
        const QStyleOptionButton* b = qstyleoption_cast<const QStyleOptionButton*>(option);
        if(option->state & QStyle::State_Enabled)
        {
            if(option->state & QStyle::State_Sunken)
                Button.DrawAt(painter, option->rect, 0, 2, 0);
            else if(option->state & QStyle::State_MouseOver)
                Button.DrawAt(painter, option->rect, 0, 1, 0);
            else if((option->state & QStyle::State_HasFocus) || (b && b->features & QStyleOptionButton::DefaultButton)) // not sure what XP does here
                Button.DrawAt(painter, option->rect, 0, 4, 0);
            else Button.DrawAt(painter, option->rect, 0, 0, 0);
        }
        else
        {
            Button.DrawAt(painter, option->rect, 0, 3, 0);
        }

        return;
    }
    case QStyle::PE_IndicatorCheckBox:
    {
        if(!(option->state & QStyle::State_Enabled))
        {
            if(option->state & QStyle::State_NoChange)
                CheckBox.DrawAt(painter, option->rect, 1, 11, 0);
            else if(option->state & QStyle::State_On)
                CheckBox.DrawAt(painter, option->rect, 1, 7, 0);
            else CheckBox.DrawAt(painter, option->rect, 1, 3, 0);
        }
        else if(option->state & QStyle::State_NoChange)
        {
            if(option->state & QStyle::State_Sunken)
                CheckBox.DrawAt(painter, option->rect, 1, 10, 0);
            else if(option->state & QStyle::State_MouseOver)
                CheckBox.DrawAt(painter, option->rect, 1, 9, 0);
            else CheckBox.DrawAt(painter, option->rect, 1, 8, 0);
        }
        else if(option->state & QStyle::State_On)
        {
            if(option->state & QStyle::State_Sunken)
                CheckBox.DrawAt(painter, option->rect, 1, 6, 0);
            else if(option->state & QStyle::State_MouseOver)
                CheckBox.DrawAt(painter, option->rect, 1, 5, 0);
            else CheckBox.DrawAt(painter, option->rect, 1, 4, 0);
        }
        else
        {
            if(option->state & QStyle::State_Sunken)
                CheckBox.DrawAt(painter, option->rect, 1, 2, 0);
            else if(option->state & QStyle::State_MouseOver)
                CheckBox.DrawAt(painter, option->rect, 1, 1, 0);
            else CheckBox.DrawAt(painter, option->rect, 1, 0, 0);
        }

        return;
    }
    case QStyle::PE_IndicatorRadioButton:
    {
        if(!(option->state & QStyle::State_Enabled))
        {
            if(option->state & QStyle::State_On)
                RadioButton.DrawAt(painter, option->rect, 1, 7, 0);
            else RadioButton.DrawAt(painter, option->rect, 1, 3, 0);
        }
        else if(option->state & QStyle::State_On)
        {
            if(option->state & QStyle::State_Sunken)
                RadioButton.DrawAt(painter, option->rect, 1, 6, 0);
            else if(option->state & QStyle::State_MouseOver)
                RadioButton.DrawAt(painter, option->rect, 1, 5, 0);
            else RadioButton.DrawAt(painter, option->rect, 1, 4, 0);
        }
        else
        {
            if(option->state & QStyle::State_Sunken)
                RadioButton.DrawAt(painter, option->rect, 1, 2, 0);
            else if(option->state & QStyle::State_MouseOver)
                RadioButton.DrawAt(painter, option->rect, 1, 1, 0);
            else RadioButton.DrawAt(painter, option->rect, 1, 0, 0);
        }

        return;
    }
    case QStyle::PE_FrameGroupBox:
    {
        GroupBox.DrawAt(painter, option->rect, 0, 0, 0);
        return;
    }
    case QStyle::PE_Frame:
    case QStyle::PE_FrameLineEdit:
    {
        if(!(option->state & QStyle::State_Enabled))
            FieldOutlineDisabled.DrawAt(painter, option->rect, 0, 0, 0);
        else if(option->state & QStyle::State_ReadOnly)
            FieldOutlineReadOnly.DrawAt(painter, option->rect, 0, 0, 0);
        else FieldOutline.DrawAt(painter, option->rect, 0, 0, 0);
        return;
    }
    case QStyle::PE_FrameTabWidget:
    {
        TabFrame.DrawAt(painter, option->rect, 0, 0, 0);
        return;
    }
    case QStyle::PE_IndicatorTabClose:
    {
        QRect nRect = option->rect;
        nRect.setLeft(nRect.left()+1);
        nRect.setTop(nRect.top()+1);
        if(!(option->state & QStyle::State_Enabled))
            TabClose.DrawAt(painter, nRect, 0, 0, 0);
        else if(option->state & QStyle::State_Sunken)
            TabClose.DrawAt(painter, nRect, 0, 2, 0);
        else if(option->state & QStyle::State_MouseOver)
            TabClose.DrawAt(painter, nRect, 0, 1, 0);
        else TabClose.DrawAt(painter, nRect, 0, 0, 0);
        return;
    }
    case QStyle::PE_IndicatorHeaderArrow:
    {
        // I wonder what's used here in the original theme
        QRect r2 = option->rect;
        r2.setY(r2.y()+3);
        // ok now THIS was copied from QCommonStyle.cpp
        // Qt documentation says that we're supposed to use QStyle::State_UpArrow...
        // two lame hacks in one! :)

        QPolygon poly;
        bool hqa_prev = (painter->renderHints() & QPainter::HighQualityAntialiasing);
        painter->setRenderHint(QPainter::HighQualityAntialiasing, true);

        if(const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option))
        {
            if(header->sortIndicator & QStyleOptionHeader::SortUp)
            {
                // draw up arrow with ...text color?

                poly.append(QPoint(r2.left(), r2.top()));
                poly.append(QPoint(r2.left()+9, r2.top()));
                poly.append(QPoint(r2.left()+4, r2.top()+5));

                //ListHeaderArrow.DrawAt(painter, r2, 0, 0+3, 0);
            }
            else if(header->sortIndicator & QStyleOptionHeader::SortDown)
            {
                //ListHeaderArrow.DrawAt(painter, r2, 0, 4+3, 0);

                poly.append(QPoint(r2.left(), r2.top()+5));
                poly.append(QPoint(r2.left()+9, r2.top()+5));
                poly.append(QPoint(r2.left()+4, r2.top()));
            }
        }

        QPainterPath p;
        p.addPolygon(poly);
        painter->fillPath(p, SysMetrics.DkShadow3D);

        painter->setRenderHint(QPainter::HighQualityAntialiasing, hqa_prev);

        return;
    }
    case QStyle::PE_FrameMenu:
    {
        QRect r2 = option->rect;
        r2.setWidth(r2.width()-1);
        r2.setHeight(r2.height()-1); // right/bottom borders don't get drawn otherwise (out of menu window)

        QPen old_pen = painter->pen();
        QBrush old_brush = painter->brush();
        painter->setPen(SysMetrics.Background);
        painter->setBrush(SysMetrics.Menu);
        painter->drawRect(r2);
        painter->setBrush(old_brush);
        painter->setPen(old_pen);
        return;
    }
    case QStyle::PE_PanelButtonTool:
    {
        if(!(option->state & QStyle::State_Enabled))
            ToolBarButton.DrawAt(painter, option->rect, 0, 3, 0);
        else if(option->state & QStyle::State_Sunken)
            ToolBarButton.DrawAt(painter, option->rect, 0, 2, 0);
        else if(option->state & QStyle::State_On) // one exception to one-liner if-else
        {
            if(option->state & QStyle::State_MouseOver)
                ToolBarButton.DrawAt(painter, option->rect, 0, 5, 0);
            else ToolBarButton.DrawAt(painter, option->rect, 0, 4, 0);
        }
        else if(option->state & QStyle::State_MouseOver)
            ToolBarButton.DrawAt(painter, option->rect, 0, 1, 0);
        else ToolBarButton.DrawAt(painter, option->rect, 0, 0, 0);
        return;
    }
    case QStyle::PE_IndicatorToolBarHandle:
    {
        ToolBarGripperH.DrawAt(painter, option->rect, 0, 0, 0);
        return;
    }
    case QStyle::PE_FrameStatusBarItem:
    {
        StatusBarItem.DrawAt(painter, option->rect, 0, 0, 0);
        return;
    }
    case QStyle::PE_PanelStatusBar:
    {
        StatusBarBackground.DrawAt(painter, option->rect, 0, 0, 0);
        return;
    }
    default:
        break;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

static bool TestWidgetShouldHaveHover(QWidget* w)
{
    //qDebug("widget = %s", w->metaObject()->className());
    if (dynamic_cast<QComboBox*>(w) ||
        dynamic_cast<QAbstractButton*>(w) ||
        dynamic_cast<QSlider*>(w) ||
        dynamic_cast<QHeaderView*>(w) ||
        dynamic_cast<QTabBar*>(w) ||
        dynamic_cast<QScrollBar*>(w))
            return true;
    return false;
}

void LunaticStyle::polish(QWidget* w)
{
    if(!Active)
    {
        QProxyStyle::polish(w);
        return;
    }

    if (TestWidgetShouldHaveHover(w))
    {
        w->setProperty("lunatic::hover", w->testAttribute(Qt::WA_Hover));
        w->setAttribute(Qt::WA_Hover, true);
    }
    QProxyStyle::polish(w);
}

void LunaticStyle::unpolish(QWidget* w)
{
    if (!Active)
    {
        QProxyStyle::unpolish(w);
        return;
    }

    QProxyStyle::unpolish(w);
    if (TestWidgetShouldHaveHover(w))
    {
        w->setAttribute(Qt::WA_Hover, w->property("lunatic::hover").toBool());
    }
}

bool LunaticStyle::eventFilter(QObject* object, QEvent* ev)
{
    if(!Active)
    {
        return QProxyStyle::eventFilter(object, ev);
    }

    return false;
}

int LunaticStyle::styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const
{
    if(!Active)
    {
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

    switch(hint)
    {
    case QStyle::SH_EtchDisabledText:
        return 0;
    case QStyle::SH_GroupBox_TextLabelColor:
        if(option->state & QStyle::State_Enabled)
            return GroupBox.TextColor.rgb();
        return ButtonDisabled.TextColor.rgb();
    case QStyle::SH_TabBar_CloseButtonPosition:
        return 0;
    default:
        break;
    }

    return QProxyStyle::styleHint(hint, option, widget, returnData);
}

int LunaticStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
    if(!Active)
    {
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    switch(metric)
    {
    case QStyle::PM_ScrollBarExtent:
        return 17;
    default:
        break;
    }

    return QProxyStyle::pixelMetric(metric, option, widget);
}

void LunaticStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
{
    if(!Active)
    {
        QProxyStyle::drawComplexControl(control, option, painter, widget);
        return;
    }

    switch(control)
    {
    case QStyle::CC_ScrollBar:
    {
        QStyleOption opt = *option;
        const QStyleOptionSlider* opt_s = qstyleoption_cast<const QStyleOptionSlider*>(option);
        painter->setClipping(false);
        if(!opt_s) break;
        if(opt_s->minimum == opt_s->maximum)
            opt.state &= ~QStyle::State_Enabled;
        if(option->state & QStyle::State_Horizontal)
        {
            QRect rec_down = subControlRect(control, option, QStyle::SC_ScrollBarAddLine, widget);
            QRect rec_up = subControlRect(control, option, QStyle::SC_ScrollBarSubLine, widget);
            QRect rec_slider = subControlRect(control, option, QStyle::SC_ScrollBarSlider, widget);
            QRect rec_track = option->rect;
            rec_track.setLeft(rec_track.left()-1);
            rec_track.setBottom(rec_track.bottom()+1);

            /*
            if(!(option->state & QStyle::State_Enabled))
                ScrollBarTrackH.DrawAt(painter, rec_track, 0, 3, 0);
            else if(option->state & QStyle::State_MouseOver)
                ScrollBarTrackH.DrawAt(painter, rec_track, 0, 1, 0);
            else if(option->state & QStyle::State_Sunken)
                ScrollBarTrackH.DrawAt(painter, rec_track, 0, 2, 0);
            else */ // I did support highlighting trackbars before, but apparently some themes don't even have a valid picture slot for mouseovered trackbar
            ScrollBarTrackH.DrawAt(painter, rec_track, 0, 0, 0);
            //qDebug("painter: %d,%d", painter->device()->width(), painter->device()->height());
            //qDebug("drawing track at %d,%d,%d,%d", rec_track.x(), rec_track.y(), rec_track.width(), rec_track.height());
            opt.rect = rec_down;
            opt.state = (option->activeSubControls == QStyle::SC_ScrollBarAddLine) ? opt.state | QStyle::State_MouseOver : opt.state &= ~QStyle::State_MouseOver;
            opt.state = (opt.state & QStyle::State_MouseOver && option->state & QStyle::State_Sunken) ? opt.state | QStyle::State_Sunken : opt.state &= ~QStyle::State_Sunken;
            drawControl(QStyle::CE_ScrollBarAddLine, &opt, painter, widget);
            opt.rect = rec_up;
            opt.state = (option->activeSubControls == QStyle::SC_ScrollBarSubLine) ? opt.state | QStyle::State_MouseOver : opt.state &= ~QStyle::State_MouseOver;
            opt.state = (opt.state & QStyle::State_MouseOver && option->state & QStyle::State_Sunken) ? opt.state | QStyle::State_Sunken : opt.state &= ~QStyle::State_Sunken;
            drawControl(QStyle::CE_ScrollBarSubLine, &opt, painter, widget);
            // ok now let's check if we can slide
            if(opt_s->minimum != opt_s->maximum)
            {
                opt.rect = rec_slider;
                opt.state = (option->activeSubControls == QStyle::SC_ScrollBarSlider) ? opt.state | QStyle::State_MouseOver : opt.state &= ~QStyle::State_MouseOver;
                opt.state = (opt.state & QStyle::State_MouseOver && option->state & QStyle::State_Sunken) ? opt.state | QStyle::State_Sunken : opt.state &= ~QStyle::State_Sunken;
                drawControl(QStyle::CE_ScrollBarSlider, &opt, painter, widget);
            }
        }
        else
        {
            QRect rec_right = subControlRect(control, option, QStyle::SC_ScrollBarAddLine, widget);
            QRect rec_left = subControlRect(control, option, QStyle::SC_ScrollBarSubLine, widget);
            QRect rec_slider = subControlRect(control, option, QStyle::SC_ScrollBarSlider, widget);
            QRect rec_track = option->rect;
            rec_track.setTop(rec_track.top()-1);
            rec_track.setRight(rec_track.right()+1);
            /*if(!(option->state & QStyle::State_Enabled))
                ScrollBarTrackV.DrawAt(painter, rec_track, 0, 3, 0);
            else if(option->state & QStyle::State_MouseOver)
                ScrollBarTrackV.DrawAt(painter, rec_track, 0, 1, 0);
            else if(option->state & QStyle::State_Sunken)
                ScrollBarTrackV.DrawAt(painter, rec_track, 0, 2, 0);
            else */ // see above, fine features removed for the sake of compatibility with XP
            ScrollBarTrackV.DrawAt(painter, rec_track, 0, 0, 0);
            opt.rect = rec_right;
            opt.state = (option->activeSubControls == QStyle::SC_ScrollBarAddLine) ? opt.state | QStyle::State_MouseOver : opt.state &= ~QStyle::State_MouseOver;
            opt.state = (opt.state & QStyle::State_MouseOver && option->state & QStyle::State_Sunken) ? opt.state | QStyle::State_Sunken : opt.state &= ~QStyle::State_Sunken;
            drawControl(QStyle::CE_ScrollBarAddLine, &opt, painter, widget);
            opt.rect = rec_left;
            opt.state = (option->activeSubControls == QStyle::SC_ScrollBarSubLine) ? opt.state | QStyle::State_MouseOver : opt.state &= ~QStyle::State_MouseOver;
            opt.state = (opt.state & QStyle::State_MouseOver && option->state & QStyle::State_Sunken) ? opt.state | QStyle::State_Sunken : opt.state &= ~QStyle::State_Sunken;
            drawControl(QStyle::CE_ScrollBarSubLine, &opt, painter, widget);
            if(opt_s->minimum != opt_s->maximum)
            {
                opt.rect = rec_slider;
                opt.state = (option->activeSubControls == QStyle::SC_ScrollBarSlider) ? opt.state | QStyle::State_MouseOver : opt.state &= ~QStyle::State_MouseOver;
                opt.state = (opt.state & QStyle::State_MouseOver && option->state & QStyle::State_Sunken) ? opt.state | QStyle::State_Sunken : opt.state &= ~QStyle::State_Sunken;
                drawControl(QStyle::CE_ScrollBarSlider, &opt, painter, widget);
            }
        }

        painter->setClipping(true);

        return;
    }
    case QStyle::CC_SpinBox:
    {
        if(!(option->state & QStyle::State_Enabled))
            FieldOutlineDisabled.DrawAt(painter, option->rect, 0, 0, 0);
        else FieldOutline.DrawAt(painter, option->rect, 0, 0, 0);

        const QStyleOptionSpinBox* opt_s = qstyleoption_cast<const QStyleOptionSpinBox*>(option);
        if(!opt_s) break;
        QRect rec_up = subControlRect(control, option, QStyle::SC_SpinBoxUp, widget);
        rec_up.setTop(option->rect.top()+1);
        rec_up.setLeft(rec_up.left()-1);
        rec_up.setBottom(rec_up.bottom()+1);
        rec_up.setRight(option->rect.right());
        QRect rec_down = subControlRect(control, option, QStyle::SC_SpinBoxDown, widget);
        rec_down.setLeft(rec_down.left()-1);
        rec_down.setBottom(option->rect.bottom());
        rec_down.setRight(option->rect.right());
        if(!(opt_s->stepEnabled & QAbstractSpinBox::StepUpEnabled) || !(option->state & QStyle::State_Enabled))
            SpinBoxUp.DrawAt(painter, rec_up, 0, 3, 0);
        else if(option->activeSubControls == QStyle::SC_SpinBoxUp)
        {
            if(option->state & QStyle::State_Sunken)
                SpinBoxUp.DrawAt(painter, rec_up, 0, 2, 0);
            else SpinBoxUp.DrawAt(painter, rec_up, 0, 1, 0);
        }
        else SpinBoxUp.DrawAt(painter, rec_up, 0, 0, 0);
        if(!(opt_s->stepEnabled & QAbstractSpinBox::StepDownEnabled) || !(option->state & QStyle::State_Enabled))
            SpinBoxDown.DrawAt(painter, rec_down, 0, 3, 0);
        else if(option->activeSubControls == QStyle::SC_SpinBoxDown)
        {
            if(option->state & QStyle::State_Sunken)
                SpinBoxDown.DrawAt(painter, rec_down, 0, 2, 0);
            else SpinBoxDown.DrawAt(painter, rec_down, 0, 1, 0);
        }
        else SpinBoxDown.DrawAt(painter, rec_down, 0, 0, 0);
        return;
    }
    case QStyle::CC_Slider:
    {
        //SC_SliderGroove
        const QStyleOptionSlider* opt_s = qstyleoption_cast<const QStyleOptionSlider*>(option);
        if(!opt_s) break;

        QRect rec_groove = subControlRect(control, option, QStyle::SC_SliderGroove, widget);
        QRect rec_thumb = subControlRect(control, option, QStyle::SC_SliderHandle, widget);

        // ok now, fix the groove so it's only 2-3 pixels wide
        if(option->state & QStyle::State_Horizontal)
        {
            if(rec_groove.height() > 3)
            {
                rec_groove.setY(rec_groove.y()+rec_groove.height()/2-1);
                rec_groove.setHeight(3);
            }
        }
        else
        {
            if(rec_groove.width() > 3)
            {
                rec_groove.setX(rec_groove.x()+rec_groove.width()/2-1);
                rec_groove.setWidth(3);
            }
        }

        SliderTrack.DrawAt(painter, rec_groove, 0, 0, 0);

        if(option->state & QStyle::State_Horizontal)
        {
            if(!(option->state & QStyle::State_Enabled))
                SliderThumbH.DrawAt(painter, rec_thumb, 0, 4, 0);
            else if(option->activeSubControls == QStyle::SC_SliderHandle)
            {
                if(option->state & QStyle::State_Sunken)
                    SliderThumbH.DrawAt(painter, rec_thumb, 0, 2, 0);
                else SliderThumbH.DrawAt(painter, rec_thumb, 0, 1, 0);
            }
            else SliderThumbH.DrawAt(painter, rec_thumb, 0, 0, 0);
        }
        else
        {
            if(!(option->state & QStyle::State_Enabled))
                SliderThumbV.DrawAt(painter, rec_thumb, 0, 4, 0);
            else if(option->activeSubControls == QStyle::SC_SliderHandle)
            {
                if(option->state & QStyle::State_Sunken)
                    SliderThumbV.DrawAt(painter, rec_thumb, 0, 2, 0);
                else SliderThumbV.DrawAt(painter, rec_thumb, 0, 1, 0);
            }
            else SliderThumbV.DrawAt(painter, rec_thumb, 0, 0, 0);
        }

        return;
    }
    case QStyle::CC_ComboBox:
    {
        const QStyleOptionComboBox* opt_c = qstyleoption_cast<const QStyleOptionComboBox*>(option);
        if(!opt_c) break;

        if(opt_c->frame)
        {
            if(!(option->state & QStyle::State_Enabled))
                FieldOutlineDisabled.DrawAt(painter, option->rect, 0, 0, 0);
            else FieldOutline.DrawAt(painter, option->rect, 0, 0, 0);

            // also draw the blue selection if this field is focused.
            if (opt_c->state & QStyle::State_HasFocus)
            {
                QRect rec_sel = subControlRect(control, option, QStyle::SC_ComboBoxEditField, widget);
                painter->fillRect(rec_sel, SysMetrics.Highlight);
            }
        }

        QRect rec_button = subControlRect(control, option, QStyle::SC_ComboBoxArrow, widget);
        rec_button.setRight(option->rect.right());
        rec_button.setTop(option->rect.top()+1);
        rec_button.setBottom(option->rect.bottom());

        if(!(option->state & QStyle::State_Enabled))
            ComboBoxDropdown.DrawAt(painter, rec_button, 0, 3, 0);
        else if(option->state & QStyle::State_On)
            ComboBoxDropdown.DrawAt(painter, rec_button, 0, 2, 0);
        else if(option->activeSubControls & (QStyle::SC_ComboBoxArrow|QStyle::SC_ComboBoxFrame|(!opt_c->editable ? QStyle::SC_ComboBoxEditField : 0)))
            ComboBoxDropdown.DrawAt(painter, rec_button, 0, 1, 0);
        else ComboBoxDropdown.DrawAt(painter, rec_button, 0, 0, 0);
        return;
    }
    default:
        break;
    }

    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

void LunaticStyle::polish(QPalette& p)
{
    QProxyStyle::polish(p);

    if(!Active)
        return;

    p.setColor(QPalette::Window, SysMetrics.BtnFace);
    p.setColor(QPalette::Highlight, SysMetrics.Highlight);
    p.setColor(QPalette::HighlightedText, SysMetrics.HighlightText);
    p.setColor(QPalette::Dark, SysMetrics.DkShadow3D);
    p.setColor(QPalette::Light, SysMetrics.Light3D);
    p.setColor(QPalette::Shadow, SysMetrics.DkShadow3D);
    p.setColor(QPalette::Button, SysMetrics.Menu); // I don't want to rewrite menu item styling (it's the same anyway...)
    p.setColor(QPalette::ButtonText, Button.TextColor);
    p.setColor(QPalette::Base, SysMetrics.Window);
    p.setColor(QPalette::Foreground, Button.TextColor);
    p.setColor(QPalette::WindowText, Button.TextColor);
    p.setColor(QPalette::Text, Button.TextColor);
    p.setColor(QPalette::ToolTipBase, QColor(255, 255, 240));
    p.setColor(QPalette::ToolTipText, QColor(0,0,0));

    // set disabled text field color
    p.setColor(QPalette::Disabled, QPalette::Base, FieldOutlineDisabled.FillColor);

    // we don't set anything else here for now
}
