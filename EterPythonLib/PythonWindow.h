#pragma once

#include "StdAfx.h"
#include "../eterBase/Utils.h"

#ifdef ENABLE_RENDER_TARGET
#include <cstdint>
#include "../EterLib/GrpRenderTargetTexture.h"
#endif

namespace UI
{
	class CWindow
	{
	public:
		typedef std::list<CWindow*> TWindowContainer;

		static DWORD Type();
		BOOL IsType(DWORD dwType);

		enum EHorizontalAlign
		{
			HORIZONTAL_ALIGN_LEFT = 0,
			HORIZONTAL_ALIGN_CENTER = 1,
			HORIZONTAL_ALIGN_RIGHT = 2,
		};

		enum EVerticalAlign
		{
			VERTICAL_ALIGN_TOP = 0,
			VERTICAL_ALIGN_CENTER = 1,
			VERTICAL_ALIGN_BOTTOM = 2,
		};

		enum EFlags
		{
			FLAG_MOVABLE = (1 << 0),
			FLAG_LIMIT = (1 << 1),
			FLAG_SNAP = (1 << 2),
			FLAG_DRAGABLE = (1 << 3),
			FLAG_ATTACH = (1 << 4),
			FLAG_RESTRICT_X = (1 << 5),
			FLAG_RESTRICT_Y = (1 << 6),
			FLAG_NOT_CAPTURE = (1 << 7),
			FLAG_FLOAT = (1 << 8),
			FLAG_NOT_PICK = (1 << 9),
			FLAG_IGNORE_SIZE = (1 << 10),
			FLAG_RTL = (1 << 11),
			FLAG_ALPHA_SENSITIVE = (1 << 12),
			FLAG_REMOVE_LIMIT = (1 << 13),
			FLAG_IS_BOARD_WITHOUT_ALPHA = (1 << 14),
#ifdef ENABLE_ANIMATED_BOARD
			FLAG_ANIMATED_BOARD = (1 << 15),
#endif
		};

		enum WindowTypes
		{
			WINDOW_TYPE_WINDOW,
			WINDOW_TYPE_EX_IMAGE,

			WINDOW_TYPE_MAX_NUM
		};

	public:
		CWindow(PyObject* ppyObject);
		virtual ~CWindow();

		void			AddChild(CWindow* pWin);

		void			Clear();
		void			DestroyHandle();
		void			Update();
		void			Render();

		void			SetName(const char* c_szName);
		const char* GetName() { return m_strName.c_str(); }
		void			SetSize(long width, long height);
		long			GetWidth() { return m_lWidth; }
		long			GetHeight() { return m_lHeight; }

		void			SetHorizontalAlign(DWORD dwAlign);
		void			SetVerticalAlign(DWORD dwAlign);
		void			SetPosition(long x, long y);
		void			GetPosition(long* plx, long* ply);
		long			GetPositionX(void) const { return m_x; }
		long			GetPositionY(void) const { return m_y; }
		RECT& GetRect() { return m_rect; }
		void			GetLocalPosition(long& rlx, long& rly);
		void			GetMouseLocalPosition(long& rlx, long& rly);
		long			UpdateRect();

		RECT& GetLimitBias() { return m_limitBiasRect; }
		void			SetLimitBias(long l, long r, long t, long b) { m_limitBiasRect.left = l, m_limitBiasRect.right = r, m_limitBiasRect.top = t, m_limitBiasRect.bottom = b; }

		void			Show();
		void			Hide();
		bool			IsShow() { return m_bShow; }
		bool			IsRendering();

		bool			HasParent() { return m_pParent ? true : false; }
		bool			HasChild() { return m_pChildList.empty() ? false : true; }
		int				GetChildCount() { return m_pChildList.size(); }

		void			IsTransparentOnPixel(long* x, long* y, bool* ret);
		void			HaveSomeChildOnOnPixel(long* x, long* y, bool* ret);

		CWindow* GetRoot();
		CWindow* GetParent();
		bool			IsChild(CWindow* pWin);
		void			DeleteChild(CWindow* pWin);
		void			SetTop(CWindow* pWin);

