
#include <psl.h>
using namespace wfl;
using namespace psl;

static void Coding(WaveField& sub);
static void LoadWfAndSaveAsBmp(WaveField& wf, const WFL_RECT& rect, const char* name);
static void LoadWfAndSaveAsBmp(WaveField& wf, const char* name);
static const char* Str(const char* format, ...);

static const double gRatio = 1.0;				 // �����ŃX�P�[���{�������߂�
static const Point	gRefPos(0.0, -50.0e-3, -200e-3); // �����ŎQ�ƌ��̈ʒu�����߂�
static const Point	gEyePos[5] =
{ 
	Point(0, 0, 200e-3),		// ����
	Point(0, 20e-3, 200e-3),	// ��
	Point(0, -20e-3, 200e-3),	// ��
	Point(-20e-3, 0, 200e-3),	// ��
	Point(20e-3, 0, 200e-3)		// �E
};

int main()
{
	Start();    wfl::SetNumThreads();								//�ő���Ƀv���Z�b�T�R�A��p����

	// ���f���̐ݒ�
	double objectSize = 20e-3 * gRatio;								//���̂̎��T�C�Y
	Point  objectPos(0.0, 0.0, -150e-3); objectPos *= gRatio;		//���̂̒��S�ʒu

	// ���̃��f���t�@�C���̓ǂݍ��݂Ɛݒ�
	IndexedFaceSet model;
	{
		model.LoadMqo("model\\torii3.mqo");
		model.Localize();											//���̂��ꎞ�I�Ɍ��_�ɒu��
		model.SetWidth(objectSize);									//���̃T�C�Y(��)��ݒ�
		model += objectPos;											//���̈ʒu��ݒ�
		model.AutoNormalVector();									//�O�[���[�V�F�[�f�B���O�̂��߂̏���
	}

	// �t���[���o�b�t�@�̐ݒ�
	double	  rgbLambda[] = { 630e-9, 540e-9, 460e-9 };				//�t���J���[�p�ɗp����g��(R, G, B)
	double	  px = 0.8e-6,				py = 0.8e-6;				//�T���v�����O�Ԋu
	int		  nx = 64 * 1024 * gRatio,  ny = 64 * 1024 * gRatio;	//�T���v�����O��
	WaveField frame(nx, ny, px, py);

	// �V�F�[�_�[�̐ݒ�
	Vector light(-0.408, -0.408, -0.816);							//�Ɩ����̕��� (���̕�����Mqo�̃f�t�H���g)
	double gamma = 0.01;											//�␳�����l

	// �Q�ƌ��̐ݒ�
	WaveField ref(nx, ny, px, py);
	Point	  refPos = gRefPos; refPos *= gRatio;

	// ���ʁA�㉺���E�̊e���_���v�Z
	size_t eyePosEnd = _countof(gEyePos);
	for (int eyePosIdx = 0; eyePosIdx < eyePosEnd; ++eyePosIdx)
	{
		// �����Đ��̐ݒ�
		ImagingViewer view;
		{
			Point eyePos = gEyePos[eyePosIdx]; eyePos *= gRatio; //���_�ʒu
			view.SetOrigin(eyePos);
			view.SetImagingDistance(24e-3);
			view.SetPupilDiameter(6e-3);
			view.SetPx(px);
			view.SetPy(py);
			view.Init();
		}

		// �����Đ����̍쐬�ɗp����
		ColorImage image(view.GetNx(), view.GetNy());

		// ���̌��g�𓵂܂œ`������V�~�����[�V����
		for (int RGBcounter = 1; RGBcounter <= 3; ++RGBcounter)
		{
			Printf("\n\n%d�F�ڂ̌v�Z�J�n\n", RGBcounter);

			frame.Clear();
			frame.SetWavelength(rgbLambda[RGBcounter - 1]);				//�e�F�̔g���ݒ�
			frame.SetOrigin(objectPos);									//�t���[���o�b�t�@�̈ʒu�͕��̃��f���̒��S

			TfbLambertShading shading(gamma, light, (ColorMode)RGBcounter);//�����o�[�g�V�F�[�_

			// SurfaceBuilder�̐ݒ�
			SurfaceBuilder sb(frame);									//�ш搧���̂��߁C�T���v�����ƃT���v���Ԋu���𐳂����ݒ�
			{
				sb.SetBandLimitMethod(3);								//�ш搧�������x��3�ɂ���D
				sb.SetCenter(Point(0, 0, 0));							//�ш搧���̂��߁C�z���O�����̒��S�ʒu(0,0,0)��ݒ�
				sb.SetDiffractionRatio(0.9);							//��ܗ��ݒ�
				sb.SetCullingRate(0.6);									//�J�����O���ݒ�
				sb.SetShader(shading);									//��ŗp�ӂ����V�F�[�_�[�I�u�W�F�N�g��g�ݍ���
			}

			// ���̃��f��model����̌��g���v�Z���ăt���[���o�b�t�@�ɉ��Z
			sb.AddObjectFieldSb(frame, model, 1);						//�X�C�b�`�o�b�N�@�ŕ��̌��g�v�Z

			// ���̖ʂ̌��g���z
			switch (RGBcounter)
			{
			case RED:	frame.SaveAsWf(Str("output\\%d\\object-R.wf", eyePosIdx)); break;
			case GREEN: frame.SaveAsWf(Str("output\\%d\\object-G.wf", eyePosIdx)); break;
			case BLUE:	frame.SaveAsWf(Str("output\\%d\\object-B.wf", eyePosIdx)); break;
			}

			// �t���l���z���O�����p�̕��̌��g�𓾂邽�߃z���O�����̈ʒu(z = 0)�܂œ`���v�Z����
			frame.AsmProp(-frame.GetOrigin().GetZ());

			// �z���O������(z = 0)�œ����镨�̌��g�̕ۑ�
			switch (RGBcounter)
			{
			case RED:	frame.SaveAsWf(Str("output\\%d\\frame-R.wf", eyePosIdx)); break;
			case GREEN: frame.SaveAsWf(Str("output\\%d\\frame-G.wf", eyePosIdx)); break;
			case BLUE:	frame.SaveAsWf(Str("output\\%d\\frame-B.wf", eyePosIdx)); break;
			}

			// ���ʔg���Q�ƌ��Ƃ��Đݒ�
			ref.Clear();
			ref.SetWavelength(rgbLambda[RGBcounter - 1]);
			ref.AddSphericalWave(refPos);
			ref.ConvToConjugate();

			// ��l�����Ȃ̍쐬
			frame *= ref;
			Coding(frame);

			// ���Ȃ̕ۑ�
			switch (RGBcounter)
			{
			case RED:	frame.SaveAsWf(Str("output\\%d\\fringe-R.wf", eyePosIdx)); break;
			case GREEN: frame.SaveAsWf(Str("output\\%d\\fringe-G.wf", eyePosIdx)); break;
			case BLUE:	frame.SaveAsWf(Str("output\\%d\\fringe-B.wf", eyePosIdx)); break;
			}

			//-------------------------------------------------------
			// �����܂ł����Ȃ̋L�^
			// �������炪�����Đ��̃V�~�����[�V����
			//-------------------------------------------------------

			// ���ȂɍĐ��Ɩ����𓖂Ă�
			ref.ConvToConjugate();
			frame *= ref;

			// �J���[�̌����Đ������쐬
			view.SpectralView(frame, objectPos, image);
		}

		// ���̌��g�A���Ȃ̉摜��������ۂ̐ݒ�
		double	 wDiv2 = view.GetWidth() / 2, hDiv2 = view.GetHeight() / 2;
		WFL_RECT rect(-wDiv2, +hDiv2, +wDiv2, -hDiv2);

		// ���̌��̃V�~�����[�V��������
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\object-R", eyePosIdx));
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\object-G", eyePosIdx));
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\object-B", eyePosIdx));

		// ���Ȃ̃V�~�����[�V��������
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\fringe-R", eyePosIdx));
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\fringe-G", eyePosIdx));
		LoadWfAndSaveAsBmp(frame, Str("output\\%d\\fringe-B", eyePosIdx));

		// �����Đ����̃V�~�����[�V��������
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
	printf("\n�m�F�F%s\n", fname);
	va_end(args);
	return fname;
}
