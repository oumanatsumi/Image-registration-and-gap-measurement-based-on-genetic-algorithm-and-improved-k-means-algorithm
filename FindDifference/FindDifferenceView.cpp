
// FindDifferenceView.cpp: CFindDifferenceView 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "FindDifference.h"
#endif

#include "FindDifferenceDoc.h"
#include "FindDifferenceView.h"
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
# include <opencv2/opencv.hpp>
# include <iostream>
#include <math.h>
#include <time.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "Genetic.h"
#include <opencv2\core\types_c.h>
using namespace cv;
using namespace std;

// CFindDifferenceView

IMPLEMENT_DYNCREATE(CFindDifferenceView, CView)

BEGIN_MESSAGE_MAP(CFindDifferenceView, CView)
	// 标准打印命令
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CFindDifferenceView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_Pic_Fit, &CFindDifferenceView::OnPicFit)
	ON_COMMAND(ID_OPEN_PIC, &CFindDifferenceView::OnOpenPic)
	ON_COMMAND(ID_OPEN_2ND_PIC, &CFindDifferenceView::OnOpen2ndPic)
	ON_COMMAND(ID_Find_Difference, &CFindDifferenceView::OnFindDifference)
	ON_COMMAND(ID_Start_Find_Difference, &CFindDifferenceView::OnStartFindDifference)
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

// CFindDifferenceView 构造/析构
Mat MainImg;//图像
Mat MainImgGray;//灰度图像
bool isOpenImg;//是否打开图片
Mat SecondImg;//第二张图
Mat SecondImgGray;//灰度图像

Mat TransImg;	//待旋转缩放的图
vector<CPoint> FindDiffBoundPoint;  //找出不同后存放边界的点
vector<vector<CPoint>>DiffPointSets;
//vector<CPoint> Diff1;		//第一个不同的边缘点集
//vector<CPoint> Diff2;		//第二个不同的边缘点集
Mat SubImg;//相减后的图
Mat AdjustResImg;//经过配准后的图
//Mat AdjustResImgWithoutResult;//结果图但是隐藏找出来的不同
Mat ComputerFindDiffResult;//计算机找不同的结果
//DiffBoundary* diffBounds;//每个不同处的边界数组

//找不同游戏
bool isAdjust = false;
bool isFindingDifference = false;
int LeftDiffNum = 2;//还剩几个不同
//bool isFindDiff1 = false;//是否找到了第一个
//bool isFindDiff2 = false;//是否找到了第二个
//CPoint Diff1LeftUp;			//第一个不同的左上
//CPoint Diff1RightDown;		//第一个不同的右下
//CPoint Diff2LeftUp;			//第二个不同的左上
//CPoint Diff2RightDown;	//第二个不同的右下

bool* isFindDiff;//是否找到了第n个的数组
class DiffBoundary//边界的类型
{
public:
	int Diff_X_Min = 0;
	int Diff_X_Max = 100;
	int Diff_Y_Min = 0;
	int Diff_Y_Max = 100;
};

vector<DiffBoundary> globalDiffBounds;

bool isComputerFindDiff=false;//电脑是否找过不同（针对先电脑找后手工找的情况）
CFindDifferenceView::CFindDifferenceView() noexcept
{
	// TODO: 在此处添加构造代码

}

CFindDifferenceView::~CFindDifferenceView()
{
}

BOOL CFindDifferenceView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return CView::PreCreateWindow(cs);
}

// CFindDifferenceView 绘图

void CFindDifferenceView::OnDraw(CDC* /*pDC*/)
{
	CFindDifferenceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: 在此处为本机数据添加绘制代码
}


// CFindDifferenceView 打印


void CFindDifferenceView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CFindDifferenceView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 默认准备
	return DoPreparePrinting(pInfo);
}

void CFindDifferenceView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加额外的打印前进行的初始化过程
}

void CFindDifferenceView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加打印后进行的清理过程
}

void CFindDifferenceView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CFindDifferenceView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CFindDifferenceView 诊断

#ifdef _DEBUG
void CFindDifferenceView::AssertValid() const
{
	CView::AssertValid();
}

void CFindDifferenceView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFindDifferenceDoc* CFindDifferenceView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFindDifferenceDoc)));
	return (CFindDifferenceDoc*)m_pDocument;
}
#endif //_DEBUG

bool sortFun_x(const CPoint& p1, const CPoint& p2)
{
	return p1.x < p2.x;//升序排列  
}

bool sortFun_y(const CPoint& p1, const CPoint& p2)
{
	return p1.y < p2.y;//升序排列  
}

// CFindDifferenceView 消息处理程序
Mat ImageRotate2(Mat oldimg, double roll)
{
	Point center(oldimg.cols / 2, oldimg.rows / 2);
	Mat Rx(3, 3, CV_32FC1);

	double theta_r = roll * 3.1415926 / 180; /** 3.1415926 / 180*/
	float cos_theta = cos(theta_r);
	float sin_theta = sin(theta_r);
	Rx.at<float>(0, 0) = cos_theta;
	Rx.at<float>(0, 1) = -sin_theta;
	Rx.at<float>(0, 2) = (1 - cos_theta) * center.x + center.y * sin_theta;

	Rx.at<float>(1, 0) = sin_theta;
	Rx.at<float>(1, 1) = cos_theta;
	Rx.at<float>(1, 2) = (1 - cos_theta) * center.y - center.x * sin_theta;

	Rx.at<float>(2, 0) = 0;
	Rx.at<float>(2, 1) = 0;
	Rx.at<float>(2, 2) = 1;
	//std::cout << rot_mat << std::endl;

	Mat rotated_ROI;
	//cv::warpAffine(face_img, rotated_ROI, rot_mat, face_img.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar::all(0));
	warpPerspective(oldimg, rotated_ROI, Rx, Size(oldimg.cols, oldimg.rows));
	//imshow("roll face", rotated_ROI);
	return rotated_ROI;
}

Mat ImageScale2(Mat oldimg, float ScaleNum)
{
	// TODO: 在此添加命令处理程序代码
	int oldLocX, oldLocY;//新的坐标
	int newRows, newCols;//新的高和宽
	newRows = oldimg.rows * ScaleNum;
	newCols = oldimg.cols * ScaleNum;

	Mat newImg;
	newImg.create(newRows, newCols, oldimg.type());
	for (int i = 0; i < newCols; i++)
	{
		for (int j = 0; j < newRows; j++)
		{
			oldLocX = (int)(i * (1 / ScaleNum) + 0.5);
			oldLocY = (int)(j * (1 / ScaleNum) + 0.5);
			newImg.at<Vec3b>(j, i)[0] = oldimg.at<Vec3b>(oldLocY, oldLocX)[0];
			newImg.at<Vec3b>(j, i)[1] = oldimg.at<Vec3b>(oldLocY, oldLocX)[1];
			newImg.at<Vec3b>(j, i)[2] = oldimg.at<Vec3b>(oldLocY, oldLocX)[2];
		}
	}
	Mat resImg;
	resImg.create(oldimg.size(), oldimg.type());
	int cols_offset = newImg.cols - oldimg.cols;
	int rows_offset = newImg.rows - oldimg.rows;
	for (int i = 0; i < oldimg.cols; i++)
	{
		for (int j = 0; j < oldimg.rows; j++)
		{
			resImg.at<Vec3b>(j, i)[0] = newImg.at<Vec3b>(j + rows_offset / 2, i + cols_offset / 2)[0];
			resImg.at<Vec3b>(j, i)[1] = newImg.at<Vec3b>(j + rows_offset / 2, i + cols_offset / 2)[1];
			resImg.at<Vec3b>(j, i)[2] = newImg.at<Vec3b>(j + rows_offset / 2, i + cols_offset / 2)[2];
		}
	}
	return resImg;
	//imshow("缩放后图像", resImg);
}