		bool			IsIn(long x, long y);
		bool			IsIn();
		CWindow* PickWindow(long x, long y);
		CWindow* PickTopWindow(long x, long y);

		void			__RemoveReserveChildren();

		void			AddFlag(DWORD flag) { SET_BIT(m_dwFlag, flag); }
		void			RemoveFlag(DWORD flag) { REMOVE_BIT(m_dwFlag, flag); }
		bool			IsFlag(DWORD flag) { return (m_dwFlag & flag) ? true : false; }

		virtual void	OnRender();
		virtual void	OnUpdate();
		virtual void	OnChangePosition() {}

		virtual void	OnSetFocus();
		virtual void	OnKillFocus();

		virtual void	OnMouseDrag(long lx, long ly);
		virtual void	OnMouseOverIn();
		virtual void	OnMouseOverOut();
		virtual void	OnMouseOver();
		virtual void	OnDrop();
		virtual void	OnTop();
		virtual void	OnIMEUpdate();

		virtual void	OnMoveWindow(long x, long y);

		BOOL			RunIMETabEvent();
		BOOL			RunIMEReturnEvent();
		BOOL			RunIMEKeyDownEvent(int ikey);

		CWindow* RunKeyDownEvent(int ikey);
		BOOL			RunKeyUpEvent(int ikey);
		BOOL			RunPressEscapeKeyEvent();
		BOOL			RunPressExitKeyEvent();

		virtual BOOL	OnIMETabEvent();
		virtual BOOL	OnIMEReturnEvent();
		virtual BOOL	OnIMEKeyDownEvent(int ikey);

		virtual BOOL	OnIMEChangeCodePage();
		virtual BOOL	OnIMEOpenCandidateListEvent();
		virtual BOOL	OnIMECloseCandidateListEvent();
		virtual BOOL	OnIMEOpenReadingWndEvent();
		virtual BOOL	OnIMECloseReadingWndEvent();

		virtual BOOL	OnMouseLeftButtonDown();
		virtual BOOL	OnMouseLeftButtonUp();
		virtual BOOL	OnMouseLeftButtonDoubleClick();
		virtual BOOL	OnMouseRightButtonDown();
		virtual BOOL	OnMouseRightButtonUp();
		virtual BOOL	OnMouseRightButtonDoubleClick();
		virtual BOOL	OnMouseMiddleButtonDown();
		virtual BOOL	OnMouseMiddleButtonUp();
		virtual BOOL	RunMouseWheelEvent(long nLen);

		virtual BOOL	OnKeyDown(int ikey);
		virtual BOOL	OnKeyUp(int ikey);
		virtual BOOL	OnPressEscapeKey();
		virtual BOOL	OnPressExitKey();
		///////////////////////////////////////

		virtual void	SetColor(DWORD dwColor) {}
		virtual BOOL	OnIsType(DWORD dwType);

		virtual BOOL	IsWindow() { return TRUE; }
#ifdef ENABLE_ANIMATED_BOARD
		virtual void SetScale(float fx, float fy);
#endif
	protected:
		std::string			m_strName;

		EHorizontalAlign	m_HorizontalAlign;
		EVerticalAlign		m_VerticalAlign;
		long				m_x, m_y;
		long				m_lWidth, m_lHeight;
		RECT				m_rect;
		RECT				m_limitBiasRect;

		bool				m_bMovable;
		bool				m_bShow;
#ifdef ENABLE_ANIMATED_BOARD
		bool m_bAnimatingShow;
		bool m_bAnimatingHide;
		float m_fAnimScaleStart;
		float m_fAnimScaleEnd;
		float m_fAnimScale;
		uint32_t m_dwAnimDuration;
		uint32_t m_dwAnimStartTime;

		D3DXVECTOR2 m_v2Scale;
		D3DXMATRIX m_matScaling;

#endif
		DWORD				m_dwFlag;

		PyObject* m_poHandler;

		CWindow* m_pParent;
		TWindowContainer	m_pChildList;

		BOOL				m_isUpdatingChildren;

