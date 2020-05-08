
// FindDifferenceView.h: CFindDifferenceView 类的接口
//

#pragma once


class CFindDifferenceView : public CView
{
protected: // 仅从序列化创建
	CFindDifferenceView() noexcept;
	DECLARE_DYNCREATE(CFindDifferenceView)

// 特性
public:
	CFindDifferenceDoc* GetDocument() const;

// 操作
public:

// 重写
public:
	virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// 实现
public:
	virtual ~CFindDifferenceView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPicFit();
	afx_msg void OnOpenPic();
	afx_msg void OnOpen2ndPic();
	afx_msg void OnFindDifference();
	afx_msg void OnStartFindDifference();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};

#ifndef _DEBUG  // FindDifferenceView.cpp 中的调试版本
inline CFindDifferenceDoc* CFindDifferenceView::GetDocument() const
   { return reinterpret_cast<CFindDifferenceDoc*>(m_pDocument); }
#endif