void CFindDifferenceView::OnOpenPic()
{
	// TODO: 在此添加命令处理程序代码
	CString filePath;
	CFileDialog fileDlg(TRUE, L"*.bmp", filePath.GetBuffer(), OFN_HIDEREADONLY, L"bmp文件(*.bmp)|*.bmp|jpg文件(*.jpg)|*.jpg||");
	if (fileDlg.DoModal() == IDOK)
		filePath = fileDlg.GetPathName();
	USES_CONVERSION;
	cv::String cvfilePath = W2A(filePath);
	MainImg = cv::imread(cvfilePath);
	MainImgGray = cv::imread(cvfilePath, 0);
	bool if_selected = false;
	if (filePath == L"")
	{
		MessageBox(L"请选择图片！");
	}
	else
	{
		if_selected = true;
	}
	if (if_selected)//如果选择了图片
	{
		imshow("原图像", MainImg);
		imshow("原图像灰度图", MainImgGray);
	}
	CDC* pDC = new CClientDC(this);
	//CPen* pen_Rectangle = new CPen(PS_SOLID, 3, RGB(0, 0, 255));
	//CPen* dashPen = new CPen(PS_DASH, 1, RGB(0, 0, 0));
	//pDC->SelectObject(pen_Rectangle);

	//pDC->MoveTo(10, 10);
	//pDC->LineTo(100, 100);
	for (int i = 0; i < MainImg.cols; i++)
	{
		for (int j = 0; j < MainImg.rows; j++)
		{
			int B = MainImg.at<Vec3b>(j, i)[0];
			int G = MainImg.at<Vec3b>(j, i)[1];
			int R = MainImg.at<Vec3b>(j, i)[2];
			pDC->SetPixel(i, j, RGB(R, G, B));
		}
	}
}


void CFindDifferenceView::OnOpen2ndPic()
{
	// TODO: 在此添加命令处理程序代码
	CString filePath;
	CFileDialog fileDlg(TRUE, L"*.bmp", filePath.GetBuffer(), OFN_HIDEREADONLY, L"bmp文件(*.bmp)|*.bmp|jpg文件(*.jpg)|*.jpg||");
	if (fileDlg.DoModal() == IDOK)
		filePath = fileDlg.GetPathName();
	USES_CONVERSION;
	cv::String cvfilePath = W2A(filePath);
	SecondImg = cv::imread(cvfilePath);
	SecondImgGray = cv::imread(cvfilePath, 0);
	bool if_selected = false;
	if (filePath == L"")
	{
		MessageBox(L"请选择图片！");
	}
	else
	{
		if_selected = true;
	}
	if (if_selected)//如果选择了图片
	{
		imshow("图2原图像", SecondImg);
		imshow("图2灰度图", SecondImgGray);
	}

	CDC* pDC = new CClientDC(this);
	//CPen* pen_Rectangle = new CPen(PS_SOLID, 3, RGB(0, 0, 255));
	//CPen* dashPen = new CPen(PS_DASH, 1, RGB(0, 0, 0));
	//pDC->SelectObject(pen_Rectangle);

	//pDC->MoveTo(10, 10);
	//pDC->LineTo(100, 100);
	for (int i = 0; i < SecondImg.cols; i++)
	{
		for (int j = 0; j < SecondImg.rows; j++)
		{
			int B = SecondImg.at<Vec3b>(j, i)[0];
			int G = SecondImg.at<Vec3b>(j, i)[1];
			int R = SecondImg.at<Vec3b>(j, i)[2];
			pDC->SetPixel(i+MainImg.cols+100, j, RGB(R, G, B));

		}
	}

}


void CFindDifferenceView::OnPicFit()
{
	// TODO: 在此添加命令处理程序代码
	if (MainImg.empty() || SecondImg.empty())
	{
		MessageBox(L"请先打开两张图片！");
	}
	else
	{
		if (!isAdjust)
		{
			Genetic dlg;
			dlg.DoModal();
			//开始遗传算法

			ImageTrans  x = dlg.Calc(MainImg, SecondImg);
			double angle = x.angle;
			double ratio = x.scaleRatio;
			//ImageTrans  x;
			//double angle = -10;
			//double ratio = 1.25;

			CString s;
			s.Format(L"最佳旋转角度:%lf", angle);
			MessageBox(s);
			s.Format(L"最佳缩放比例:%lf", ratio);
			MessageBox(s);
			Mat RoImg = ImageRotate2(SecondImg, angle);
			AdjustResImg = ImageScale2(RoImg, ratio);
			imshow("配准结果", AdjustResImg);

			CDC* pDC = new CClientDC(this);
			for (int i = 0; i < AdjustResImg.cols; i++)
			{
				for (int j = 0; j < AdjustResImg.rows; j++)
				{
					int B = AdjustResImg.at<Vec3b>(j, i)[0];
					int G = AdjustResImg.at<Vec3b>(j, i)[1];
					int R = AdjustResImg.at<Vec3b>(j, i)[2];
					pDC->SetPixel(i + MainImg.cols + 100, j, RGB(R, G, B));

				}
			}
			isAdjust = true;
		}
		else
		{
			MessageBox(L"你已经进行过配准了！");
		}

	}
}

/****************************************************/
/****************************************************/
/**************       K-Means算法(1)！！    **************/
/****************************************************/
/****************************************************/