		TWindowContainer	m_pReserveChildList;
		BYTE				m_windowType;

#ifdef _DEBUG
	public:
		DWORD				DEBUG_dwCounter;
#endif
	};

	class CLayer : public CWindow
	{
	public:
		CLayer(PyObject* ppyObject) : CWindow(ppyObject) {}
		virtual ~CLayer() {}

		BOOL IsWindow() { return FALSE; }
	};

#ifdef ENABLE_RENDER_TARGET
	class CRenderTarget : public CWindow
	{
	public:
		CRenderTarget(PyObject* ppyObject);
		virtual ~CRenderTarget();

		bool SetRenderTarget(int index);

#ifdef ENABLE_INGAME_WIKI_SYSTEM
		void SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom);
		RECT rect_ex;
#endif

	protected:
		DWORD m_dwIndex;
		void OnRender();
	};
#endif

	class CBox : public CWindow
	{
	public:
		CBox(PyObject* ppyObject);
		virtual ~CBox();

		void SetColor(DWORD dwColor);

	protected:
		void OnRender();

	protected:
		DWORD m_dwColor;
	};

	class CBar : public CWindow
	{
	public:
		CBar(PyObject* ppyObject);
		virtual ~CBar();

		void SetColor(DWORD dwColor);

	protected:
		void OnRender();

	protected:
		DWORD m_dwColor;
	};

	class CLine : public CWindow
	{
	public:
		CLine(PyObject* ppyObject);
		virtual ~CLine();

		void SetColor(DWORD dwColor);

	protected:
		void OnRender();

	protected:
		DWORD m_dwColor;
	};

	class CBar3D : public CWindow
	{
	public:
		static DWORD Type();

	public:
		CBar3D(PyObject* ppyObject);
		virtual ~CBar3D();

		void SetColor(DWORD dwLeft, DWORD dwRight, DWORD dwCenter);

	protected:
		void OnRender();

	protected:
		DWORD m_dwLeftColor;
		DWORD m_dwRightColor;
		DWORD m_dwCenterColor;
	};

	class CTextLine : public CWindow
	{
	public:
		CTextLine(PyObject* ppyObject);
		virtual ~CTextLine();

		void SetMax(int iMax);
		void SetHorizontalAlign(int iType);
		void SetVerticalAlign(int iType);
		void SetSecret(BOOL bFlag);
		void SetOutline(BOOL bFlag);
		void SetFeather(BOOL bFlag);
		void SetMultiLine(BOOL bFlag);
		void SetFontName(const char* c_szFontName);
		void SetFontColor(DWORD dwColor);
		void SetLimitWidth(float fWidth);

		void ShowCursor();
		void HideCursor();

		int GetCursorPosition();

		void SetText(const char* c_szText);
		const char* GetText();

		void GetTextSize(int* pnWidth, int* pnHeight);

#ifdef ENABLE_MULTI_TEXTLINE
		int GetTextLineCount();
		void DisableEnterToken();
		void SetLineHeight(int iHeight);
		int GetLineHeight();
#endif

	protected:
		void OnUpdate();
		void OnRender();
		void OnChangePosition();

		virtual void OnSetText(const char* c_szText);

	protected:
		CGraphicTextInstance m_TextInstance;
	};

	class CNumberLine : public CWindow
	{
	public:
		CNumberLine(PyObject* ppyObject);
		CNumberLine(CWindow* pParent);
		virtual ~CNumberLine();

		void SetPath(const char* c_szPath);
		void SetHorizontalAlign(int iType);
		void SetNumber(const char* c_szNumber);

	protected:
		void ClearNumber();
		void OnRender();
		void OnChangePosition();

	protected:
		std::string m_strPath;
		std::string m_strNumber;
		std::vector<CGraphicImageInstance*> m_ImageInstanceVector;

		int m_iHorizontalAlign;
		DWORD m_dwWidthSummary;
	};

	class CImageBox : public CWindow
	{
	public:
		CImageBox(PyObject* ppyObject);
		virtual ~CImageBox();

		BOOL LoadImage(const char* c_szFileName);
		BOOL UnloadImage();

		void SetDiffuseColor(float fr, float fg, float fb, float fa);

		int GetWidth();
		int GetHeight();

#ifdef ENABLE_NEW_DUNGEON_LIB
		void SetCoolTime(float fCoolTime);
		void SetCoolTimeStart(float fCoolTimeStart);
		float m_fCoolTime;
		float m_fCoolTimeStart;
#endif

	protected:
		virtual void OnCreateInstance();
		virtual void OnDestroyInstance();
		std::string m_strFileName;

		virtual void OnUpdate();
		virtual void OnRender();
		void OnChangePosition();

	protected:
		CGraphicImageInstance* m_pImageInstance;
	};
	class CMarkBox : public CWindow
	{
	public:
		CMarkBox(PyObject* ppyObject);
		virtual ~CMarkBox();

		void LoadImage(const char* c_szFilename);
		void SetDiffuseColor(float fr, float fg, float fb, float fa);
		void SetIndex(UINT uIndex);
		void SetScale(FLOAT fScale);

	protected:
		virtual void OnCreateInstance();
		virtual void OnDestroyInstance();
		

		virtual void OnUpdate();
		virtual void OnRender();
		void OnChangePosition();
	protected:
		CGraphicMarkInstance* m_pMarkInstance;
	};

	class CExpandedImageBox : public CImageBox
	{
	public:
		static DWORD Type();

	public:
		CExpandedImageBox(PyObject* ppyObject);
		virtual ~CExpandedImageBox();

		void SetScale(float fx, float fy);
		void SetOrigin(float fx, float fy);
		void SetRotation(float fRotation);
		void SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom);
		void SetRenderingRectWithScale(float fLeft, float fTop, float fRight, float fBottom);
		void SetRenderingMode(int iMode);

		void SetImageClipRect(float fLeft, float fTop, float fRight, float fBottom, bool bIsVertical = false);

		D3DXCOLOR GetPixelColor(int x, int y) { if (m_pImageInstance) return m_pImageInstance->GetPixelColor(x, y); else return D3DXCOLOR(0, 0, 0, 0); }

	protected:
		void OnCreateInstance();
		void OnDestroyInstance();

		virtual void OnUpdate();
		virtual void OnRender();

		BOOL OnIsType(DWORD dwType);
	};

	class CAniImageBox : public CWindow
	{
	public:
		static DWORD Type();

	public:
		CAniImageBox(PyObject* ppyObject);
		virtual ~CAniImageBox();

		void SetDelay(int iDelay);
		void SetDiffuseColor(float r, float g, float b, float a);
		void AppendImage(const char* c_szFileName);
		void SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom);
		void SetRenderingRectWithScale(float fLeft, float fTop, float fRight, float fBottom);
		void SetRenderingMode(int iMode);
		void SetAniImgScale(float x, float y);

		void ResetFrame();

