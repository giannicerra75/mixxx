/***************************************************************************
                          mixxxview.cpp  -  description
                             -------------------
    begin                : Mon Feb 18 09:48:17 CET 2002
    copyright            : (C) 2002 by Tue and Ken .Haste Andersen
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mixxxview.h"

#include <qtable.h>
#include <qdir.h>
#include <qpixmap.h>
#include <qtooltip.h>
#include <qevent.h>
#include <qsplitter.h>
#include <qmenubar.h>
#include <qmainwindow.h>

#include "wtracktable.h"
#include "wtreeview.h"
#include "wwidget.h"
#include "wknob.h"
#include "wpushbutton.h"
#include "wslider.h"
#include "wslidercomposed.h"
#include "wdisplay.h"
#include "wvumeter.h"
#include "wlabel.h"
#include "wnumber.h"
#include "wnumberpos.h"
#include "wnumberbpm.h"
#include "wnumberrate.h"
#include "wpixmapstore.h"
#include "wvisualwaveform.h"
#include "wvisualsimple.h"
#include "woverview.h"
#include "mixxxkeyboard.h"
#include "controlobject.h"
#include "controlobjectthreadwidget.h"

#include "imgloader.h"
#include "imginvert.h"
#include "imgcolor.h"
#include "wskincolor.h"

MixxxView::MixxxView(QWidget *parent, ConfigObject<ConfigValueKbd> *kbdconfig, bool bVisualsWaveform, QString qSkinPath, ConfigObject<ConfigValue> *pConfig) : QWidget(parent, "Mixxx")
{
    // Path to image files
    WWidget::setPixmapPath(qSkinPath.append("/"));

    m_qWidgetList.setAutoDelete(true);

    m_pKeyboard = new MixxxKeyboard(kbdconfig);
    installEventFilter(m_pKeyboard);

    //qDebug("skin %s",qSkinPath.latin1());

    // Read XML file
    QDomDocument skin("skin");
    QFile file(WWidget::getPath("skin.xml"));
    if (!file.open(IO_ReadOnly))
    {
        qFatal("Could not open skin definition file: %s",file.name().latin1());
    }
    if (!skin.setContent(&file))
    {
        qFatal("Error parsing skin definition file: %s",file.name().latin1());
    }

	file.close();
    QDomElement docElem = skin.documentElement();

#ifdef __WIN__
#ifndef QT3_SUPPORT
    // QPixmap fix needed on Windows 9x
    QPixmap::setDefaultOptimization(QPixmap::MemoryOptim);
#endif
#endif

    // Default values for visuals
    m_pTrackTable = 0;
    m_pTreeView = 0;
    m_pTextCh1 = 0;
    m_pTextCh2 = 0;
    m_pVisualCh1 = 0;
    m_pVisualCh2 = 0;
    m_pNumberPosCh1 = 0;
    m_pNumberPosCh2 = 0;
    m_pSliderRateCh1 = 0;
    m_pSliderRateCh2 = 0;
    m_bZoom = false;
    m_pSplitter = 0;
    m_bVisualWaveform = false;
    m_pOverviewCh1 = 0;
    m_pOverviewCh2 = 0;

	QDomNode colsch = docElem.namedItem("Schemes");
	if (!colsch.isNull() && colsch.isElement()) {
		QString schname = pConfig->getValueString(ConfigKey("[Config]","Scheme"));
		QDomNode sch = colsch.firstChild();

		bool found = false;
		while (!sch.isNull() && !found) {
			QString thisname = WWidget::selectNodeQString(sch, "Name");
			if (thisname == schname) {
				found = true;
			} else {
				sch = sch.nextSibling();
			}
		}

		if (found) {
			ImgSource* imsrc = parseFilters(sch.namedItem("Filters"));
			WPixmapStore::setLoader(imsrc);
			WSkinColor::setLoader(imsrc);
		}
	}

    // Load all widgets defined in the XML file
    QDomNode node = docElem.firstChild();
    while (!node.isNull())
    {
        if (node.isElement())
        {
	//printf("%s\n", node.nodeName());
            if (node.nodeName()=="PushButton")
            {
                WPushButton *p = new WPushButton(this);
                p->setup(node);
                p->installEventFilter(m_pKeyboard);
                m_qWidgetList.append(p);
            }
            else if (node.nodeName()=="Knob")
            {
                WKnob *p = new WKnob(this);
                p->setup(node);
                p->installEventFilter(m_pKeyboard);
                m_qWidgetList.append(p);
            }
	    else if (node.nodeName()=="Label")
            {
                WLabel *p = new WLabel(this);
                p->setup(node);

                m_qWidgetList.append(p);
            }
            else if (node.nodeName()=="Number")
            {
                WNumber *p = new WNumber(this);
                p->setup(node);
                p->installEventFilter(m_pKeyboard);
                m_qWidgetList.append(p);
            }
            else if (node.nodeName()=="NumberBpm")
            {
                if (WWidget::selectNodeInt(node, "Channel")==1)
                {
                    WNumberBpm *p = new WNumberBpm("[Channel1]", this);
                    p->setup(node);
                    p->installEventFilter(m_pKeyboard);
                    m_qWidgetList.append(p);
                }
                else if (WWidget::selectNodeInt(node, "Channel")==2)
                {
                    WNumberBpm *p = new WNumberBpm("[Channel2]", this);
                    p->setup(node);
                    p->installEventFilter(m_pKeyboard);
                    m_qWidgetList.append(p);
                }
            }
            else if (node.nodeName()=="NumberPos")
            {
                if (WWidget::selectNodeInt(node, "Channel")==1 && m_pNumberPosCh1==0)
                {
                    m_pNumberPosCh1 = new WNumberPos("[Channel1]", this);
                    m_pNumberPosCh1->setup(node);
                    m_pNumberPosCh1->installEventFilter(m_pKeyboard);
                    m_qWidgetList.append(m_pNumberPosCh1);
                }
                else if (WWidget::selectNodeInt(node, "Channel")==2 && m_pNumberPosCh2==0)
                {
                    m_pNumberPosCh2 = new WNumberPos("[Channel2]", this);
                    m_pNumberPosCh2->setup(node);
                    m_pNumberPosCh2->installEventFilter(m_pKeyboard);
                    m_qWidgetList.append(m_pNumberPosCh2);
                }
            }
            else if (node.nodeName()=="NumberRate")
            {
                if (WWidget::selectNodeInt(node, "Channel")==1)
                {
                    WNumberRate *p = new WNumberRate("[Channel1]", this);
                    p->setup(node);
                    p->installEventFilter(m_pKeyboard);
                    m_qWidgetList.append(p);
                }
                else if (WWidget::selectNodeInt(node, "Channel")==2)
                {
                    WNumberRate *p = new WNumberRate("[Channel2]", this);
                    p->setup(node);
                    p->installEventFilter(m_pKeyboard);
                    m_qWidgetList.append(p);
                }
            }
            else if (node.nodeName()=="Display")
            {
                WDisplay *p = new WDisplay(this);
                p->setup(node);
                p->installEventFilter(m_pKeyboard);
                m_qWidgetList.append(p);
            }
            else if (node.nodeName()=="Background")
            {
                QString filename = WWidget::selectNodeQString(node, "Path");
				QPixmap* background = WPixmapStore::getPixmap(WWidget::getPath(filename));
                this->setPaletteBackgroundPixmap(*background);
				// TODO: Delete background from pixmap store to save memory?
                // FWI: Begin of fullscreen patch
                // this->setFixedSize(background.width(),background.height()+((QMainWindow *)parent)->menuBar()->height());
                // parent->setFixedSize(background.width(),background.height()+((QMainWindow *)parent)->menuBar()->height());
                this->setFixedSize(background->width(),background->height());
                parent->setMinimumSize(background->width(),background->height());
                // FWI: End of fullscreen patch
                this->move(0,0);
				
				QColor c(255,255,255);
				if (!WWidget::selectNode(node, "BgColor").isNull()) {
					c.setNamedColor(WWidget::selectNodeQString(node, "BgColor"));
				}
				parent->setEraseColor(WSkinColor::getCorrectColor(c));
            }
            else if (node.nodeName()=="SliderComposed")
            {
                WSliderComposed *p = new WSliderComposed(this);
                p->setup(node);
                p->installEventFilter(m_pKeyboard);
                m_qWidgetList.append(p);

                // If rate slider...
                if (compareConfigKeys(node, "[Channel1],rate"))
                    m_pSliderRateCh1 = p;
                else if (compareConfigKeys(node, "[Channel2],rate"))
                    m_pSliderRateCh2 = p;
            }
            else if (node.nodeName()=="VuMeter")
            {
                WVuMeter *p = new WVuMeter(this);
                m_qWidgetList.append(p);
                p->setup(node);
                p->installEventFilter(m_pKeyboard);
            }
            else if (node.nodeName()=="Overview")
            {
                if (WWidget::selectNodeInt(node, "Channel")==1 && m_pOverviewCh1==0)
                {
                    m_pOverviewCh1 = new WOverview(this);
                    m_qWidgetList.append(m_pOverviewCh1);
                    m_pOverviewCh1->setup(node);
                }
                else if (WWidget::selectNodeInt(node, "Channel")==2 && m_pOverviewCh2==0)
                {
                    m_pOverviewCh2 = new WOverview(this);
                    m_qWidgetList.append(m_pOverviewCh2);
                    m_pOverviewCh2->setup(node);
                }
            }
            else if (node.nodeName()=="Visual")
            {
                if (WWidget::selectNodeInt(node, "Channel")==1 && m_pVisualCh1==0)
                {
                    if (bVisualsWaveform)
                    {
                        m_pVisualCh1 = new WVisualWaveform(this, 0, (QGLWidget *)m_pVisualCh2);
                        if (((WVisualWaveform *)m_pVisualCh1)->isValid())
                        {
                            ((WVisualWaveform *)m_pVisualCh1)->setup(node);
                            m_pVisualCh1->installEventFilter(m_pKeyboard);
                            m_qWidgetList.append(m_pVisualCh1);
                            m_bVisualWaveform = true;
                        }
                        else
                        {
                            m_bVisualWaveform = false;
                            delete m_pVisualCh1;
                        }
                    }
                    if (!m_bVisualWaveform)
                    {
                        m_pVisualCh1 = new WVisualSimple(this, 0);
                        ((WVisualSimple *)m_pVisualCh1)->setup(node);
                        m_pVisualCh1->installEventFilter(m_pKeyboard);
                        m_qWidgetList.append(m_pVisualCh1);
                    }
                    ControlObjectThreadWidget *p = new ControlObjectThreadWidget(ControlObject::getControl(ConfigKey("[Channel1]", "wheel")));
                    p->setWidget((QWidget *)m_pVisualCh1, true, Qt::LeftButton);
                    //ControlObject::setWidget((QWidget *)m_pVisualCh1, ConfigKey("[Channel1]", "wheel"), true, Qt::LeftButton);
                }
                else if (WWidget::selectNodeInt(node, "Channel")==2 && m_pVisualCh2==0)
                {
                    if (bVisualsWaveform)
                    {
                        m_pVisualCh2 = new WVisualWaveform(this, "", (QGLWidget *)m_pVisualCh1);
                        if (((WVisualWaveform *)m_pVisualCh2)->isValid())
                        {
                            ((WVisualWaveform *)m_pVisualCh2)->setup(node);
                            m_pVisualCh2->installEventFilter(m_pKeyboard);
                            m_bVisualWaveform = true;
                            m_qWidgetList.append(m_pVisualCh2);
                        }
                        else
                        {
                            m_bVisualWaveform = false;
                            delete m_pVisualCh2;
                        }
                    }
                    if (!m_bVisualWaveform)
                    {
                        m_pVisualCh2 = new WVisualSimple(this, 0);
                        ((WVisualSimple *)m_pVisualCh2)->setup(node);
                        m_pVisualCh2->installEventFilter(m_pKeyboard);
                        m_qWidgetList.append(m_pVisualCh2);
                    }
                    ControlObjectThreadWidget *p = new ControlObjectThreadWidget(ControlObject::getControl(ConfigKey("[Channel2]", "wheel")));
                    p->setWidget((QWidget *)m_pVisualCh2, true, Qt::LeftButton);
                    //ControlObject::setWidget((QWidget *)m_pVisualCh2, ConfigKey("[Channel2]", "wheel"), true, Qt::LeftButton);
                }

                if (!WWidget::selectNode(node, "Zoom").isNull() && WWidget::selectNodeQString(node, "Zoom")=="true")
                    m_bZoom = true;
            }
            else if (node.nodeName()=="Text")
            {
                QLabel *p = new QLabel(this);
                p->installEventFilter(m_pKeyboard);

                m_qWidgetList.append(p);

                // Set position
                QString pos = WWidget::selectNodeQString(node, "Pos");
                int x = pos.left(pos.find(",")).toInt();
                int y = pos.mid(pos.find(",")+1).toInt();
                p->move(x,y);

                // Size
                QString size = WWidget::selectNodeQString(node, "Size");
                x = size.left(size.find(",")).toInt();
                y = size.mid(size.find(",")+1).toInt();
                p->setFixedSize(x,y);

                // Background color
				QColor bgc(255,255,255);
				if (!WWidget::selectNode(node, "BgColor").isNull()) {
                    bgc.setNamedColor(WWidget::selectNodeQString(node, "BgColor"));
                }
				p->setPaletteBackgroundColor(WSkinColor::getCorrectColor(bgc));

                // Foreground color
				QColor fgc(0,0,0);
				if (!WWidget::selectNode(node, "FgColor").isNull()) {
                    fgc.setNamedColor(WWidget::selectNodeQString(node, "FgColor"));
                }
				p->setPaletteForegroundColor(WSkinColor::getCorrectColor(fgc));

                // Alignment
                if (!WWidget::selectNode(node, "Align").isNull() && WWidget::selectNodeQString(node, "Align")=="right")
                    p->setAlignment(Qt::AlignRight);

                // Associate pointers
                if (WWidget::selectNodeInt(node, "Channel")==1)
                    m_pTextCh1 = p;
                else if (WWidget::selectNodeInt(node, "Channel")==2)
                    m_pTextCh2 = p;

            }
            else if (node.nodeName()=="Splitter")
            {
                m_pSplitter = new QSplitter(this);
                m_pSplitter->installEventFilter(m_pKeyboard);

                // Set position
                QString pos = WWidget::selectNodeQString(node, "Pos");
                int x = pos.left(pos.find(",")).toInt();
                int y = pos.mid(pos.find(",")+1).toInt();
                m_pSplitter->move(x,y);

                // Size
                QString size = WWidget::selectNodeQString(node, "Size");
                x = size.left(size.find(",")).toInt();
                y = size.mid(size.find(",")+1).toInt();
                m_pSplitter->setFixedSize(x,y);

                // This is QT 3.2 only
                //m_pSplitter->setHandleWidth(2);

                m_qWidgetList.append(m_pSplitter);
            }
            else if (node.nodeName()=="TrackTable")
            {
                if (m_pSplitter)
                {
                    m_pTrackTable = new WTrackTable(m_pSplitter);
                    m_pSplitter->setResizeMode(m_pTrackTable, QSplitter::Stretch);
                }
                else
                {
                    m_pTrackTable = new WTrackTable(this);
                    m_qWidgetList.append(m_pTrackTable);
                }
                m_pTrackTable->setup(node);
                m_pTrackTable->installEventFilter(m_pKeyboard);
            }
            else if (node.nodeName()=="TreeView")
            {
                if (m_pSplitter)
                {
                    m_pTreeView = new WTreeView(pConfig->getValueString(ConfigKey("[Playlist]","Directory")), m_pSplitter, tr("TreeView"));
                    m_pSplitter->setResizeMode(m_pTreeView, QSplitter::Stretch);
                }
                else
                {
                    m_pTreeView = new WTreeView(pConfig->getValueString(ConfigKey("[Playlist]","Directory")), this, tr("TreeView"));
                    m_qWidgetList.append(m_pTreeView);
                }
                m_pTreeView->setup(node);
                m_pTreeView->installEventFilter(m_pKeyboard);
            }

        }
        node = node.nextSibling();
    }

#ifdef __WIN__
#ifndef QT3_SUPPORT
    // QPixmap fix needed on Windows 9x
    QPixmap::setDefaultOptimization(QPixmap::NormalOptim);
#endif
#endif

}

MixxxView::~MixxxView()
{
    m_qWidgetList.clear();
}

void MixxxView::checkDirectRendering()
{
    // Check if DirectRendering is enabled and display warning
    if ((m_pVisualCh1 && !((WVisualWaveform *)m_pVisualCh1)->directRendering()) ||
        (m_pVisualCh2 && !((WVisualWaveform *)m_pVisualCh2)->directRendering()))
        QMessageBox::warning(0, "OpenGL Direct Rendering",
                                "Direct redering is not enabled on your machine.\n\nThis means that the waveform displays will be very\nslow and take a lot of CPU time. Either update your\nconfiguration to enable direct rendering, or disable\nthe waveform displays in the control panel by\nselecting \"Simple\" under waveform displays.\nNOTE: In case you run on NVidia hardware,\ndirect rendering may not be present, but you will\nnot experience a degradation in performance.");
}

bool MixxxView::activeWaveform()
{
    return m_bVisualWaveform;
}

bool MixxxView::compareConfigKeys(QDomNode node, QString key)
{
    QDomNode n = node;

    // Loop over each <Connection>, check if it's ConfigKey matches key
    while (!n.isNull())
    {
        n = WWidget::selectNode(n, "Connection");
        if (!n.isNull())
        {
            if  (WWidget::selectNodeQString(n, "ConfigKey").contains(key))
                return true;
        }
    }
    return false;
}

ImgSource* MixxxView::parseFilters(QDomNode filt) {
	if (!filt.hasChildNodes()) {
		return 0;
	}

	ImgSource* ret = new ImgLoader();

	QDomNode f = filt.firstChild();

	while (!f.isNull()) {
		QString name = f.nodeName().lower();
		if (name == "invert") {
			ret = new ImgInvert(ret);
		} else if (name == "hueinv") {
			ret = new ImgHueInv(ret);
		} else if (name == "add") {
			ret = new ImgAdd(ret, WWidget::selectNodeInt(f, "Amount"));
		} else if (name == "scalewhite") {
			ret = new ImgScaleWhite(ret, WWidget::selectNodeFloat(f, "Amount"));
		} else {
			qDebug("Unkown image filter: %s\n", name);
		}
		f = f.nextSibling();
	}

	return ret;
}