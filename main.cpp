
#include <psl.h>
using namespace wfl;
using namespace psl;

<<<<<<< HEAD
static void Coding(WaveField& sub);
static void LoadAndSave(const char* name, WaveField& wf, const WFL_RECT& rect);
static void LoadAndSave(const char* name, WaveField& wf);

static const double gRatio = 1.0 / 8.0;				 // �����ŃX�P�[���{�������߂�
static const Point	gRefPos(0.0, -50.0e-3, -200e-3); // �����ŎQ�ƌ��̈ʒu�����߂�
static const Point	gEyePos(0, 0, 200e-3);	 // �����Ŏ��_�̈ʒu�����߂�
=======
static void			Coding(WaveField& sub);
static const char*	Str(const char* format, ...);

static const double gScale = 1.0;				 // �����ŃX�P�[���{�������߂�
static const Point	gRefPos(0.0, -40.0e-3, -200e-3); // �����ŎQ�ƌ��̈ʒu�����߂�

// �����Ŏ��_�̈ʒu�����߂�
static const Point	gEyePos[5] =
{ 
	Point(0, 0, 200e-3),		// ����
	Point(0, 40e-3, 200e-3),	// ��
	Point(0, -40e-3, 200e-3),	// ��
	Point(-40e-3, 0, 200e-3),	// ��
	Point(40e-3, 0, 200e-3)		// �E
};
>>>>>>> develop

