/******************************************************************************
 * $Id: instrument.cpp, v1.0 2010/08/30 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  Dashboard Plugin
 * Author:   Jean-Eudes Onfray
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif  // precompiled headers
#include <cmath>

#include "instrument.h"
#include "wx28compat.h"

extern int g_iSDMMFormat;

#ifdef __OCPN__ANDROID__
#include "qdebug.h"
#endif

//----------------------------------------------------------------
//
//    Generic DashboardInstrument Implementation
//
//----------------------------------------------------------------

DashboardInstrument::DashboardInstrument(wxWindow* pparent, wxWindowID id,
                                         wxString title, DASH_CAP cap_flag)
    : wxControl(pparent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE) {
  m_title = title;
  m_cap_flag.set(cap_flag);

  SetBackgroundStyle(wxBG_STYLE_CUSTOM);
  SetDrawSoloInPane(false);
  wxClientDC dc(this);
  int width;
  dc.GetTextExtent(m_title, &width, &m_TitleHeight, 0, 0, g_pFontTitle);

  Connect(wxEVT_ERASE_BACKGROUND,
          wxEraseEventHandler(DashboardInstrument::OnEraseBackground));
  Connect(wxEVT_PAINT, wxPaintEventHandler(DashboardInstrument::OnPaint));

  //  On OSX, there is an orphan mouse event that comes from the automatic
  //  exEVT_CONTEXT_MENU synthesis on the main wxWindow mouse handler.
  //  The event goes to an instrument window (here) that may have been deleted
  //  by the preferences dialog.  Result is NULL deref. Solution:  Handle
  //  right-click here, and DO NOT skip() Strangely, this does not work for
  //  GTK... See: http://trac.wxwidgets.org/ticket/15417

#if defined(__WXOSX__) || defined(__WXQT__)
  Connect(wxEVT_RIGHT_DOWN,
          wxMouseEventHandler(DashboardInstrument::MouseEvent), NULL, this);
#endif
}

void DashboardInstrument::MouseEvent(wxMouseEvent& event) {
  if (event.GetEventType() == wxEVT_RIGHT_DOWN) {
    wxContextMenuEvent evtCtx(wxEVT_CONTEXT_MENU, this->GetId(),
                              this->ClientToScreen(event.GetPosition()));
    evtCtx.SetEventObject(this);
    GetParent()->GetEventHandler()->AddPendingEvent(evtCtx);
  }
}

CapType DashboardInstrument::GetCapacity() { return m_cap_flag; }
void DashboardInstrument::SetDrawSoloInPane(bool value) {
  m_drawSoloInPane = value;
}
void DashboardInstrument::OnEraseBackground(wxEraseEvent& WXUNUSED(evt)) {
  // intentionally empty
}

void DashboardInstrument::OnPaint(wxPaintEvent& WXUNUSED(event)) {
  wxAutoBufferedPaintDC pdc(this);
  if (!pdc.IsOk()) {
    wxLogMessage(
        _T("DashboardInstrument::OnPaint() fatal: ")
        _T("wxAutoBufferedPaintDC.IsOk() false."));
    return;
  }

  wxSize size = GetClientSize();
  if (size.x == 0 || size.y == 0) {
    wxLogMessage(_T("DashboardInstrument::OnPaint() fatal: Zero size DC."));
    return;
  }

#if wxUSE_GRAPHICS_CONTEXT
  wxGCDC dc(pdc);
#else
  wxDC &dc(pdc);
#endif

  wxColour cl;
  GetGlobalColor(_T("DASHB"), &cl);
  dc.SetBackground(cl);
#ifdef __WXGTK__
  dc.SetBrush(cl);
  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.DrawRectangle(0, 0, size.x, size.y);
#endif
  dc.Clear();

  Draw(&dc);

  if (!m_drawSoloInPane) {
    wxPen pen;
    pen.SetStyle(wxPENSTYLE_SOLID);
    GetGlobalColor(_T("DASHL"), &cl);
    pen.SetColour(cl);
    dc.SetPen(pen);
    dc.SetBrush(cl);
    dc.DrawRoundedRectangle(0, 0, size.x, m_TitleHeight, 3);

    dc.SetFont(*g_pFontTitle);
    GetGlobalColor(_T("DASHF"), &cl);
    dc.SetTextForeground(cl);
    dc.DrawText(m_title, 5, 0);
  }
}

//----------------------------------------------------------------
//
//    DashboardInstrument_Simple Implementation
//
//----------------------------------------------------------------

DashboardInstrument_Single::DashboardInstrument_Single(wxWindow* pparent,
                                                       wxWindowID id,
                                                       wxString title,
                                                       DASH_CAP cap_flag,
                                                       wxString format)
    : DashboardInstrument(pparent, id, title, cap_flag) {
  m_format = format;
  m_data = _T("---");
  m_DataHeight = 0;
}

wxSize DashboardInstrument_Single::GetSize(int orient, wxSize hint) {
  wxClientDC dc(this);
  int w;
  dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
  dc.GetTextExtent(_T("000"), &w, &m_DataHeight, 0, 0, g_pFontData);

  if (orient == wxHORIZONTAL) {
    return wxSize(DefaultWidth, wxMax(hint.y, m_TitleHeight + m_DataHeight));
  } else {
    return wxSize(wxMax(hint.x, DefaultWidth), m_TitleHeight + m_DataHeight);
  }
}

void DashboardInstrument_Single::Draw(wxGCDC* dc) {
  wxColour cl;
  dc->SetFont(*g_pFontData);
  GetGlobalColor(_T("DASHF"), &cl);
  dc->SetTextForeground(cl);

  dc->DrawText(m_data, 10, m_TitleHeight);
}

void DashboardInstrument_Single::SetData(DASH_CAP st, double data,
                                         wxString unit) {
  if (m_cap_flag.test(st)) {
    if (!std::isnan(data)) {
      if (unit == _T("C"))
        m_data = wxString::Format(m_format, data) + DEGREE_SIGN + _T("C");
      else if (unit == _T("\u00B0"))
        m_data = wxString::Format(m_format, data) + DEGREE_SIGN;
      else if (unit == _T("\u00B0T"))
        m_data = wxString::Format(m_format, data) + DEGREE_SIGN + _(" true");
      else if (unit == _T("\u00B0M"))
        m_data = wxString::Format(m_format, data) + DEGREE_SIGN + _(" mag");
      else if (unit == _T("\u00B0L"))
        m_data = _T(">") + wxString::Format(m_format, data) + DEGREE_SIGN;
      else if (unit == _T("\u00B0R"))
        m_data = wxString::Format(m_format, data) + DEGREE_SIGN + _T("<");
      else if (unit == _T("N"))  // Knots
        m_data = wxString::Format(m_format, data) + _T(" Kts");
      /* maybe in the future ...
                      else if (unit == _T("M")) // m/s
                        m_data = wxString::Format(m_format, data)+_T(" m/s");
                      else if (unit == _T("K")) // km/h
                        m_data = wxString::Format(m_format, data)+_T(" km/h");
       ... to be completed
       */
      else
        m_data = wxString::Format(m_format, data) + _T(" ") + unit;
    } else
      m_data = _T("---");

    Refresh();
  }
}