#ifdef ENABLE_FISH_GAME
		void SetRotation(float fRotation);
		void SetMoveStatus(bool bFlag);
		void SetMoveSpeed(float speed);
		void SetMovePos(float directionX, float directionY);
		void CheckMove();
		void OnMoveDone();
		float GetRotation();
#endif

	protected:
		void OnUpdate();
		void OnRender();
		void OnChangePosition();
		virtual void OnEndFrame();

#ifdef ENABLE_FISH_GAME
		float nextX, nextY;
		bool moveStart;
		float fSpeed;
#endif

		BOOL OnIsType(DWORD dwType);

	protected:
		BYTE m_bycurDelay;
		BYTE m_byDelay;
		BYTE m_bycurIndex;
		std::vector<CGraphicExpandedImageInstance*> m_ImageVector;

		std::queue<std::string> m_ImageFileNames;
		std::function<void(CGraphicExpandedImageInstance*)> m_SetRenderingRect, m_SetRenderingMode, m_SetDiffuseColor;

		float m_fScaleX;
		float m_fScaleY;
	};

	class CButton : public CWindow
	{
	public:
		CButton(PyObject* ppyObject);
		virtual ~CButton();

		BOOL SetUpVisual(const char* c_szFileName);
		BOOL SetOverVisual(const char* c_szFileName);
		BOOL SetDownVisual(const char* c_szFileName);
		BOOL SetDisableVisual(const char* c_szFileName);

#ifdef ENABLE_DUNGEON_TRACKING_SYSTEM
#if defined(RENEWAL_MISSION_BOOKS)
		static DWORD Type();
		void SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom);
