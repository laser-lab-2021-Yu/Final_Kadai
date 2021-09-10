
#include <psl.h>
using namespace wfl;
using namespace psl;

static void Coding(WaveField& sub);
static void LoadAndSave(const char* fname, WaveField& wf, const WFL_RECT& rect);

static const double gRatio = 1.0 / 8.0;
static const Point	gRefPos(0.0, -1.0e-3, -40e-3);
static const Point	gEyePos(0, 0.0, 20e-3);

int main()
{
	Start();    wfl::SetNumThreads();								//�ő���Ƀv���Z�b�T�R�A��p����

	// ���f���̐ݒ�
	double objectSize = 3e-3 * gRatio;								//���̂̎��T�C�Y
	Point  objectPos(0.0, 0.0, -20e-3); objectPos *= gRatio;		//���̂̒��S�ʒu

	// ���̃��f���t�@�C���̓ǂݍ��݂Ɛݒ�
	IndexedFaceSet model;
	model.LoadMqo("model\\pondelion.mqo");
	model.Localize();												//���̂��ꎞ�I�Ɍ��_�ɒu��
	model.SetWidth(objectSize);										//���̃T�C�Y(��)��ݒ�
	model += objectPos;												//���̈ʒu��ݒ�
	model.AutoNormalVector();										//�O�[���[�V�F�[�f�B���O�̂��߂̏���

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

	// �����Đ��̐ݒ�
	ImagingViewer view;
	Point eyePos = gEyePos; eyePos *= gRatio; //���_�ʒu
	Point focusPos = objectPos;
	view.SetOrigin(eyePos);
	view.SetImagingDistance(24e-3);
	view.SetPupilDiameter(6e-3);
	view.SetPx(px);
	view.SetPy(py);
	view.Init();

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
		sb.SetBandLimitMethod(3);									//�ш搧�������x��3�ɂ���D
		sb.SetCenter(Point(0, 0, 0));								//�ш搧���̂��߁C�z���O�����̒��S�ʒu(0,0,0)��ݒ�
		sb.SetDiffractionRatio(0.9);								//��ܗ��ݒ�
		sb.SetCullingRate(0.6);										//�J�����O���ݒ�
		sb.SetShader(shading);										//��ŗp�ӂ����V�F�[�_�[�I�u�W�F�N�g��g�ݍ���

		// ���̃��f��model����̌��g���v�Z���ăt���[���o�b�t�@�ɉ��Z
		sb.AddObjectFieldSb(frame, model, 1);						//�X�C�b�`�o�b�N�@�ŕ��̌��g�v�Z

		// ���̌��g���z
		switch (RGBcounter)
		{
		case RED:	frame.SaveAsWf("output\\object-R.wf"); break;
		case GREEN: frame.SaveAsWf("output\\object-G.wf"); break;
		case BLUE:	frame.SaveAsWf("output\\object-B.wf"); break;
		}

		// �t���l���z���O�����p�̕��̌��g�𓾂邽�߃z���O�����̈ʒu(z = 0)�܂œ`���v�Z����
		frame.ExactAsmProp(-frame.GetOrigin().GetZ());

		// �z���O������(z = 0)�œ����镨�̌��g�̕ۑ�
		switch (RGBcounter)
		{
		case RED:	frame.SaveAsWf("output\\frame-R.wf"); break;
		case GREEN: frame.SaveAsWf("output\\frame-G.wf"); break;
		case BLUE:	frame.SaveAsWf("output\\frame-B.wf"); break;
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
		case RED:	frame.SaveAsWf("output\\fringe-R.wf"); break;
		case GREEN: frame.SaveAsWf("output\\fringe-G.wf"); break;
		case BLUE:	frame.SaveAsWf("output\\fringe-B.wf"); break;
		}

		// ���ȂɍĐ��Ɩ����𓖂Ă�
		ref.ConvToConjugate();
		frame *= ref;

		// RGB�e�F�̌����Đ������쐬
		view.Clear();
		//view.SetWavelength(frame.GetWavelength());
		view.View(frame, focusPos);
		view.ImageRotation(2);

		// RGB�e�F�̌����Đ��������Z
		image += view;
	}

	WaveField red, green, blue;
	WFL_RECT rect(0, view.GetNy() - 1, view.GetNx() - 1, view.GetNy() / 2);
	// ���̌��g�̃V�~�����[�V��������
	LoadAndSave("output\\object-R", red, rect);
	LoadAndSave("output\\object-G", green, rect);
	LoadAndSave("output\\object-B", blue, rect);
	// ���Ȃ̃V�~�����[�V��������
	LoadAndSave("output\\fringe-R", red, rect);
	LoadAndSave("output\\fringe-G", green, rect);
	LoadAndSave("output\\fringe-B", blue, rect);
	// �����Đ����̃V�~�����[�V��������
	image.NormalizeXYZ(10);
	image.SaveAsBmpSRGB("output\\result.bmp");
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

void LoadAndSave(const char* fname, WaveField& wf, const WFL_RECT& rect)
{
	char fullname[256];
	sprintf_s(fullname, "%s.wf", fname);
	wf.LoadWf(fullname);
	//wf.SetWindow(rect);
	sprintf_s(fullname, "%s.bmp", fname);
	wf.SaveAsBmp(fullname, AMPLITUDE);
}