//----------------------------------------------------------------
//
//    DashboardInstrument_Position Implementation
//
//----------------------------------------------------------------

DashboardInstrument_Position::DashboardInstrument_Position(wxWindow* pparent,
                                                           wxWindowID id,
                                                           wxString title,
                                                           DASH_CAP cap_flag1,
                                                           DASH_CAP cap_flag2)
    : DashboardInstrument(pparent, id, title, cap_flag1) {
  m_cap_flag.set(cap_flag2);

  m_data1 = _T("---");
  m_data2 = _T("---");
  m_cap_flag1 = cap_flag1;
  m_cap_flag2 = cap_flag2;
  m_DataHeight = 0;
}

wxSize DashboardInstrument_Position::GetSize(int orient, wxSize hint) {
  wxClientDC dc(this);
  int w;
  dc.GetTextExtent(m_title, &w, &m_TitleHeight, 0, 0, g_pFontTitle);
  dc.GetTextExtent(_T("000  00.0000 W"), &w, &m_DataHeight, 0, 0, g_pFontData);

  if (orient == wxHORIZONTAL) {
    return wxSize(w + 10, wxMax(hint.y, m_TitleHeight + m_DataHeight * 2));
  } else {
    return wxSize(wxMax(hint.x, w + 10), m_TitleHeight + m_DataHeight * 2);
  }
}