////欧式距离
//double Eul_dis(double x1, double y1, double x2, double y2) {
//	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
//}
//
////初始k个质心
//void Get_centroid(int n) {
//	double init_x = 0.0, init_y = 0.0;
//	n = FindDiffBoundPoint.size();
//	for (int i = 1; i <= n; ++i) {
//		//printf("请输入第%d个数据坐标：", i);
//		//scanf("%lf %lf", &s[i].x, &s[i].y);
//		s[i].x = FindDiffBoundPoint[i - 1].x;
//		s[i].y = FindDiffBoundPoint[i - 1].y;
//		init_x += s[i].x;
//		init_y += s[i].y;
//	}
//	init_x /= (double)n;
//	init_y /= (double)n;        //选择一个初始全局质心
//	//printf("初始一个质心：(%.2lf, %.2lf)\n", init_x, init_y);
//	for (int i = 1; i <= n; ++i) {
//		ss[i].dis = Eul_dis(init_x, init_y, s[i].x, s[i].y);
//		ss[i].id = i;
//	}
//	sort(ss + 1, ss + n + 1, cmp); //选择距离初始全局质心最远的k个点，作为初始的k个质心
//	//m_Node first = ss[0];
//	//m_Node second = ss[1];
//
//	//for (int i = 0; i < 10; i++)
//	//{
//	//	if (ss[i].dis > first.dis)
//	//	{
//	//		second = first;
//	//		first = ss[i];
//	//	}
//	//	else if (ss[i].dis < first.dis && ss[i].dis > second.dis)
//	//	{
//	//		second = ss[i];
//	//	}
//	//}
//
//	//printf("请输入您想收敛的数据群个数：");
//	//scanf("%d", &k); //k个质心 
//	//m_mean[1].x = s[first.id].x;
//	//m_mean[1].y = s[first.id].y;
//	//m_mean[2].x = s[second.id].x;
//	//m_mean[2].y = s[second.id].y;
//
//	for (int i = 1; i <= 2; ++i) {
//		int cnt = ss[i].id;
//		m_mean[i].x = s[cnt].x;
//		m_mean[i].y = s[cnt].y;
//	}
//}
//
////迭代更新质心
//void K_means(int n, int k) {
//	double max_dis = 0.0, limit_dis = 0.5;
//	do {    //我用的质心最大移动距离作为收敛条件，当然也可以用其他方法 
//		for (int i = 1; i <= k; ++i) {
//			V[i].clear();
//		}
//		for (int i = 1; i <= n; ++i) { //枚举数据点和各个质心 
//			double dis = -1.0;
//			int cnt;
//			for (int j = 1; j <= k; ++j) {
//				double ans = Eul_dis(s[i].x, s[i].y, m_mean[j].x, m_mean[j].y);
//				if (ans > dis) {
//					dis = ans;
//					cnt = j;
//				}
//			}
//			V[cnt].push_back(i); //使数据点对应其质心
//		}
//		for (int i = 1; i <= k; ++i) {
//			double sum_x = 0.0, sum_y = 0.0;
//			for (int j = 0; j < V[i].size(); ++j) {
//				int cnt = V[i][j];
//				sum_x += s[cnt].x;
//				sum_y += s[cnt].y;
//			}
//			double ans1 = sum_x / (double)V[i].size();
//			double ans2 = sum_y / (double)V[i].size();
//			max_dis = min(max_dis, Eul_dis(m_mean[i].x, m_mean[i].y, ans1, ans2));
//			m_mean[i].x = ans1; //更新质心坐标
//			m_mean[i].y = ans2;
//		}
//	} while (max_dis > limit_dis);
//}
//
////打印结果 
//void Print_node(int n, int k) {
//	for (int i = 1; i <= k; ++i) {
//		//printf("\n\n\n");
//		//printf("第%d个簇的质心坐标：(%.2lf, %.2lf)\n", i, m_mean[i].x, m_mean[i].y);
//		//printf("第%d个簇的数据个数：%d\n", i, V[i].size());
//		for (int j = 0; j < V[i].size(); ++j) {
//			int cnt = V[i][j];
//			//printf("(%.2lf, %.2lf)\n", s[cnt].x, s[cnt].y);
//			if (i == 1)
//			{
//				CPoint newPoint;
//				newPoint.x = s[cnt].x;
//				newPoint.y = s[cnt].y;
//				Diff1.push_back(newPoint);
//			}
//			if (i == 2)
//			{
//				CPoint newPoint;
//				newPoint.x = s[cnt].x;
//				newPoint.y = s[cnt].y;
//				Diff2.push_back(newPoint);
//			}
//		}
//	}
//}


/****************************************************/
/****************************************************/
/**************       K-Means算法(2)！！    **************/
/****************************************************/
/****************************************************/

int K, Vectordim, datasize, seed = 1;
float** m_data, ** kmatrix;
float* max_column, * min_column;

/*创建维数可指定的二维动态数组array[m][n]*/
float** m_array(int m, int n)
{
	float** p;
	int i;
	p = (float**)malloc(m * sizeof(float*));
	p[0] = (float*)malloc(m * n * sizeof(float));
	for (i = 1; i < m; ++i) p[i] = p[i - 1] + n;
	return p;
}

/*释放二维数组所占用的内存*/
void freearray(float** p)
{
	free(*p); free(p);
}

void loaddata(int input_K)
{
	FILE* fp;
	int i, j;
	K = input_K; //自定义k
	Vectordim = 2; datasize = FindDiffBoundPoint.size();
	m_data = m_array(datasize, Vectordim + 1);
	for (i = 0; i < datasize; i++)
	{
		m_data[i][Vectordim] = 0;
		m_data[i][0] = (float)FindDiffBoundPoint[i].x;
		m_data[i][1] = (float)FindDiffBoundPoint[i].y;
	}
}

double euclid_distance(float a[], float b[], int dim)
{
	int i;
	double sum = 0;
	for (i = 0; i < dim; i++)
		sum += pow(a[i] - b[i], 2);
	return sqrt(sum);
}

void getmaxmin(float** a)
{
	int i, j;
	max_column = (float*)malloc(sizeof(float) * Vectordim);
	min_column = (float*)malloc(sizeof(float) * Vectordim);

	for (i = 0; i < Vectordim; i++)
	{
		max_column[i] = a[0][i];
		min_column[i] = a[0][i];
	}

	for (i = 0; i < Vectordim; i++)
	{
		for (j = 1; j < datasize; j++)
		{
			if (a[j][i] > max_column[i])max_column[i] = a[j][i];
			if (a[j][i] < min_column[i])min_column[i] = a[j][i];
			/*printf("max_column[%d]=%f,  min_column[%d]=%f\n",i,max_column[i],i,min_column[i]);*/
		}
	}
}

void initializerandom()
{
	seed++;
	srand((unsigned)time(NULL) + seed);
}

float randomreal(float Low, float High)
{
	return ((float)rand() / RAND_MAX) * (High - Low) + Low;
}

void K_locations_random()
{
	int i, j;
	kmatrix = m_array(K, Vectordim + 1);
	//printf("Randomly the K-locations are initialized as follows:\n");
	for (i = 0; i < K; i++)
	{
		initializerandom();
		kmatrix[i][Vectordim] = (float)(i + 1);
		//printf("location---%d:  ", i + 1);
		for (j = 0; j < Vectordim; j++)
		{
			kmatrix[i][j] = randomreal(min_column[i], max_column[i]); //printf("%f,   ", kmatrix[i][j]);
		}
		//printf("\n");
	}
}

