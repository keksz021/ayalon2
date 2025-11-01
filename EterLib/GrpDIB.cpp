#include "StdAfx.h"
#include "GrpDIB.h"

CGraphicDib::CGraphicDib()
{
	Initialize();
}

CGraphicDib::~CGraphicDib()
{
	Destroy();
}

void CGraphicDib::Initialize()
{
	m_hDC=NULL;
	m_hBmp=NULL;
	m_pvBuf=NULL;
	m_width=0;
	m_height=0;
}

void CGraphicDib::Destroy()
{			
	if (m_hBmp) DeleteObject(m_hBmp);			
	if (m_hDC)	DeleteDC(m_hDC);
	
	Initialize();			
}
/* //régi
bool CGraphicDib::Create(HDC hDC, int width, int height)
{
	Destroy();

	m_width = width;
	m_height = height;

	ZeroMemory(&m_bmi.bmiHeader,  sizeof(BITMAPINFOHEADER));
	m_bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	m_bmi.bmiHeader.biWidth       =  m_width;
	m_bmi.bmiHeader.biHeight      = -m_height;
	m_bmi.bmiHeader.biPlanes      = 1;			
	m_bmi.bmiHeader.biBitCount    = 32;
	m_bmi.bmiHeader.biCompression = BI_RGB;

	m_hDC=CreateCompatibleDC(hDC);
	if (!m_hDC)
	{
		assert(!"CGraphicDib::Create CreateCompatibleDC Error");
		return false;
	}

	m_hBmp=CreateDIBSection(m_hDC, &m_bmi, DIB_RGB_COLORS, &m_pvBuf, NULL, 0);
	if (!m_hBmp)
	{
		assert(!"CGraphicDib::Create CreateDIBSection Error");
		return false;
	}

	SelectObject(m_hDC, m_hBmp);

	::SetTextColor(m_hDC, RGB(255, 255, 255));

	return true;
}
*/

bool CGraphicDib::Create(HDC hDC, int width, int height)
{
	Destroy();

	// Biztonsági határértékek
	if (width <= 0 || height <= 0)
	{
		m_width = m_height = 0;
		m_hDC = NULL;
		m_hBmp = NULL;
		m_pvBuf = NULL;
		return false;
	}

	// Túlméretezett kép elleni védelem (GDI limit)
	if (width > 8192 || height > 8192)
	{
		m_width = m_height = 0;
		m_hDC = NULL;
		m_hBmp = NULL;
		m_pvBuf = NULL;
		return false;
	}

	m_width = width;
	m_height = height;

	ZeroMemory(&m_bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
	m_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bmi.bmiHeader.biWidth = m_width;
	m_bmi.bmiHeader.biHeight = -m_height; // top-down
	m_bmi.bmiHeader.biPlanes = 1;
	m_bmi.bmiHeader.biBitCount = 32;
	m_bmi.bmiHeader.biCompression = BI_RGB;

	// Ha a hívó NULL HDC-t ad, csináljunk biztonságosan egyet
	HDC srcDC = hDC ? hDC : GetDC(NULL);
	bool releaseSrc = (hDC == NULL);

	m_hDC = CreateCompatibleDC(srcDC);

	if (releaseSrc)
		ReleaseDC(NULL, srcDC);

	if (!m_hDC)
	{
		m_hBmp = NULL;
		m_pvBuf = NULL;
		m_width = m_height = 0;
		return false;
	}

	m_hBmp = CreateDIBSection(m_hDC, &m_bmi, DIB_RGB_COLORS, &m_pvBuf, NULL, 0);
	if (!m_hBmp || !m_pvBuf)
	{
		DeleteDC(m_hDC);
		m_hDC = NULL;
		m_hBmp = NULL;
		m_pvBuf = NULL;
		m_width = m_height = 0;
		return false;
	}

	// Biztonságosan kiválasztjuk a bitmapet a DC-be
	SelectObject(m_hDC, m_hBmp);
	::SetTextColor(m_hDC, RGB(255, 255, 255));

	return true;
}



HDC	CGraphicDib::GetDCHandle()
{
	return m_hDC;
}

void CGraphicDib::SetBkMode(int iBkMode)
{
	::SetBkMode(m_hDC, iBkMode);
}

void CGraphicDib::TextOut(int ix, int iy, const char *c_szText)
{
	::SetBkColor(m_hDC, 0);	
	::TextOut(m_hDC, ix, iy, c_szText, strlen(c_szText));
}

void CGraphicDib::Put(HDC hDC, int x, int y)
{
	SetDIBitsToDevice(
	  hDC,     
	  x,       
	  y,       
	  m_width, 
	  m_height,
	  0,
	  0,
	  0,
	  m_height,
	  m_pvBuf,
	  &m_bmi,
	  DIB_RGB_COLORS
	);
}

void* CGraphicDib::GetPointer()
{
	return m_pvBuf;
}

int CGraphicDib::GetWidth()
{
	return m_width;
}

int CGraphicDib::GetHeight()
{
	return m_height;
}