#endif
#endif
		const char* GetUpVisualFileName();
		const char* GetOverVisualFileName();
		const char* GetDownVisualFileName();

		void Flash();
		void EnableFlash();
		void DisableFlash();
		void Enable();
		void Disable();

		void SetUp();
		void Up();
		void Over();
		void Down();

#ifdef ENABLE_EMOTICONS_SYSTEM
		void SetScale(float fx, float fy);
#endif

		BOOL IsDisable();
		BOOL IsPressed();

	protected:
		void OnUpdate();
		void OnRender();
		void OnChangePosition();

		BOOL OnMouseLeftButtonDown();
		BOOL OnMouseLeftButtonDoubleClick();
		BOOL OnMouseLeftButtonUp();
		void OnMouseOverIn();
		void OnMouseOverOut();

		BOOL IsEnable();

#ifdef ENABLE_DUNGEON_TRACKING_SYSTEM
#if defined(RENEWAL_MISSION_BOOKS)
		void SetCurrentVisual(CGraphicExpandedImageInstance* pVisual);
		BOOL OnIsType(DWORD dwType);
#else
		void SetCurrentVisual(CGraphicImageInstance* pVisual);
#endif
#endif
	protected:
		BOOL m_bEnable;
		BOOL m_isPressed;
		BOOL m_isFlash;

#ifdef ENABLE_DUNGEON_TRACKING_SYSTEM
#if defined(RENEWAL_MISSION_BOOKS)
		CGraphicExpandedImageInstance* m_pcurVisual;
		CGraphicExpandedImageInstance m_upVisual;
		CGraphicExpandedImageInstance m_overVisual;
		CGraphicExpandedImageInstance m_downVisual;
		CGraphicExpandedImageInstance m_disableVisual;
#else
		CGraphicImageInstance* m_pcurVisual;
		CGraphicImageInstance m_upVisual;
		CGraphicImageInstance m_overVisual;
		CGraphicImageInstance m_downVisual;
		CGraphicImageInstance m_disableVisual;
#endif
#endif
	};

	class CRadioButton : public CButton
	{
	public:
#ifdef ENABLE_DUNGEON_TRACKING_SYSTEM
#if defined(RENEWAL_MISSION_BOOKS)
		static DWORD Type();
#endif
#endif
		CRadioButton(PyObject* ppyObject);
		virtual ~CRadioButton();

	protected:
		BOOL OnMouseLeftButtonDown();
		BOOL OnMouseLeftButtonUp();
		void OnMouseOverIn();
		void OnMouseOverOut();
#ifdef ENABLE_DUNGEON_TRACKING_SYSTEM
#if defined(RENEWAL_MISSION_BOOKS)
		BOOL OnIsType(DWORD dwType);
#endif
#endif
	};

	class CToggleButton : public CButton
	{
	public:
#ifdef ENABLE_DUNGEON_TRACKING_SYSTEM
		static DWORD Type();
#endif
		CToggleButton(PyObject* ppyObject);
		virtual ~CToggleButton();

	protected:
		BOOL OnMouseLeftButtonDown();
		BOOL OnMouseLeftButtonUp();
		void OnMouseOverIn();
		void OnMouseOverOut();
#ifdef ENABLE_DUNGEON_TRACKING_SYSTEM
#if defined(RENEWAL_MISSION_BOOKS)
		BOOL OnIsType(DWORD dwType);
#endif
#endif
	};

	class CDragButton : public CButton
	{
	public:
		CDragButton(PyObject* ppyObject);
		virtual ~CDragButton();

		void SetRestrictMovementArea(int ix, int iy, int iwidth, int iheight);

	protected:
		void OnChangePosition();
		void OnMouseOverIn();
		void OnMouseOverOut();

	protected:
		RECT m_restrictArea;
	};
};

extern BOOL g_bOutlineBoxEnable;
