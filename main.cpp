
#include <psl.h>
using namespace wfl;
using namespace psl;

static void Coding(WaveField& sub);
static void LoadWfAndSaveAsBmp(WaveField& wf, const WFL_RECT& rect, const char* name);
static void LoadWfAndSaveAsBmp(WaveField& wf, const char* name);
static const char* Str(const char* format, ...);

static const double gRatio = 1.0;				 // ここでスケール倍率を決める
static const Point	gRefPos(0.0, -50.0e-3, -200e-3); // ここで参照光の位置を決める
static const Point	gEyePos[5] =
{ 
	Point(0, 0, 200e-3),		// 正面
	Point(0, 20e-3, 200e-3),	// 上
	Point(0, -20e-3, 200e-3),	// 下
	Point(-20e-3, 0, 200e-3),	// 左
	Point(20e-3, 0, 200e-3)		// 右
};

int main()
{
	Start();    wfl::SetNumThreads();								//最大限にプロセッサコアを用いる

	// モデルの設定
	double objectSize = 20e-3 * gRatio;								//物体の実サイズ
	Point  objectPos(0.0, 0.0, -150e-3); objectPos *= gRatio;		//物体の中心位置

	// 物体モデルファイルの読み込みと設定
	IndexedFaceSet model;
	{
		model.LoadMqo("model\\torii3.mqo");
		model.Localize();											//物体を一時的に原点に置く
		model.SetWidth(objectSize);									//物体サイズ(幅)を設定
		model += objectPos;											//物体位置を設定
		model.AutoNormalVector();									//グーローシェーディングのための準備
	}

	// フレームバッファの設定
	double	  rgbLambda[] = { 630e-9, 540e-9, 460e-9 };				//フルカラー用に用いる波長(R, G, B)
	double	  px = 0.8e-6,				py = 0.8e-6;				//サンプリング間隔
	int		  nx = 64 * 1024 * gRatio,  ny = 64 * 1024 * gRatio;	//サンプリング数
	WaveField frame(nx, ny, px, py);

	// シェーダーの設定
	Vector light(-0.408, -0.408, -0.816);							//照明光の方向 (この方向はMqoのデフォルト)
	double gamma = 0.01;											//補正制限値

	// 参照光の設定
	WaveField ref(nx, ny, px, py);
	Point	  refPos = gRefPos; refPos *= gRatio;

	// 正面、上下左右の各視点を計算
	size_t eyePosEnd = _countof(gEyePos);
	for (int eyePosIdx = 0; eyePosIdx < eyePosEnd; ++eyePosIdx)
	{
		// 結像再生の設定
		ImagingViewer view;
		{
			Point eyePos = gEyePos[eyePosIdx]; eyePos *= gRatio; //視点位置
			view.SetOrigin(eyePos);
			view.SetImagingDistance(24e-3);
			view.SetPupilDiameter(6e-3);
			view.SetPx(px);
			view.SetPy(py);
			view.Init();
		}

		// 結像再生像の作成に用いる
		ColorImage image(view.GetNx(), view.GetNy());

		// 物体光波を瞳まで伝搬するシミュレーション
		for (int RGBcounter = 1; RGBcounter <= 3; ++RGBcounter)
		{
			Printf("\n\n%d色目の計算開始\n", RGBcounter);

			frame.Clear();
			frame.SetWavelength(rgbLambda[RGBcounter - 1]);				//各色の波長設定
			frame.SetOrigin(objectPos);									//フレームバッファの位置は物体モデルの中心

			TfbLambertShading shading(gamma, light, (ColorMode)RGBcounter);//ランバートシェーダ

			// SurfaceBuilderの設定
			SurfaceBuilder sb(frame);									//帯域制限のため，サンプル数とサンプル間隔等を正しく設定
			{
				sb.SetBandLimitMethod(3);								//帯域制限をレベル3にする．
				sb.SetCenter(Point(0, 0, 0));							//帯域制限のため，ホログラムの中心位置(0,0,0)を設定
				sb.SetDiffractionRatio(0.9);							//回折率設定
				sb.SetCullingRate(0.6);									//カリング率設定
				sb.SetShader(shading);									//上で用意したシェーダーオブジェクトを組み込む
			}

			// 物体モデルmodelからの光波を計算してフレームバッファに加算
			sb.AddObjectFieldSb(frame, model, 1);						//スイッチバック法で物体光波計算

			// 物体面の光波分布
			switch (RGBcounter)
			{
			case RED:	frame.SaveAsWf(Str("output\\%d\\object-R.wf", eyePosIdx)); break;
			case GREEN: frame.SaveAsWf(Str("output\\%d\\object-G.wf", eyePosIdx)); break;
			case BLUE:	frame.SaveAsWf(Str("output\\%d\\object-B.wf", eyePosIdx)); break;
			}

			// フレネルホログラム用の物体光波を得るためホログラムの位置(z = 0)まで伝搬計算する
			frame.AsmProp(-frame.GetOrigin().GetZ());

			// ホログラム面(z = 0)で得られる物体光波の保存
			switch (RGBcounter)
			{
			case RED:	frame.SaveAsWf(Str("output\\%d\\frame-R.wf", eyePosIdx)); break;
			case GREEN: frame.SaveAsWf(Str("output\\%d\\frame-G.wf", eyePosIdx)); break;
			case BLUE:	frame.SaveAsWf(Str("output\\%d\\frame-B.wf", eyePosIdx)); break;
			}

			// 球面波を参照光として設定
			ref.Clear();
			ref.SetWavelength(rgbLambda[RGBcounter - 1]);
			ref.AddSphericalWave(refPos);
			ref.ConvToConjugate();

			// 二値化干渉縞の作成
			frame *= ref;
			Coding(frame);

			// 干渉縞の保存
			switch (RGBcounter)
			{
			case RED:	frame.SaveAsWf(Str("output\\%d\\fringe-R.wf", eyePosIdx)); break;
			case GREEN: frame.SaveAsWf(Str("output\\%d\\fringe-G.wf", eyePosIdx)); break;
			case BLUE:	frame.SaveAsWf(Str("output\\%d\\fringe-B.wf", eyePosIdx)); break;
			}

			//-------------------------------------------------------
			// ここまでが干渉縞の記録
			// ここからが結像再生のシミュレーション
			//-------------------------------------------------------

			// 干渉縞に再生照明光を当てる
			ref.ConvToConjugate();
			frame *= ref;

			// カラーの結像再生像を作成
			view.SpectralView(frame, objectPos, image);
		}

		// 物体光波、干渉縞の画像化をする際の設定
		double	 wDiv2 = view.GetWidth() / 2, hDiv2 = view.GetHeight() / 2;
		WFL_RECT rect(-wDiv2, +hDiv2, +wDiv2, -hDiv2);

		// 物体光のシミュレーション結果
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\object-R", eyePosIdx));
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\object-G", eyePosIdx));
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\object-B", eyePosIdx));

		// 干渉縞のシミュレーション結果
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\fringe-R", eyePosIdx));
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\fringe-G", eyePosIdx));
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\fringe-B", eyePosIdx));

		// 結像再生像のシミュレーション結果
		image.NormalizeXYZ();
		image.SaveAsBmpSRGB(Str("output\\%d\\result.bmp", eyePosIdx));
	}
}

