#pragma once

#include <Utils.h>
#include <Sampler.h>
#include <Intersection.h>

enum class PhaseFunctionType {
	Isotropic,        //����ͬ�Ե��ຯ��
	HenyeyGreenstein, //����-����˹̹�ຯ��
};

struct PhaseSample {
	PhaseSample(vec3 dir = vec3(0.0f), float pdf = -1.0f) :
		phase_dir(dir), phase_pdf(pdf) {}

	vec3 phase_dir;
	float phase_pdf;
};
typedef PhaseSample PhaseSampleError;

struct PhaseInfo {
	vec3 attenuation;
	float phase_pdf;
};

//�ຯ��
class PhaseFunction {
public:

	virtual PhaseSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const = 0;
	virtual PhaseInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) const = 0;

protected:
	PhaseFunction(PhaseFunctionType type) : type_(type) {}

private:
	PhaseFunctionType type_; //�ຯ������
};

//����ͬ�Ե��ຯ��
class IsotropicPhaseFunction : public PhaseFunction {
public:
	IsotropicPhaseFunction() : PhaseFunction(PhaseFunctionType::Isotropic) {}

	virtual PhaseSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const override;
	virtual PhaseInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) const override;
};

//����-����˹̹�ຯ��
class HenyeyGreensteinPhaseFunction : public PhaseFunction {
public:
	HenyeyGreensteinPhaseFunction(const vec3& g) : PhaseFunction(PhaseFunctionType::HenyeyGreenstein), g_(g) {}

	virtual PhaseSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const override;
	virtual PhaseInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) const override;

private:
	vec3 g_; //����ɢ�����ƽ�����ҵĲ���
};

enum class MediumType {
	Homogeneous, //���Ƚ���
};

class Medium {
public:
	virtual bool SampleDistance(float max_distance, float* distance, float* pdf, vec3* attenuation, Sampler* sampler) const = 0;
	virtual std::pair<vec3, float> EvalDistance(bool scattered, float distance) const = 0;
	virtual PhaseSample SamplePhaseFunction(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const = 0;
	virtual PhaseInfo EvalPhaseFunction(const vec3& V, const vec3& L, const IntersectionInfo& info) const = 0;

protected:
	Medium(MediumType type) : type_(type) {}

private:
	MediumType type_; //��������
};

//���Ƚ���
class HomogeneousMedium : public Medium {
public:
	HomogeneousMedium(const vec3& sigma_a, const vec3& sigma_s, PhaseFunction* phase_function);
	~HomogeneousMedium();

	bool SampleDistance(float max_distance, float* distance, float* pdf, vec3* attenuation, Sampler* sampler) const override;
	std::pair<vec3, float> EvalDistance(bool scattered, float distance) const override;
	PhaseSample SamplePhaseFunction(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const override;
	PhaseInfo EvalPhaseFunction(const vec3& V, const vec3& L, const IntersectionInfo& info) const override;

private:
	PhaseFunction* phase_function_;//�ຯ��
	float medium_sampling_weight_; //���������ڽ����ڲ�����ɢ���Ȩ��
	vec3 sigma_s_;                 //ɢ��ϵ��
	vec3 sigma_t_;                 //˥��ϵ��
};