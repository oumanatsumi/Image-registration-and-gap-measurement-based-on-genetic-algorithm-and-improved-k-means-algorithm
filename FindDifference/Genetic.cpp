// Genetic.cpp: 实现文件
//

#include "pch.h"
#include "FindDifference.h"
#include "Genetic.h"
#include "afxdialogex.h"
#include "afxdialogex.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include	<time.h>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2\imgproc\types_c.h>

// Genetic 对话框
using namespace std;
using namespace cv;

#define POPSIZE 20               /* 种群大小 */
//#define MAXGENS 10           /* 迭代次数最大值*/
#define NVARS 2                 /* 问题规模（变量个数） */
#define PXOVER 0.7      /* 交配的概率*/
#define PMUTATION 0.07        /*  变异概率*/
#define TRUE 1
#define FALSE 0
#define PI 3.1415926535979323846

int generation;                  /* current generation no. */
int cur_best;                    /* best individual */
FILE* galog;                     /* an output(1) file */
FILE* output;                     /* an output(1) file */
cv::Mat img1;//正确的图
cv::Mat img2;//需要旋转的图
cv::Mat imgTemp;
CString mes = L"";
double lbound[NVARS] = { -45 ,1 };
double ubound[NVARS] = { 45, 1.5 };
IMPLEMENT_DYNAMIC(Genetic, CDialogEx)

Genetic::Genetic(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_Genetic, pParent)
	, IterationNum(50)
	, testAngle(999)
	, testRatio(999)
{

}

Genetic::~Genetic()
{
}

void Genetic::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, IterationNum);
	DDX_Text(pDX, IDC_EDIT2, testAngle);
	DDX_Text(pDX, IDC_EDIT3, testRatio);
}


BEGIN_MESSAGE_MAP(Genetic, CDialogEx)
	ON_BN_CLICKED(IDOK, &Genetic::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_Test, &Genetic::OnBnClickedButtonTest)
END_MESSAGE_MAP()


// Genetic 消息处理程序
struct genotype /* genotype (GT), a member of the population */
{
	double gene[NVARS];        /* a string of variables */
	double fitness;            /* GT's fitness */
	double upper[NVARS];       /* GT's variables upper bound */
	double lower[NVARS];       /* GT's variables lower bound */
	double rfitness;           /* relative fitness适应值概率密度 */
	double cfitness;           /* cumulative fitness累加分布 */
};

struct genotype population[POPSIZE + 1];    /* population */
struct genotype newpopulation[POPSIZE + 1]; /* new population; */


void initialize(void);
double randval(double, double);
void evaluate(void);
void keep_the_best(void);
void elitist(void);
void select(void);
void crossover(void);
void Xover(int, int);
void swap(double*, double*);
void mutate(void);
void report(void);


double randval(double low, double high)
{
	//srand(time(NULL));//利用时间产生不同的种子，使得产生的随机数列不同
	double val;
	val = low + (high - low) * (rand() * 1.0 / RAND_MAX);
	return(val);
}
void initialize(void)
{
	//FILE *infile;
	int i, j;

	/*lbound[0] = -3.0;
	lbound[1] = 4.1;
	ubound[0] = 12.1;
	ubound[1] =5.8 ;*/

	/*if ((infile = fopen("gadata.txt", "r")) == NULL)
	{
	fprintf(galog, "\nCannot open input file!\n");
	exit(1);
	}

	remove("output.dat");*/
	/* initialize variables within the bounds */

	for (i = 0; i < NVARS; i++)
	{
		/*fscanf_s(infile, "%lf", &lbound);
		fscanf_s(infile, "%lf", &ubound);*/


		for (j = 0; j < POPSIZE; j++)
		{
			population[j].fitness = 0;
			population[j].rfitness = 0;
			population[j].cfitness = 0;
			population[j].lower[i] = lbound[i];
			population[j].upper[i] = ubound[i];
			population[j].gene[i] = randval(population[j].lower[i], population[j].upper[i]);
		}
		population[POPSIZE].fitness = -999999.9;
	}
	//population[POPSIZE].fitness=0;
	//fclose(infile);
}