void Coding(WaveField& fringe)
{
	for (int iy = 0; iy < fringe.GetNy(); iy++)
	{
		for (int ix = 0; ix < fringe.GetNx(); ix++)
		{
			double val = fringe.GetPixel(ix, iy).GetReal();
			double amplitude;
			if (val > 0)
				amplitude = 1.0;
			else
				amplitude = 0.0;
			fringe.SetPixel(ix, iy, Complex(amplitude, 0.0));
		}
	}
}

void LoadWfAndSaveAsBmp(WaveField& wf, const WFL_RECT& rect, const char* name)
{
	wf.Clear();
	wf.SetWindowMax();

	char fname[256];
	sprintf_s(fname, "%s.wf", name);
	wf.LoadWf(fname);
	sprintf_s(fname, "%s.bmp", name);
	wf.Normalize();
	wf.SetWindow(rect);
	wf.SaveAsBmp(fname, AMPLITUDE);
}

void LoadWfAndSaveAsBmp(WaveField& wf, const char* name)
{
	wf.Clear();
	wf.SetWindowMax();
	
	char fname[256];
	sprintf_s(fname, "%s.wf", name);
	wf.LoadWf(fname);
	sprintf_s(fname, "%s.bmp", name);
	wf.Normalize();
	wf.SaveAsBmp(fname, AMPLITUDE);
}

#include <stdarg.h>
const char* Str(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	static char fname[256];
	vsprintf_s(fname, format, args);
	printf("\n確認：%s\n", fname);
	va_end(args);
	return fname;
}