int main()
{
	Start();    wfl::SetNumThreads();								//�ő���Ƀv���Z�b�T�R�A��p����

	// ���f���̐ݒ�
	double objectSize = 20e-3 * gScale;								//���̂̎��T�C�Y
	Point  objectPos(0.0, 0.0, -150e-3); objectPos *= gScale;		//���̂̒��S�ʒu

	// ���̃��f���t�@�C���̓ǂݍ��݂Ɛݒ�
	IndexedFaceSet model;
	{
<<<<<<< HEAD
		model.LoadMqo("model\\japan.mqo");
=======
		model.LoadMqo("model\\torii3.mqo");
>>>>>>> develop
		model.Localize();											//���̂��ꎞ�I�Ɍ��_�ɒu��
		model.SetWidth(objectSize);									//���̃T�C�Y(��)��ݒ�
		model += objectPos;											//���̈ʒu��ݒ�
		model.AutoNormalVector();									//�O�[���[�V�F�[�f�B���O�̂��߂̏���
	}

	// �t���[���o�b�t�@�̐ݒ�
	double	  rgbLambda[] = { 630e-9, 540e-9, 460e-9 };				//�t���J���[�p�ɗp����g��(R, G, B)
	double	  px = 0.8e-6,				py = 0.8e-6;				//�T���v�����O�Ԋu
	int		  nx = 64 * 1024 * gScale,  ny = 64 * 1024 * gScale;	//�T���v�����O��
	WaveField frame(nx, ny, px, py);

	// �V�F�[�_�[�̐ݒ�
	Vector light(-0.408, -0.408, -0.816);							//�Ɩ����̕��� (���̕�����Mqo�̃f�t�H���g)
	double gamma = 0.01;											//�␳�����l

	// �Q�ƌ��̐ݒ�
	WaveField ref(nx, ny, px, py);
	Point	  refPos = gRefPos; refPos *= gScale;

	// ���ʁA�㉺���E�̊e���_���v�Z
	int eyePosEnd = _countof(gEyePos);
	for (int eyePosIdx = 0; eyePosIdx < eyePosEnd; ++eyePosIdx)
	{
		Printf("\n\n%d�Ԗڂ̎��_�̌v�Z�J�n\n", eyePosIdx);

		// �����Đ��̐ݒ�
		ImagingViewer view;
		{
			Point eyePos = gEyePos[eyePosIdx]; eyePos *= gScale;	//���_�ʒu
			view.SetOrigin(eyePos);
			view.SetImagingDistance(24e-3);
			view.SetPupilDiameter(6e-3);
			view.SetPx(px);
			view.SetPy(py);
			view.Init();
		}

		// ���̌��g�̃V�~�����[�V�����摜�̍쐬�ɗp����
		ColorImage object(view.GetNx(), view.GetNy());

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

			//-----------------------------------------
			// (1) ���̌��g�V�~�����[�V�����Đ������v�Z
			//-----------------------------------------
			view.SpectralView(frame, objectPos, object);

			// �t���l���z���O�����p�̕��̌��g�𓾂邽�߃z���O�����̈ʒu(z = 0)�܂œ`���v�Z����
			frame.AsmProp(-frame.GetOrigin().GetZ());

			// ���ʔg���Q�ƌ��Ƃ��Đݒ�
			ref.Clear();
			ref.SetWavelength(rgbLambda[RGBcounter - 1]);
			ref.AddSphericalWave(refPos);

			// ��l�����Ȃ̍쐬
			frame *= ref.ConvToConjugate();
			Coding(frame);

			//-------------------------------------------------------
			// �����܂ł����Ȃ̋L�^
			// �������炪���ȃV�~�����[�V����
			//-------------------------------------------------------

			// ���ȂɍĐ��Ɩ����𓖂Ă�
			frame *= ref.ConvToConjugate();

			//---------------------------------------
			// (2) ���ȃV�~�����[�V�����Đ������v�Z
			//---------------------------------------
			view.SpectralView(frame, objectPos, image);
		}

		// ���̌��g�V�~�����[�V�����Đ�����ۑ�
		object.NormalizeXYZ();
		object.SaveAsBmpSRGB(Str("output\\object%d.bmp", eyePosIdx));

<<<<<<< HEAD
		// �J���[�̌����Đ������쐬
		view.SpectralView(frame, objectPos, image);
	}

	// ���̌��g�A���Ȃ̉摜��������ۂ̐ݒ�
	double	 wDiv2 = view.GetWidth() / 2, hDiv2 = view.GetHeight() / 2;
	WFL_RECT rect(-wDiv2, +hDiv2, +wDiv2, -hDiv2);

	// ���̌��̃V�~�����[�V��������
	LoadAndSave("output\\object-R", frame);
	LoadAndSave("output\\object-G", frame);
	LoadAndSave("output\\object-B", frame);
	// ���Ȃ̃V�~�����[�V��������
	LoadAndSave("output\\fringe-R", frame);
	LoadAndSave("output\\fringe-G", frame);
	LoadAndSave("output\\fringe-B", frame);
	// �����Đ����̃V�~�����[�V��������
	image.NormalizeXYZ();
	image.SaveAsBmpSRGB("output\\result.bmp");
=======
		// ���ȃV�~�����[�V�����Đ�����ۑ�
		image.NormalizeXYZ();
		image.SaveAsBmpSRGB(Str("output\\view%d.bmp", eyePosIdx));
	}
>>>>>>> develop
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
<<<<<<< HEAD
	wf.Clear();
	wf.SetWindowMax();

	char fullname[256];
	sprintf_s(fullname, "%s.wf", name);
	wf.LoadWf(fullname);
	sprintf_s(fullname, "%s.bmp", name);
	wf.SetWindow(rect);
	wf.Normalize();
	wf.SaveAsBmp(fullname, AMPLITUDE);
}

void LoadAndSave(const char* name, WaveField& wf)
{
	wf.Clear();
	wf.SetWindowMax();

	char fullname[256];
	sprintf_s(fullname, "%s.wf", name);
	wf.LoadWf(fullname);
	sprintf_s(fullname, "%s.bmp", name);
	wf.Normalize();
	wf.SaveAsBmp(fullname, AMPLITUDE);
=======
	va_list args;
	va_start(args, format);
	static char fname[256];
	vsprintf_s(fname, format, args);
	printf("\n�m�F�F%s\n", fname);
	va_end(args);
	return fname;
>>>>>>> develop
}