/***********************************************************/
/* Random value generator: Generates a value within bounds */
/***********************************************************/


cv::Mat ImageRotate(cv::Mat oldimg, double roll)
{
	CPoint center(oldimg.cols / 2, oldimg.rows / 2);
	cv::Mat Rx(3, 3, CV_32FC1);

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

	cv::Mat rotated_ROI;
	//cv::warpAffine(face_img, rotated_ROI, rot_mat, face_img.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar::all(0));
	warpPerspective(oldimg, rotated_ROI, Rx, cv::Size(oldimg.cols, oldimg.rows));
	return rotated_ROI;
}

cv::Mat ImageScale(Mat oldimg, float ScaleNum)
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
}


//double CalcImgOffset(double angle)
//{
//	imgTemp = ImageRotate(img2, angle);
//	 //int x_offset = 0;
//	 //int y_offset = 0;
//	 //for (int i = 0; i < img2.cols; i++)
//	 //{
//		// for (int j = 0; j < img2.rows; j++)
//		// {
//		//	 if (img2.at<cv::Vec3b>(j, i)[0] != 0 || img2.at<cv::Vec3b>(j, i)[1] != 0 || img2.at<cv::Vec3b>(j, i)[2] != 0)
//		//	 {
//		//		 x_offset = i;
//		//		 y_offset = j;
//		//			 break; break;
//		//	 }
//		// }
//	 //}
//	double accumulate = 0.0;
//	for (int i = 0; i < img1.cols; i++)
//	{
//		for (int j = 0; j < img1.rows; j++)
//		{
//			//accumulate += abs(img1.at<cv::Vec3b>(j, i)[0] - img2.at<cv::Vec3b>(j+ y_offset, i+ x_offset)[0]) / 256.0;
//			//accumulate += abs(img1.at<cv::Vec3b>(j, i)[1] - img2.at<cv::Vec3b>(j + y_offset, i + x_offset)[1]) / 256.0;
//			//accumulate += abs(img1.at<cv::Vec3b>(j, i)[2] - img2.at<cv::Vec3b>(j + y_offset, i + x_offset)[2]) / 256.0;
//			/*accumulate += abs(img1.at<uchar>(j, i)- imgTemp.at<uchar>(j , i));*/
//			accumulate += abs(img1.at<cv::Vec3b>(j, i)[0] - imgTemp.at<cv::Vec3b>(j, i)[0]);
//			accumulate += abs(img1.at<cv::Vec3b>(j, i)[1] - imgTemp.at<cv::Vec3b>(j , i )[1]);
//			accumulate += abs(img1.at<cv::Vec3b>(j, i)[2] - imgTemp.at<cv::Vec3b>(j , i)[2]);
//		}
//	}
//	return accumulate;
//}

//均值Hash算法  
string HashValue(Mat& src, int cols, int rows)
{
	string rst(cols * rows, '\0');
	Mat img;
	if (src.channels() == 3)
		cvtColor(src, img, CV_BGR2GRAY);
	else
		img = src.clone();
	resize(img, img, Size(cols, rows));
	//imshow("ass", img);
	//waitKey(0);
	/* 第二步，简化色彩(Color Reduce)。
	   将缩小后的图片，转为64级灰度。*/

	uchar* pData;
	for (int i = 0; i < img.rows; i++)
	{
		pData = img.ptr<uchar>(i);
		for (int j = 0; j < img.cols; j++)
		{
			pData[j] = pData[j] / 4;
		}
	}

	/* 第三步，计算平均值。
   计算所有64个像素的灰度平均值。*/
	int average = mean(img).val[0];

	/* 第四步，比较像素的灰度。
 将每个像素的灰度，与平均值进行比较。大于或等于平均值记为1,小于平均值记为0*/
	Mat mask = (img >= (uchar)average);

	/* 第五步，计算哈希值。*/
	int index = 0;
	for (int i = 0; i < mask.rows; i++)
	{
		pData = mask.ptr<uchar>(i);
		for (int j = 0; j < mask.cols; j++)
		{
			if (pData[j] == 0)
				rst[index++] = '0';
			else
				rst[index++] = '1';
		}
	}
	return rst;
}