int existemptyclass()
{
	int* empty, i, j, ef;
	empty = (int*)malloc(sizeof(int) * K);
	for (i = 0; i < K; i++) empty[i] = 0;
	for (i = 0; i < datasize; i++)
	{
		for (j = 1; j <= K; j++)
		{
			if (j == (int)m_data[i][Vectordim]) empty[j - 1]++;
		}
	}
	for (i = 0, ef = 0; i < K; i++)
		if (0 == empty[i]) ef = 1;
	return ef;
}

int cluster()
{
	int i, j, flag, eflag = 1;
	double closest, d;
	for (i = 0; i < datasize; i++)
	{
		closest = euclid_distance(m_data[i], kmatrix[0], Vectordim);
		flag = 1;

		for (j = 1; j < K; j++)
		{
			d = euclid_distance(m_data[i], kmatrix[j], Vectordim);
			if (d < closest) { closest = d; flag = j + 1; }
		}
		if (m_data[i][Vectordim] != (float)flag) { eflag = 0; }
		m_data[i][Vectordim] = (float)flag;
	}

	return eflag;
}

void update_k_location()
{
	int i, j, number, m;
	float* temp;
	temp = (float*)malloc(sizeof(float) * (Vectordim));
	for (m = 0; m < Vectordim; m++) temp[m] = 0;
	for (number = 0, i = 1; i <= K; i++)
	{
		for (m = 0; m < Vectordim; m++) temp[m] = 0;
		for (j = 0; j < datasize; j++)
		{
			if (m_data[j][Vectordim] == i)
			{
				number++;
				for (m = 0; m < Vectordim; m++)
				{
					temp[m] += m_data[j][m];
				}
			}
		}
		for (m = 0; m < Vectordim; m++)
		{
			kmatrix[i - 1][m] = temp[m] / number;
			/*printf("%f\n",kmatrix[i-1][m]);*/
		}
	}
	free(temp);
}

void output()
{
	int i, j, m;
	/*for(m=0;m<datasize;m++)*/
	/*printf("data[%d][Vectordim]=%f\n",m,data[m][Vectordim]);*/
	for (i = 0; i < K; i++)
	{
		vector<CPoint> newDiffSet;
		DiffPointSets.push_back(newDiffSet);
	}
	//for (i = 1; i <= K; i++)
	//{
		//printf("The following data are clusterd as CLASS %d:\n", i);
		for (j = 0; j < datasize; j++)
		{
			if (m_data[j][Vectordim] ==  1.0f)
			{
				/*	for (m = 0; m < Vectordim; m++) printf("%f  ", m_data[j][m]);
					printf("\n");*/
				CPoint newPoint;
				newPoint.x = m_data[j][0];
				newPoint.y = m_data[j][1];
			/*	Diff1.push_back(newPoint);*/
				DiffPointSets[0].push_back(newPoint);
			}
			if (m_data[j][Vectordim] == 2.0f)
			{
				/*	for (m = 0; m < Vectordim; m++) printf("%f  ", m_data[j][m]);
					printf("\n");*/
				CPoint newPoint;
				newPoint.x = m_data[j][0];
				newPoint.y = m_data[j][1];
		/*		Diff2.push_back(newPoint);*/
				DiffPointSets[1].push_back(newPoint);
			}
			if (m_data[j][Vectordim] == 3.0f)
			{
				/*	for (m = 0; m < Vectordim; m++) printf("%f  ", m_data[j][m]);
					printf("\n");*/
				CPoint newPoint;
				newPoint.x = m_data[j][0];
				newPoint.y = m_data[j][1];
				DiffPointSets[2].push_back(newPoint);
			}
			if (m_data[j][Vectordim] == 4.0f)
			{
				/*	for (m = 0; m < Vectordim; m++) printf("%f  ", m_data[j][m]);
					printf("\n");*/
				CPoint newPoint;
				newPoint.x = m_data[j][0];
				newPoint.y = m_data[j][1];
				DiffPointSets[3].push_back(newPoint);
			}
		}
	//}
}

/****************************************************/
/****************************************************/
/**************       K-Means算法(2)END    *************/
/****************************************************/
/****************************************************/

void SobelDiv(Mat img, int threshold)
{

	for (int i = 15; i < img.cols - 15; i++)
	{
		for (int j = 15; j < img.rows - 15; j++)
		{
			int xB = img.at<Vec3b>(j + 1, i - 1)[0] + 2 * img.at<Vec3b>(j + 1, i)[0] + img.at<Vec3b>(j + 1, i + 1)[0]
				- img.at<Vec3b>(j - 1, i - 1)[0] - 2 * img.at<Vec3b>(j - 1, i)[0] - img.at<Vec3b>(j - 1, i + 1)[0];
			int xG = img.at<Vec3b>(j + 1, i - 1)[1] + 2 * img.at<Vec3b>(j + 1, i)[1] + img.at<Vec3b>(j + 1, i + 1)[1]
				- img.at<Vec3b>(j - 1, i - 1)[1] - 2 * img.at<Vec3b>(j - 1, i)[1] - img.at<Vec3b>(j - 1, i + 1)[1];
			int xR = img.at<Vec3b>(j + 1, i - 1)[2] + 2 * img.at<Vec3b>(j + 1, i)[2] + img.at<Vec3b>(j + 1, i + 1)[2]
				- img.at<Vec3b>(j - 1, i - 1)[2] - 2 * img.at<Vec3b>(j - 1, i)[2] - img.at<Vec3b>(j - 1, i + 1)[2];
			int yB = img.at<Vec3b>(j - 1, i - 1)[0] + 2 * img.at<Vec3b>(j, i - 1)[0] + img.at<Vec3b>(j + 1, i - 1)[0]
				- img.at<Vec3b>(j - 1, i + 1)[0] - 2 * img.at<Vec3b>(j, i + 1)[0] - img.at<Vec3b>(j + 1, i + 1)[0];
			int yG = img.at<Vec3b>(j - 1, i - 1)[1] + 2 * img.at<Vec3b>(j, i - 1)[1] + img.at<Vec3b>(j + 1, i - 1)[1]
				- img.at<Vec3b>(j - 1, i + 1)[1] - 2 * img.at<Vec3b>(j, i + 1)[1] - img.at<Vec3b>(j + 1, i + 1)[1];
			int yR = img.at<Vec3b>(j - 1, i - 1)[2] + 2 * img.at<Vec3b>(j, i - 1)[2] + img.at<Vec3b>(j + 1, i - 1)[2]
				- img.at<Vec3b>(j - 1, i + 1)[2] - 2 * img.at<Vec3b>(j, i + 1)[2] - img.at<Vec3b>(j + 1, i + 1)[2];
			double tB = sqrt((double)(xB * xB + yB * yB) + 0.5);
			if (tB > 255)	tB = 255;
			double tG = sqrt((double)(xG * xG + yG * yG) + 0.5);
			if (tG > 255)	tG = 255;
			double tR = sqrt((double)(xR * xR + yR * yR) + 0.5);
			if (tR > 255)	tR = 255;

			/*if (tB > threshold && tG > threshold && tR > threshold)*/
			if ((tB + tG + tB) / 3 > threshold)
			{
				CPoint newPoint;
				newPoint.x = i;
				newPoint.y = j;
				FindDiffBoundPoint.push_back(newPoint);
				//SubImg.at<Vec3b>(j, i)[0] = 255;
				//SubImg.at<Vec3b>(j, i)[1] = 255;
				//SubImg.at<Vec3b>(j, i)[2] = 255;
			}
		}
	}
	//imshow("Sobel后的差值图像", SubImg);
}

