
#include <psl.h>
using namespace wfl;
using namespace psl;

static void Coding(WaveField& sub);

static const double gRatio = 1.0 / 16.0;
static const Point	gRefPos(0.0, -0.5e-3, -10e-3);

int main()
{
	Start();    wfl::SetNumThreads();								//最大限にプロセッサコアを用いる

	// モデルの設定
	double objectSize = 3e-3 * gRatio;								//物体の実サイズ
	Point  objectPos(0.0, 0.0, -20e-3); objectPos *= gRatio;		//物体の中心位置

	// 物体モデルファイルの読み込みと設定
	IndexedFaceSet model;
	model.LoadMqo("SampleData\\star8.mqo");
	model.Localize();												//物体を一時的に原点に置く
	model.SetWidth(objectSize);										//物体サイズ(幅)を設定
	model += objectPos;												//物体位置を設定
	model.AutoNormalVector();										//グーローシェーディングのための準備

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

	// 結像再生の設定
	ImagingViewer view;
	Point eyePos(0, 0, 20e-3); eyePos *= gRatio; //視点位置
	Point focusPos = objectPos + Point(0.0, 0.0, 0.0);
	view.SetOrigin(eyePos);
	view.SetImagingDistance(24e-3);
	view.SetPupilDiameter(6e-3);
	view.SetPx(px);
	view.SetPy(py);
	view.Init();

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
		sb.SetBandLimitMethod(3);									//帯域制限をレベル3にする．
		sb.SetCenter(Point(0, 0, 0));								//帯域制限のため，ホログラムの中心位置(0,0,0)を設定
		sb.SetDiffractionRatio(0.9);								//回折率設定
		sb.SetCullingRate(0.6);										//カリング率設定
		sb.SetShader(shading);										//上で用意したシェーダーオブジェクトを組み込む

		// 物体モデルmodelからの光波を計算してフレームバッファに加算
		sb.AddObjectFieldSb(frame, model, 1);						//スイッチバック法で物体光波計算

		// デバッグ用にセーブ
		switch (RGBcounter)
		{
		case RED:	frame.SaveAsWf("output\\object-R.wf"); break;
		case GREEN: frame.SaveAsWf("output\\object-G.wf"); break;
		case BLUE:	frame.SaveAsWf("output\\object-B.wf"); break;
		}

		// フレネルホログラム用の物体光波を得るためホログラムの位置(z = 0)まで伝搬計算する
		frame.ExactAsmProp(-frame.GetOrigin().GetZ());

		// ホログラム面(z = 0)で得られる物体光波の保存
		switch (RGBcounter)
		{
		case RED:	frame.SaveAsWf("output\\frame-R.wf"); break;
		case GREEN: frame.SaveAsWf("output\\frame-G.wf"); break;
		case BLUE:	frame.SaveAsWf("output\\frame-B.wf"); break;
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
		case RED:	frame.SaveAsWf("output\\fringe-R.wf"); break;
		case GREEN: frame.SaveAsWf("output\\fringe-G.wf"); break;
		case BLUE:	frame.SaveAsWf("output\\fringe-B.wf"); break;
		}

		// 干渉縞に再生照明光を当てる
		ref.ConvToConjugate();
		frame *= ref;

		// RGB各色の結像再生像を作成
		view.Clear();
		//view.SetWavelength(frame.GetWavelength());
		view.View(frame, focusPos);
		view.ImageRotation(2);

		// RGB各色の結像再生像を加算
		image += view;
	}

	image.NormalizeXYZ(10.0);
	image.SaveAsBmpSRGB("output\\result.bmp");

	WaveField test;
	test.LoadWf("output\\object-R.wf");
	test.SaveAsBmp("output\\test.bmp", Mode::AMPLITUDE);
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