//汉明距离计算  
int HanmingDistance(string& str1, string& str2, int cols, int rows)
{
	if ((str1.size() != cols * rows) || (str2.size() != cols * rows))
		return -1;
	int difference = 0;
	for (int i = 0; i < cols * rows; i++)
	{
		if (str1[i] != str2[i])
			difference++;
	}
	return difference;
}



double CalcImgOffset(double angle, double ratio)
{
	imgTemp = ImageRotate(img2, angle);
	imgTemp = ImageScale(imgTemp, ratio);
	string s1 = HashValue(img1, img1.cols, img1.rows);
	string s2 = HashValue(imgTemp, imgTemp.cols, imgTemp.rows);
	double dis;
	dis = HanmingDistance(s1, s2, img1.cols, img1.rows);
	return dis;
}


/*************************************************************/
/* Evaluation function: This takes a user defined function.  */
/* Each time this is changed, the code has to be recompiled. */
/* The current function is:  x[1]^2-x[1]*x[2]+x[3]           */
/*************************************************************/

void evaluate(void)
{
	/*if ((output = fopen("output.dat", "a")) == NULL)
	{
	exit(1);
	}*/
	int mem;
	int i;
	double x[NVARS + 1];
	for (mem = 0; mem < POPSIZE; mem++)
	{
		for (i = 0; i < NVARS; i++)
		{
			x[i + 1] = population[mem].gene[i];
		}
		double accu = CalcImgOffset(x[1], x[2]);
		population[mem].fitness = -accu;
		CString temp;
		temp.Format(L"fitness:%lf  ", population[mem].fitness);
		mes += temp;
		temp.Format(L"angle:%lf  ", population[mem].gene[0]);
		mes += temp;
	}
}

/***************************************************************/
/* Keep_the_best function: This function keeps track of the    */
/* best member of the population. Note that the last entry in  */
/* the array Population holds a copy of the best individual    */
/***************************************************************/

void keep_the_best()
{
	int mem;
	int i;
	cur_best = 0; /* stores the index of the best individual */

	for (mem = 0; mem < POPSIZE; mem++)
	{
		if (population[mem].fitness > population[POPSIZE].fitness)
		{
			cur_best = mem;
			population[POPSIZE].fitness = population[mem].fitness;
		}
	}
	/* once the best member in the population is found, copy the genes */
	for (i = 0; i < NVARS; i++)
		population[POPSIZE].gene[i] = population[cur_best].gene[i];
}

/****************************************************************/
/* Elitist function: The best member of the previous generation */
/* is stored as the last in the array. If the best member of    */
/* the current generation is worse then the best member of the  */
/* previous generation, the latter one would replace the worst  */
/* member of the current population                             */
/****************************************************************/

void elitist()
{
	int i;
	double best, worst;             /* best and worst fitness values */
	int best_mem, worst_mem; /* indexes of the best and worst member */

	best = population[0].fitness;
	worst = population[0].fitness;
	for (i = 0; i < POPSIZE - 1; ++i)
	{
		if (population[i].fitness > population[i + 1].fitness)
		{
			if (population[i].fitness >= best)
			{
				best = population[i].fitness;
				best_mem = i;
			}
			if (population[i + 1].fitness <= worst)
			{
				worst = population[i + 1].fitness;
				worst_mem = i + 1;
			}
		}
		else
		{
			if (population[i].fitness <= worst)
			{
				worst = population[i].fitness;
				worst_mem = i;
			}
			if (population[i + 1].fitness >= best)
			{
				best = population[i + 1].fitness;
				best_mem = i + 1;
			}
		}
	}
	/* if best individual from the new population is better than */
	/* the best individual from the previous population, then    */
	/* copy the best from the new population; else replace the   */
	/* worst individual from the current population with the     */
	/* best one from the previous generation                     */

	if (best >= population[POPSIZE].fitness)
	{
		for (i = 0; i < NVARS; i++)
			population[POPSIZE].gene[i] = population[best_mem].gene[i];
		population[POPSIZE].fitness = population[best_mem].fitness;
	}
	else
	{
		for (i = 0; i < NVARS; i++)
			population[worst_mem].gene[i] = population[POPSIZE].gene[i];
		population[worst_mem].fitness = population[POPSIZE].fitness;
	}
}
/**************************************************************/
/* Selection function: Standard proportional selection for    */
/* maximization problems incorporating elitist model - makes  */
/* sure that the best member survives                         */
/**************************************************************/

