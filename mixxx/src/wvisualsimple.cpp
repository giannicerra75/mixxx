/***************************************************************************
                          wvisualsimple.cpp  -  description
                             -------------------
    begin                : Thu Oct 9 2003
    copyright            : (C) 2002 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "wvisualsimple.h"
#include "wskincolor.h"
#include <qpainter.h>
#include <qpixmap.h>
#include <qdragobject.h>

WVisualSimple::WVisualSimple(QWidget *pParent, const char *pName) : WWidget(pParent, pName)
{
    setAcceptDrops(true);
    m_iValue = 64;
}

WVisualSimple::~WVisualSimple()
{
}

void WVisualSimple::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept(QUriDrag::canDecode(event));
}

void WVisualSimple::dropEvent(QDropEvent *event)
{
    QStringList lst;
    if (!QUriDrag::canDecode(event))
    {
        event->ignore();
        return;
    }

    event->accept();
    QUriDrag::decodeLocalFiles(event, lst);
    QString name = (*lst.begin());

    emit(trackDropped(name));
}

void WVisualSimple::setup(QDomNode node)
{
    // Setup position and connections
    WWidget::setup(node);

    // Size
    QString size = selectNodeQString(node, "Size");
    int x = size.left(size.find(",")).toInt();
    int y = size.mid(size.find(",")+1).toInt();
    setFixedSize(x,y);

    // Set constants for line drawing
    m_qMarkerPos1.setX(x/2);
    m_qMarkerPos1.setY(0);
    m_qMarkerPos2.setX(x/2);
    m_qMarkerPos2.setY(y);
    m_qMousePos.setX(x/2);
    m_qMousePos.setY(y/2);
    
    // Background color
	QColor c(255,255,255);
    if (!selectNode(node, "BgColor").isNull())
	{
        c.setNamedColor(selectNodeQString(node, "BgColor"));
    }
	setBackgroundColor(WSkinColor::getCorrectColor(c));

    colorSignal.setNamedColor(selectNodeQString(node, "SignalColor"));
	colorSignal = WSkinColor::getCorrectColor(colorSignal);
    colorMarker.setNamedColor(selectNodeQString(node, "MarkerColor"));
	colorMarker = WSkinColor::getCorrectColor(colorMarker);
}

void WVisualSimple::slotNewTrack()
{
}

void WVisualSimple::mouseMoveEvent(QMouseEvent *e)
{
    // Only process mouse move if it was initiated by a left click
    if (m_iStartPosX!=-1)
    {
        double v = 64.+(double)(e->x()-m_iStartPosX)/10.;
        if (v<0.)
            v = 0.;
        else if (v>127.)
            v = 127.;
        emit(valueChangedLeftDown(v));
        m_iValue = (int)v;
    }
    update();
}

void WVisualSimple::mousePressEvent(QMouseEvent *e)
{
    m_iStartPosX = -1;
    if (e->button()==Qt::LeftButton)
    {
        // Store current x position of mouse pointer
        m_iStartPosX = e->x();
        emit(valueChangedLeftDown(64.));
    }
}

void WVisualSimple::mouseReleaseEvent(QMouseEvent *)
{
    m_iValue = 64;
    emit(valueChangedLeftDown((double)m_iValue));
    update();
}   

void WVisualSimple::paintEvent(QPaintEvent *)
{
    QPixmap pm(size());
    QPainter paint;
    paint.begin(&pm, this);
    
    // Erase background
    paint.eraseRect(rect());
    
    // Draw vertical red bar in center
    paint.setPen(colorMarker);
    paint.drawLine(m_qMarkerPos1, m_qMarkerPos2);

    // Draw lines indicating position
    if (m_iValue!=64)
    {
        paint.setPen(colorSignal);
        QPoint p1 = m_qMousePos+QPoint((m_iValue-64)*2,0);
        QPoint p2 = m_qMousePos+QPoint((m_iValue-64)*2,0);    
        paint.drawLine(m_qMarkerPos1, p1);
        paint.drawLine(m_qMarkerPos2, p2);
    }

    paint.end();
    bitBlt(this, 0, 0, &pm);
}