void ImageSub(Mat img1, Mat img2)
{
	int PlusImgCols = min(img1.cols, img2.cols);
	int PlusImgRows = min(img1.rows, img2.rows);
	SubImg.create(PlusImgRows, PlusImgCols, img1.type());
	for (int i = 0; i < PlusImgCols; i++)
	{
		for (int j = 0; j < PlusImgRows; j++)
		{
			SubImg.at<Vec3b>(j, i)[0] = abs(img1.at<Vec3b>(j, i)[0] - img2.at<Vec3b>(j, i)[0]);
			SubImg.at<Vec3b>(j, i)[1] = abs(img1.at<Vec3b>(j, i)[1] - img2.at<Vec3b>(j, i)[1]);
			SubImg.at<Vec3b>(j, i)[2] = abs(img1.at<Vec3b>(j, i)[2] - img2.at<Vec3b>(j, i)[2]);
		}
	}
	//imshow("两图相减新图像", SubImg);
}


void Kmeans(int K)
{
	//Diff1.clear();//清空之前的一次kmeans的结果
	//Diff2.clear();
	for (int i = 0; i < DiffPointSets.size(); i++)
	{
		DiffPointSets[i].clear();
	}

	int end_flag, empty_flag = 0;
	long int time;
	long int iteration = 0;
	loaddata(K);
	getmaxmin(m_data);
	while (iteration<10000)
	{
		iteration++;
		K_locations_random();
		end_flag = cluster();
		if (existemptyclass()) { /*MessageBox(L"There is a empty class!\nSo restart!\n");*/ continue; }
		time = 0;
		while (!end_flag)
		{
			if (time > 1000)break;
			time++;
			update_k_location();
			end_flag = cluster();
		}
		empty_flag = existemptyclass();
		if (empty_flag) { /*MessageBox(L"There is a empty class!\nSo restart!\n");*/ continue; }
		else break;
	}
	output();
}


//float evaluate_K_distance(Mat centers,int K)
//{
//	DiffPointSets.clear();//清空之前的数据
//	for (int i = 0; i < K; i++)
//	{
//		vector<CPoint> newDiffSet;
//		DiffPointSets.push_back(newDiffSet);//开辟新的空间
//	}
//	float evaluatedDistance = 0.0;//总距离（用来评估K的取值，函数返回这个值）
//	for (int i = 0; i < FindDiffBoundPoint.size(); i++)
//	{
//		float cloestDistance=999999;//与该点距离最近的中心的距离
//		int cloestGroup;//与该点距离最近的中心的编号
//		for (int j = 0; j < K; j++)
//		{
//			float tempDistanceX = FindDiffBoundPoint[i].x - centers.at<Point2f>(j, 0).x;
//			float tempDistanceY = FindDiffBoundPoint[i].y - centers.at<Point2f>(j, 0).y;
//			float euclideanDistance = sqrt(tempDistanceX * tempDistanceX + tempDistanceY * tempDistanceY);//计算欧氏距离
//			if (euclideanDistance < cloestDistance)
//			{
//				cloestDistance = euclideanDistance;//更新最短距离
//				cloestGroup = j;//更新最近组
//			}
//		}
//		DiffPointSets[cloestGroup].push_back(FindDiffBoundPoint[i]);//将该点存入相应的组中
//		evaluatedDistance += cloestDistance;
//	}
//	return evaluatedDistance;
//}

void ClusterPoints(Mat centers, int K)//进行聚类，将所有点归类
{
	DiffPointSets.clear();//清空之前的数据
	for (int i = 0; i < K; i++)
	{
		vector<CPoint> newDiffSet;
		DiffPointSets.push_back(newDiffSet);//开辟新的空间
	}
	for (int i = 0; i < FindDiffBoundPoint.size(); i++)
	{
		float cloestDistance = 999999;//与该点距离最近的中心的距离
		int cloestGroup;//与该点距离最近的中心的编号
		for (int j = 0; j < K; j++)
		{
			float tempDistanceX = FindDiffBoundPoint[i].x - centers.at<Point2f>(j, 0).x;
			float tempDistanceY = FindDiffBoundPoint[i].y - centers.at<Point2f>(j, 0).y;
			float euclideanDistance = sqrt(tempDistanceX * tempDistanceX + tempDistanceY * tempDistanceY);//计算欧氏距离
			if (euclideanDistance < cloestDistance)
			{
				cloestDistance = euclideanDistance;//更新最短距离
				cloestGroup = j;//更新最近组
			}
		}
		DiffPointSets[cloestGroup].push_back(FindDiffBoundPoint[i]);//将该点存入相应的组中
	}
}


float Evaluate_K_Silhouette_Coefficient(int K)//根据轮廓系数
{
	float siTotal = 0.0;//每个点的si累加值
	for (int i = 0; i < DiffPointSets.size(); i++)
	{
		for (int j = 0; j < DiffPointSets[i].size(); j++)
		{
			//对每个点集中的每个点操作
			//首先计算a(i)
			float ai = 0.0;
			for (int k = 0; k < DiffPointSets[i].size();k++ )
			{
				float tempDistanceX = (float)DiffPointSets[i][j].x - (float)DiffPointSets[i][k].x;
				float tempDistanceY = (float)DiffPointSets[i][j].y - (float)DiffPointSets[i][k].y;
				float euclideanDistance = sqrt(tempDistanceX * tempDistanceX + tempDistanceY * tempDistanceY);//计算欧氏距离
				ai += euclideanDistance;
			}
			ai /= DiffPointSets[i].size();//ai是取的平均值

			//接下来计算bi
			float bi = 999999.9;
			for (int m = 0; m < DiffPointSets.size(); m++)//对于每一个点集
			{
				float biThisSet = 0.0;
				if (m != i)//排除自身所在的那个点集
				{
					for (int n = 0; n < DiffPointSets[m].size(); n++)
					{
						float tempDistanceX = (float)DiffPointSets[i][j].x - (float)DiffPointSets[m][n].x;
						float tempDistanceY = (float)DiffPointSets[i][j].y - (float)DiffPointSets[m][n].y;
						float euclideanDistance = sqrt(tempDistanceX * tempDistanceX + tempDistanceY * tempDistanceY);//计算欧氏距离
						biThisSet += euclideanDistance;
					}
					biThisSet /= DiffPointSets[m].size();
					if (biThisSet < bi) bi = biThisSet;
				}
			}

			//通过ai和bi计算si
			float si = (bi - ai) / max(ai, bi);
			siTotal += si;
		}
	}
	return siTotal/FindDiffBoundPoint.size();
}