void select(void)
{
	int mem;
	int i;
	int j;
	int k;
	double sum = 0;
	double p;

	/* find total fitness of the population */
	for (mem = 0; mem < POPSIZE; mem++)
	{
		sum += population[mem].fitness;
	}

	/* calculate relative fitness */
	for (mem = 0; mem < POPSIZE; mem++)
	{
		population[mem].rfitness = population[mem].fitness / sum;
	}
	population[0].cfitness = population[0].rfitness;

	/* calculate cumulative fitness */
	for (mem = 1; mem < POPSIZE; mem++)
	{
		population[mem].cfitness = population[mem - 1].cfitness +
			population[mem].rfitness;
	}

	/* finally select survivors using cumulative fitness. */

	for (i = 0; i < POPSIZE; i++)
	{
		p = rand() % 1000 / 1000.0;
		if (p < population[0].cfitness)
			newpopulation[i] = population[0];
		else
		{
			for (j = 0; j < POPSIZE; j++)
				if (p >= population[j].cfitness &&
					p < population[j + 1].cfitness)
					newpopulation[i] = population[j + 1];
		}
	}
	/* once a new population is created, copy it back */

	for (i = 0; i < POPSIZE; i++)
		population[i] = newpopulation[i];
}

/***************************************************************/
/* Crossover selection: selects two parents that take part in  */
/* the crossover. Implements a single point crossover          */
/***************************************************************/

void crossover(void)
{
	int  mem, one;
	int first = 0; /* count of the number of members chosen */
	double x;

	for (mem = 0; mem < POPSIZE; ++mem)
	{
		x = rand() % 1000 / 1000.0;
		if (x < PXOVER)
		{
			++first;
			if (first % 2 == 0)
				Xover(one, mem);
			else
				one = mem;
		}
	}
}
/**************************************************************/
/* Crossover: performs crossover of the two selected parents. */
/**************************************************************/

void Xover(int one, int two)
{
	int i;
	int point; /* crossover point */

			   /* select crossover point */
	if (NVARS > 1)
	{
		if (NVARS == 2)
			point = 1;
		else
			point = (rand() % (NVARS - 1)) + 1;

		for (i = 0; i < point; i++)
			swap(&population[one].gene[i], &population[two].gene[i]);

	}
}

/*************************************************************/
/* Swap: A swap procedure that helps in swapping 2 variables */
/*************************************************************/

void swap(double* x, double* y)
{
	double temp;

	temp = *x;
	*x = *y;
	*y = temp;

}

/**************************************************************/
/* Mutation: Random uniform mutation. A variable selected for */
/* mutation is replaced by a random value between lower and   */
/* upper bounds of this variable                              */
/**************************************************************/

void mutate(void)
{
	int i, j;
	double lbound, hbound;
	double x;

	for (i = 0; i < POPSIZE; i++)
		for (j = 0; j < NVARS; j++)
		{
			x = rand() % 1000 / 1000.0;
			if (x < PMUTATION)
			{
				/* find the bounds on the variable to be mutated */
				lbound = population[i].lower[j];
				hbound = population[i].upper[j];
				population[i].gene[j] = randval(lbound, hbound);
			}
		}
}

