
#include <psl.h>
using namespace wfl;
using namespace psl;

static void			Coding(WaveField& sub);
static const char*	Str(const char* format, ...);

static const double gScale = 1.0;				 // ここでスケール倍率を決める
static const Point	gRefPos(0.0, -40.0e-3, -200e-3); // ここで参照光の位置を決める

// ここで視点の位置を決める
static const Point	gEyePos[5] =
{ 
	Point(0, 0, 200e-3),		// 正面
	Point(0, 40e-3, 200e-3),	// 上
	Point(0, -40e-3, 200e-3),	// 下
	Point(-40e-3, 0, 200e-3),	// 左
	Point(40e-3, 0, 200e-3)		// 右
};

int main()
{
	Start();    wfl::SetNumThreads();								//最大限にプロセッサコアを用いる

	// モデルの設定
	double objectSize = 20e-3 * gScale;								//物体の実サイズ
	Point  objectPos(0.0, 0.0, -150e-3); objectPos *= gScale;		//物体の中心位置

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
	int		  nx = 64 * 1024 * gScale,  ny = 64 * 1024 * gScale;	//サンプリング数
	WaveField frame(nx, ny, px, py);

	// シェーダーの設定
	Vector light(-0.408, -0.408, -0.816);							//照明光の方向 (この方向はMqoのデフォルト)
	double gamma = 0.01;											//補正制限値

	// 参照光の設定
	WaveField ref(nx, ny, px, py);
	Point	  refPos = gRefPos; refPos *= gScale;

	// 正面、上下左右の各視点を計算
	int eyePosEnd = _countof(gEyePos);
	for (int eyePosIdx = 0; eyePosIdx < eyePosEnd; ++eyePosIdx)
	{
		Printf("\n\n%d番目の視点の計算開始\n", eyePosIdx);

		// 結像再生の設定
		ImagingViewer view;
		{
			Point eyePos = gEyePos[eyePosIdx]; eyePos *= gScale;	//視点位置
			view.SetOrigin(eyePos);
			view.SetImagingDistance(24e-3);
			view.SetPupilDiameter(6e-3);
			view.SetPx(px);
			view.SetPy(py);
			view.Init();
		}

		// 物体光波のシミュレーション画像の作成に用いる
		ColorImage object(view.GetNx(), view.GetNy());

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

			//-----------------------------------------
			// (1) 物体光波シミュレーション再生像を計算
			//-----------------------------------------
			view.SpectralView(frame, objectPos, object);

			// フレネルホログラム用の物体光波を得るためホログラムの位置(z = 0)まで伝搬計算する
			frame.AsmProp(-frame.GetOrigin().GetZ());

			// 球面波を参照光として設定
			ref.Clear();
			ref.SetWavelength(rgbLambda[RGBcounter - 1]);
			ref.AddSphericalWave(refPos);

			// 二値化干渉縞の作成
			frame *= ref.ConvToConjugate();
			Coding(frame);

			//-------------------------------------------------------
			// ここまでが干渉縞の記録
			// ここからが干渉縞シミュレーション
			//-------------------------------------------------------

			// 干渉縞に再生照明光を当てる
			frame *= ref.ConvToConjugate();

			//---------------------------------------
			// (2) 干渉縞シミュレーション再生像を計算
			//---------------------------------------
			view.SpectralView(frame, objectPos, image);
		}

		// 物体光波シミュレーション再生像を保存
		object.NormalizeXYZ();
		object.SaveAsBmpSRGB(Str("output\\object%d.bmp", eyePosIdx));

		// 干渉縞シミュレーション再生像を保存
		image.NormalizeXYZ();
		image.SaveAsBmpSRGB(Str("output\\view%d.bmp", eyePosIdx));
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