void CFindDifferenceView::OnFindDifference()
{
	// TODO: 在此添加命令处理程序代码
	if (!isAdjust)
	{
		MessageBox(L"请先进行图像配准！");
	}
	else
	{
		ImageSub(MainImg, AdjustResImg);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		imshow("中值滤波", SubImg);
		//RegionGrowing();
		/*SobelDiv(SubImg,10);*/
		//Get_centroid(FindDiffBoundPoint.size());
		//K_means(FindDiffBoundPoint.size(), 2);
		//Print_node(FindDiffBoundPoint.size(), 2);

		SobelDiv(SubImg, 80);

		//for (int i = 0; i < FindDiffBoundPoint.size(); i++)
		//{
		//	FindDiffBoundPoint[i].x /= MainImg.cols;
		//	FindDiffBoundPoint[i].y /= MainImg.rows;
		//}

		//Kmeans(3);

		//CString pointData;
		//for (int i = 0; i < FindDiffBoundPoint.size(); i++)
		//{
		//	CString temp;
		//	temp.Format(L"%lf %lf %lf\r\n", m_data[i][0], m_data[i][1], m_data[i][2]);
		//	pointData += temp;
		//}
		//CFile fp;
		//fp.Open(L"mydata.txt", CFile::modeWrite | CFile::modeCreate);
		//fp.Write(pointData.GetBuffer(), pointData.GetLength() * 2);
		//fp.Close();
		
		Mat InputData(FindDiffBoundPoint.size(),1, CV_32FC2),labels;
		for (int i = 0; i < InputData.rows; i++)
		{
			InputData.at<Point2f>(i,0).x= FindDiffBoundPoint[i].x;
			InputData.at<Point2f>(i, 0).y = FindDiffBoundPoint[i].y;
		}
		

		//int bestK = 2;
		//float bestValue = 999999;
		//for (int k = 2; k < 5; k++)//测试不同的K的效果，kmeans的更改第二个参数
		//{
		//	Mat centers(k, 1, InputData.type());    //用来存储聚类后的中心点
		//	kmeans(InputData, k, labels, TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0), 3, KMEANS_PP_CENTERS, centers);
		//	float valueTemp=evaluate_K_distance(centers, k);
		//	if (valueTemp < bestValue)
		//	{
		//		bestValue = valueTemp;
		//		bestK = k;
		//	}
		//}
		//
		//Mat centers(bestK, 1, InputData.type());    //用来存储聚类后的中心点
		//kmeans(InputData, bestK, labels, TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0), 3, KMEANS_PP_CENTERS, centers);
		//evaluate_K_distance(centers, bestK);


		int bestK = 2;
		float bestValue = -1.1;
		for (int k = 2; k < 5; k++)//测试不同的K的效果，kmeans的更改第二个参数
		{
			Mat centers(k, 1, InputData.type());    //用来存储聚类后的中心点
			kmeans(InputData, k, labels, TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0), 3, KMEANS_PP_CENTERS, centers);
			ClusterPoints(centers, k);
			float valueTemp=Evaluate_K_Silhouette_Coefficient(k);
			if (valueTemp > bestValue)
			{
				bestValue = valueTemp;
				bestK = k;
			}
		}
		
		Mat centers(bestK, 1, InputData.type());    //用来存储聚类后的中心点
		kmeans(InputData, bestK, labels, TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0), 3, KMEANS_PP_CENTERS, centers);
		ClusterPoints(centers, bestK);




		//CPoint c1(centers.at<Point2f>(0, 0).x, centers.at<Point2f>(0, 0).y);
		//CPoint c2(centers.at<Point2f>(1, 0).x, centers.at<Point2f>(1, 0).y);
		//CPoint c3(centers.at<Point2f>(2, 0).x, centers.at<Point2f>(2, 0).y);


		//DiffBoundary* diffBounds = new DiffBoundary[bestK];
		DiffBoundary *diffBounds = new DiffBoundary[bestK];
		//int Diff1_X_Min = Diff1[0].x;
		//int Diff1_X_Max = Diff1[0].x;
		//int Diff1_Y_Min = Diff1[0].y;
		//int Diff1_Y_Max = Diff1[0].y;

		//int Diff2_X_Min = Diff2[0].x;
		//int Diff2_X_Max = Diff2[0].x;
		//int Diff2_Y_Min = Diff2[0].y;
		//int Diff2_Y_Max = Diff2[0].y;
		//for (int i = 0; i < Diff1.size(); i++)
		//{
		//	if (Diff1[i].x > Diff1_X_Max) Diff1_X_Max = Diff1[i].x;
		//	if (Diff1[i].x < Diff1_X_Min) Diff1_X_Min = Diff1[i].x;
		//	if (Diff1[i].y > Diff1_Y_Max) Diff1_Y_Max = Diff1[i].y;
		//	if (Diff1[i].y < Diff1_Y_Min) Diff1_Y_Min = Diff1[i].y;
		//}

		//for (int i = 0; i < Diff2.size(); i++)
		//{
		//	if (Diff2[i].x > Diff2_X_Max) Diff2_X_Max = Diff2[i].x;
		//	if (Diff2[i].x < Diff2_X_Min) Diff2_X_Min = Diff2[i].x;
		//	if (Diff2[i].y > Diff2_Y_Max) Diff2_Y_Max = Diff2[i].y;
		//	if (Diff2[i].y < Diff2_Y_Min) Diff2_Y_Min = Diff2[i].y;
		//}

		////比较函数，这里的元素类型要与vector存储的类型一致

		for (int i = 0; i < bestK; i++)//找出每个聚类的边界（舍弃三个排除噪声）
		{	
			sort(DiffPointSets[i].begin(), DiffPointSets[i].end(), sortFun_x);
			diffBounds[i].Diff_X_Min = DiffPointSets[i][3].x;
			diffBounds[i].Diff_X_Max = DiffPointSets[i][DiffPointSets[i].size() - 3].x;
			sort(DiffPointSets[i].begin(), DiffPointSets[i].end(), sortFun_y);
			diffBounds[i].Diff_Y_Min = DiffPointSets[i][3].y;
			diffBounds[i].Diff_Y_Max = DiffPointSets[i][DiffPointSets[i].size() - 3].y;
		}

		//sort(Diff1.begin(), Diff1.end(), sortFun_x);
		//Diff1_X_Min = Diff1[3].x;
		//Diff1_X_Max = Diff1[Diff1.size()-3].x;
		//sort(Diff1.begin(), Diff1.end(), sortFun_y);
		//Diff1_Y_Min = Diff1[3].y;
		//Diff1_Y_Max = Diff1[Diff1.size() - 3].y;

		//sort(Diff2.begin(), Diff2.end(), sortFun_x);
		//Diff2_X_Min = Diff2[3].x;
		//Diff2_X_Max = Diff2[Diff2.size() - 3].x;
		//sort(Diff2.begin(), Diff2.end(), sortFun_y);
		//Diff2_Y_Min = Diff2[3].y;
		//Diff2_Y_Max = Diff2[Diff2.size() - 3].y;

		//AdjustResImgWithoutResult = AdjustResImg;
		ComputerFindDiffResult.create(AdjustResImg.size(), AdjustResImg.type());

		for (int i = 0; i < AdjustResImg.cols; i++)
		{
			for (int j = 0; j < AdjustResImg.rows; j++)
			{
				ComputerFindDiffResult.at<Vec3b>(j, i)[0] = AdjustResImg.at<Vec3b>(j, i)[0];
				ComputerFindDiffResult.at<Vec3b>(j, i)[1] = AdjustResImg.at<Vec3b>(j, i)[1]; 
				ComputerFindDiffResult.at<Vec3b>(j, i)[2] = AdjustResImg.at<Vec3b>(j, i)[2];
			}
		}
		isComputerFindDiff = true;

		for (int i = 0; i < bestK; i++)//找出每个聚类的边界（舍弃三个排除噪声）
		{
			Rect rect(diffBounds[i].Diff_X_Min - 15, diffBounds[i].Diff_Y_Min - 15, 
				diffBounds[i].Diff_X_Max - diffBounds[i].Diff_X_Min + 30, diffBounds[i].Diff_Y_Max - diffBounds[i].Diff_Y_Min + 30);//左上坐标（x,y）和矩形的长(x)宽(y)
			rectangle(ComputerFindDiffResult, rect, Scalar(255, 0, 0), 2, LINE_8, 0);
		}


		//Rect rect1(Diff1_X_Min - 15, Diff1_Y_Min - 15, Diff1_X_Max - Diff1_X_Min + 30, Diff1_Y_Max - Diff1_Y_Min + 30);//左上坐标（x,y）和矩形的长(x)宽(y)
		//rectangle(ComputerFindDiffResult, rect1, Scalar(255, 0, 0), 2, LINE_8, 0);
		//Rect rect2(Diff2_X_Min - 15, Diff2_Y_Min - 15, Diff2_X_Max - Diff2_X_Min + 30, Diff2_Y_Max - Diff2_Y_Min + 30);//左上坐标（x,y）和矩形的长(x)宽(y)
		//rectangle(ComputerFindDiffResult, rect2, Scalar(255, 0, 0), 2, LINE_8, 0);
		imshow("assdad", ComputerFindDiffResult);


		//Diff1LeftUp.x = Diff1_X_Min - 15;
		//Diff1LeftUp.y = Diff1_Y_Min - 15;
		//Diff1RightDown.x = Diff1_X_Max + 15;
		//Diff1RightDown.y = Diff1_Y_Max + 15;

		//Diff2LeftUp.x = Diff2_X_Min - 15;
		//Diff2LeftUp.y = Diff2_Y_Min - 15;
		//Diff2RightDown.x = Diff2_X_Max + 15;
		//Diff2RightDown.y = Diff2_Y_Max + 15;
	}
}