/***************************************************************/
/* Report function: Reports progress of the simulation. Data   */
/* dumped into the  output file are separated by commas        */
/***************************************************************/

void report(void)
{
	int i;
	double best_val;            /* best population fitness */
	double avg;                 /* avg population fitness */
	double stddev;              /* std. deviation of population fitness */
	double sum_square;          /* sum of square for std. calc */
	double square_sum;          /* square of sum for std. calc */
	double sum;                 /* total population fitness */

	sum = 0.0;
	sum_square = 0.0;

	for (i = 0; i < POPSIZE; i++)
	{
		sum += population[i].fitness;
		sum_square += population[i].fitness * population[i].fitness;
	}

	avg = sum / (double)POPSIZE;
	square_sum = avg * avg * POPSIZE;
	stddev = sqrt((sum_square - square_sum) / (POPSIZE - 1));
	best_val = population[POPSIZE].fitness;

	//printf("\n%5d,      %6.9f, %6.9f, %6.9f ", generation,best_val, avg, stddev);
}


ImageTrans Genetic::Calc(cv::Mat correctImg, cv::Mat adjustImg)
{
	img1 = correctImg;
	img2 = adjustImg;
	int i;
	double sum = 0;
	CString temp;
		srand(time(NULL));//利用时间产生不同的种子，使得产生的随机数列不同
		generation = 0;
		initialize();
		evaluate();
		keep_the_best();

		while (generation < IterationNum)
		{
			generation++;
			select();
			crossover();
			mutate();
			report();
			evaluate();
			mes += L"\r\n";
			elitist();
		}

		for (i = 0; i < NVARS; i++)
		{
			srand(time(NULL));//利用时间产生不同的种子，使得产生的随机数列不同
			temp.Format(L"\n var(%d) = %3.9f\n", i, population[POPSIZE].gene[i]);
		}
		srand(time(NULL));//利用时间产生不同的种子，使得产生的随机数列不同
		//temp.Format(L"第%d次循环;适应值=%3.9f\n", t, population[POPSIZE].fitness);
		sum += population[POPSIZE].fitness;
		/*population[POPSIZE].fitness = 0;*/

	double angle = population[POPSIZE].gene[0];
	double ratio = population[POPSIZE].gene[1];
	CString Temp;
	CStdioFile File;
	File.Open(_T("mes.txt"), CFile::modeReadWrite | CFile::modeNoTruncate | CFile::modeCreate);
	File.SeekToEnd();
	Temp = _T("\r\n-----------------------BEGIN------------------------\r\n");
	Temp += mes;
	Temp += _T("\r\n-----------------------END------------------------\r\n");
	File.WriteString(Temp);
	File.Close();

	CString fit;
	fit.Format(L"最佳适应值:%lf", sum);
	MessageBox(fit);

	ImageTrans res;
	res.angle = angle;
	res.scaleRatio = ratio;
	return res;
}


void Genetic::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(true);
	if (testAngle != 999 & testRatio != 999)
	{
		lbound[0] = testAngle - 1.5;
		ubound[0] = testAngle + 1.5;
		lbound[1] = testRatio - 0.2;
		if (testRatio - 0.2<1)
		{
			lbound[1] = 1;
		}
		ubound[1] = testRatio + 0.1;
	}
	
	CDialogEx::OnOK();
}


void Genetic::OnBnClickedButtonTest()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(true);
	if (testAngle > 90 || testAngle < -90 || testRatio < 1 || testRatio>1.8)
	{
		MessageBox(L"请输入正确的旋转角度和缩放比例！\r\n角度在-90°~90°之间，比例在1.0~1.8之间！");
	}
	else
	{
		extern Mat SecondImg;
		Mat temp1;
		Mat testImg;
		//testImg.create(img2.size(), img2.type());
		temp1 = ImageRotate(SecondImg, testAngle);
		testImg = ImageScale(temp1, testRatio);
		imshow("测试结果（供参考）", testImg);
	}
}
