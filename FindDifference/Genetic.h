#pragma once
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
# include <opencv2/opencv.hpp>

// Genetic 对话框
struct ImageTrans		//该结构体用来返回变化的各种系数
{
	double angle;			//需要旋转的角度
	double scaleRatio;//
};

// Genetic 对话框

class Genetic : public CDialogEx
{
	DECLARE_DYNAMIC(Genetic)

public:
	Genetic(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~Genetic();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_Genetic };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	afx_msg
		ImageTrans Calc(cv::Mat correctImg, cv::Mat adjustImg);
	int IterationNum;
	afx_msg void OnBnClickedOk();
	double testAngle;
	double testRatio;
	afx_msg void OnBnClickedButtonTest();
};