void CFindDifferenceView::OnStartFindDifference()
{
	// TODO: 在此添加命令处理程序代码
	if (!isAdjust)
	{
		MessageBox(L"请先进行图像配准！");
	}
	else
	{

		ImageSub(MainImg, AdjustResImg);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		medianBlur(SubImg, SubImg, 11);
		imshow("中值滤波", SubImg);


		SobelDiv(SubImg, 80);



		Mat InputData(FindDiffBoundPoint.size(), 1, CV_32FC2), labels;
		for (int i = 0; i < InputData.rows; i++)
		{
			InputData.at<Point2f>(i, 0).x = FindDiffBoundPoint[i].x;
			InputData.at<Point2f>(i, 0).y = FindDiffBoundPoint[i].y;
		}



		int bestK = 2;
		float bestValue = -1.1;
		for (int k = 2; k < 5; k++)//测试不同的K的效果，kmeans的更改第二个参数
		{
			Mat centers(k, 1, InputData.type());    //用来存储聚类后的中心点
			kmeans(InputData, k, labels, TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0), 3, KMEANS_PP_CENTERS, centers);
			ClusterPoints(centers, k);
			float valueTemp = Evaluate_K_Silhouette_Coefficient(k);
			if (valueTemp > bestValue)
			{
				bestValue = valueTemp;
				bestK = k;
			}
		}

		Mat centers(bestK, 1, InputData.type());    //用来存储聚类后的中心点
		kmeans(InputData, bestK, labels, TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0), 3, KMEANS_PP_CENTERS, centers);
		ClusterPoints(centers, bestK);

		DiffBoundary* diffBounds = new DiffBoundary[bestK];
		isFindDiff = new bool[bestK];
		for (int i = 0; i < bestK; i++)
		{
			isFindDiff[i] = false;
		}

		for (int i = 0; i < bestK; i++)//找出每个聚类的边界（舍弃三个排除噪声）
		{
			sort(DiffPointSets[i].begin(), DiffPointSets[i].end(), sortFun_x);
			diffBounds[i].Diff_X_Min = DiffPointSets[i][3].x;
			diffBounds[i].Diff_X_Max = DiffPointSets[i][DiffPointSets[i].size() - 3].x;
			sort(DiffPointSets[i].begin(), DiffPointSets[i].end(), sortFun_y);
			diffBounds[i].Diff_Y_Min = DiffPointSets[i][3].y;
			diffBounds[i].Diff_Y_Max = DiffPointSets[i][DiffPointSets[i].size() - 3].y;
		}

		for (int i = 0; i < bestK; i++)
		{
			globalDiffBounds.push_back(diffBounds[i]);
		}


		isFindingDifference = true;
		CString tishi;
		tishi.Format(L"这两幅图有 %d 处不同，加油！", bestK);
		MessageBox(tishi);
		CDC* pDC = new CClientDC(this);
		CString LittleTip;//提示语句
		LeftDiffNum = bestK;
		LittleTip.Format(L"还剩 %d 处不同", LeftDiffNum);
		pDC->TextOutW(MainImg.cols + 10, MainImg.rows + 20, LittleTip);

		if (!isComputerFindDiff)
		{
			for (int i = 0; i < AdjustResImg.cols; i++)
			{
				for (int j = 0; j < AdjustResImg.rows; j++)
				{
					int B = AdjustResImg.at<Vec3b>(j, i)[0];
					int G = AdjustResImg.at<Vec3b>(j, i)[1];
					int R = AdjustResImg.at<Vec3b>(j, i)[2];
					pDC->SetPixel(i + MainImg.cols + 100, j, RGB(R, G, B));
				}
			}
		}
	}
}