void DashboardInstrument_Position::Draw(wxGCDC* dc) {
  wxColour cl;
  dc->SetFont(*g_pFontData);
  GetGlobalColor(_T("DASHF"), &cl);
  dc->SetTextForeground(cl);

  dc->DrawText(m_data1, 10, m_TitleHeight);
  dc->DrawText(m_data2, 10, m_TitleHeight + m_DataHeight);
}

void DashboardInstrument_Position::SetData(DASH_CAP st, double data,
                                           wxString unit) {
  if (std::isnan(data)) return;
  if (st == m_cap_flag1) {
    m_data1 = toSDMM(1, data);
    m_data1[0] = ' ';
  } else if (st == m_cap_flag2) {
    m_data2 = toSDMM(2, data);
  } else
    return;
  Refresh();
}
/***************************************************************************************/
/*                     SD.DDDD -->   SDMM DD MM.MMM   --->   DD MM SS.SSSS             */
/***************************************************************************************/


wxString toSDMM(int NEflag, double a, bool hi_precision) {
  wxString s;
  double mpy;
  short neg = 0;
  int d;
  long m;
  double ang = a;
  char c = 'N';

  if (a < 0.0) {
    a = -a;
    neg = 1;
  }
  d = (int)a;
  if (neg) d = -d;
  if (NEflag) {
    if (NEflag == 1) {
      c = 'N';

      if (neg) {
        d = -d;
        c = 'S';
      }
    } else if (NEflag == 2) {
      c = 'E';

      if (neg) {
        d = -d;
        c = 'W';
      }
    }
  }

  switch (g_iSDMMFormat) {
    case 0:
      mpy = 600.0;
      if (hi_precision) mpy = mpy * 1000;

      m = (long)wxRound((a - (double)d) * mpy);

      if (!NEflag || NEflag < 1 || NEflag > 2)  // Does it EVER happen?
      {
        if (hi_precision)
          s.Printf(_T ( "%d%c %02ld.%04ld'" ), d, 0x00B0, m / 10000, m % 10000);
        else
          s.Printf(_T ( "%d%c %02ld.%01ld'" ), d, 0x00B0, m / 10, m % 10);
      } else {
        if (hi_precision)
          if (NEflag == 1)
            s.Printf(_T ( "%03d%c %02ld.%04ld' %c" ), d, 0x00B0, m / 10000,
                     (m % 10000), c);
          else
            s.Printf(_T ( "%03d%c %02ld.%04ld' %c" ), d, 0x00B0, m / 10000,
                     (m % 10000), c);
        else if (NEflag == 1)
          s.Printf(_T ( "%03d%c %02ld.%01ld' %c" ), d, 0x00B0, m / 10, (m % 10), c);
        else
          s.Printf(_T ( "%03d%c %02ld.%01ld' %c" ), d, 0x00B0, m / 10, (m % 10), c);
      }
      break;
    case 1:
      if (hi_precision)
        s.Printf(_T ( "%3.6f\u00B0 %c" ), ang, c);
      else
        s.Printf(_T ( "%3.4f\u00B0 %c" ), ang, c);
      break;
    case 2:
      m = (long)((a - (double)d) * 60);
      mpy = 10.0;
      if (hi_precision) mpy = mpy * 100;
      long sec = (long)((a - (double)d - (((double)m) / 60)) * 3600 * mpy);

      if (!NEflag || NEflag < 1 || NEflag > 2)
      {
        if (hi_precision)
          s.Printf(_T ( "%3d%c %ld'%ld.%ld\"" ), d, 0x00B0, m, sec / 1000,
                   sec % 1000);
        else
          s.Printf(_T ( "%3d%c %ld'%ld.%ld\"" ), d, 0x00B0, m, sec / 10, sec % 10);
      } else {
        if (hi_precision)
          if (NEflag == 1)
            s.Printf(_T ( "%03d%c %02ld' %02ld.%03ld\" %c" ), d, 0x00B0, m,
                     sec / 1000, sec % 1000, c);
          else
            s.Printf(_T ( "%03d%c %02ld' %02ld.%03ld\" %c" ), d, 0x00B0, m,
                     sec / 1000, sec % 1000, c);
        else if (NEflag == 1)
          s.Printf(_T ( "%03d%c %02ld' %02ld.%ld\" %c" ), d, 0x00B0, m, sec / 10,
                   sec % 10, c);
        else
          s.Printf(_T ( "%03d%c %02ld' %02ld.%ld\" %c" ), d, 0x00B0, m, sec / 10,
                   sec % 10, c);
      }
      break;
  }
  return s;
}