void CFindDifferenceView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CDC* pDC = new CClientDC(this);
	if (isFindingDifference)
	{
		for (int i = 0; i < DiffPointSets.size(); i++)
		{
			if (point.x > globalDiffBounds[i].Diff_X_Min && point.x<globalDiffBounds[i].Diff_X_Max && point.y>globalDiffBounds[i].Diff_Y_Min && point.y < globalDiffBounds[i].Diff_Y_Max
				|| point.x - MainImg.cols - 100 > globalDiffBounds[i].Diff_X_Min && point.x - MainImg.cols - 100 < globalDiffBounds[i].Diff_X_Max && point.y > globalDiffBounds[i].Diff_Y_Min && point.y < globalDiffBounds[i].Diff_Y_Max)
			{
				if (!isFindDiff[i])
				{
					MessageBox(L"找到不同了！");
					LeftDiffNum--;
					CPen* pen = new CPen(PS_SOLID, 3, RGB(0, 0, 255));
					pDC->SelectObject(pen);
					pDC->MoveTo(globalDiffBounds[i].Diff_X_Min, globalDiffBounds[i].Diff_Y_Min);
					pDC->LineTo(globalDiffBounds[i].Diff_X_Min, globalDiffBounds[i].Diff_Y_Max);
					pDC->LineTo(globalDiffBounds[i].Diff_X_Max, globalDiffBounds[i].Diff_Y_Max);
					pDC->LineTo(globalDiffBounds[i].Diff_X_Max, globalDiffBounds[i].Diff_Y_Min);
					pDC->LineTo(globalDiffBounds[i].Diff_X_Min, globalDiffBounds[i].Diff_Y_Min);

					pDC->MoveTo(globalDiffBounds[i].Diff_X_Min + MainImg.cols + 100, globalDiffBounds[i].Diff_Y_Min);
					pDC->LineTo(globalDiffBounds[i].Diff_X_Min + MainImg.cols + 100, globalDiffBounds[i].Diff_Y_Max);
					pDC->LineTo(globalDiffBounds[i].Diff_X_Max + MainImg.cols + 100, globalDiffBounds[i].Diff_Y_Max);
					pDC->LineTo(globalDiffBounds[i].Diff_X_Max + MainImg.cols + 100, globalDiffBounds[i].Diff_Y_Min);
					pDC->LineTo(globalDiffBounds[i].Diff_X_Min + MainImg.cols + 100, globalDiffBounds[i].Diff_Y_Min);
					CString LittleTip;//提示语句
					LittleTip.Format(L"还剩 %d 处不同", LeftDiffNum);
					pDC->TextOutW(MainImg.cols + 10, MainImg.rows + 20, LittleTip);
					if (LeftDiffNum == 0)
					{
						MessageBox(L"恭喜你，全部找到了！");
					}
					isFindDiff[i] = true;

				}
				else
				{
					MessageBox(L"这里没有不同！再找找看！");
				}
			}
		}
}

		////找第一个
		//if (point.x > Diff1LeftUp.x && point.x<Diff1RightDown.x && point.y>Diff1LeftUp.y && point.y < Diff1RightDown.y
		//	|| point.x-MainImg.cols-100 > Diff1LeftUp.x && point.x - MainImg.cols - 100 <Diff1RightDown.x && point.y>Diff1LeftUp.y && point.y < Diff1RightDown.y)
		//{
		//	if (!isFindDiff1)
		//	{
		//		MessageBox(L"找到不同了！");
		//		LeftDiffNum--;
		//		CPen* pen = new CPen(PS_SOLID, 3, RGB(0, 0, 255));
		//		pDC->SelectObject(pen);
		//		pDC->MoveTo(Diff1LeftUp.x, Diff1LeftUp.y);
		//		pDC->LineTo(Diff1LeftUp.x, Diff1RightDown.y);
		//		pDC->LineTo(Diff1RightDown.x, Diff1RightDown.y);
		//		pDC->LineTo(Diff1RightDown.x, Diff1LeftUp.y);
		//		pDC->LineTo(Diff1LeftUp.x, Diff1LeftUp.y);

		//		pDC->MoveTo(Diff1LeftUp.x+MainImg.cols+100, Diff1LeftUp.y);
		//		pDC->LineTo(Diff1LeftUp.x + MainImg.cols + 100, Diff1RightDown.y);
		//		pDC->LineTo(Diff1RightDown.x + MainImg.cols + 100, Diff1RightDown.y);
		//		pDC->LineTo(Diff1RightDown.x + MainImg.cols + 100, Diff1LeftUp.y);
		//		pDC->LineTo(Diff1LeftUp.x + MainImg.cols + 100, Diff1LeftUp.y);
		//		CString LittleTip;//提示语句
		//		LittleTip.Format(L"还剩 %d 处不同", LeftDiffNum);
		//		pDC->TextOutW(MainImg.cols + 10, MainImg.rows + 20, LittleTip);
		//		if (LeftDiffNum == 0)
		//		{
		//			MessageBox(L"恭喜你，全部找到了！");
		//		}
		//		isFindDiff1 = true;
		//	}
		//}

		////找第二个
		//else if (point.x > Diff2LeftUp.x && point.x<Diff2RightDown.x && point.y>Diff2LeftUp.y && point.y < Diff2RightDown.y
		//	|| point.x - MainImg.cols - 100 > Diff2LeftUp.x && point.x - MainImg.cols - 100 < Diff2RightDown.x && point.y > Diff2LeftUp.y && point.y < Diff2RightDown.y)
		//{
		//	if (!isFindDiff2)
		//	{
		//		MessageBox(L"找到不同了！");
		//		LeftDiffNum--;
		//		CPen* pen = new CPen(PS_SOLID, 3, RGB(0, 0, 255));
		//		pDC->SelectObject(pen);
		//		pDC->MoveTo(Diff2LeftUp.x, Diff2LeftUp.y);
		//		pDC->LineTo(Diff2LeftUp.x, Diff2RightDown.y);
		//		pDC->LineTo(Diff2RightDown.x, Diff2RightDown.y);
		//		pDC->LineTo(Diff2RightDown.x, Diff2LeftUp.y);
		//		pDC->LineTo(Diff2LeftUp.x, Diff2LeftUp.y);

		//		pDC->MoveTo(Diff2LeftUp.x + MainImg.cols + 100, Diff2LeftUp.y);
		//		pDC->LineTo(Diff2LeftUp.x + MainImg.cols + 100, Diff2RightDown.y);
		//		pDC->LineTo(Diff2RightDown.x + MainImg.cols + 100, Diff2RightDown.y);
		//		pDC->LineTo(Diff2RightDown.x + MainImg.cols + 100, Diff2LeftUp.y);
		//		pDC->LineTo(Diff2LeftUp.x + MainImg.cols + 100, Diff2LeftUp.y);
		//		CString LittleTip;//提示语句
		//		LittleTip.Format(L"还剩 %d 处不同", LeftDiffNum);
		//		pDC->TextOutW(MainImg.cols + 10, MainImg.rows + 20, LittleTip);
		//		if (LeftDiffNum == 0)
		//		{
		//			MessageBox(L"恭喜你，全部找到了！");
		//		}
		//		isFindDiff2 = true;
		//	}

	

	CView::OnLButtonUp(nFlags, point);
}
