#include <Integrator.h>

bool Integrator::ReasonableTesting(float value) {
	if (isnan(value) || value < EPS) {
//		cout << "�����������" << value << endl;

		return false;
	}

	return true;
}

vec3 Integrator::GetPixelColor(IntersectionInfo& info) {
	RTCRay ray = scene->camera->GenerateRay(info.pixel_ndc.x, info.pixel_ndc.y);
	RTCRayHit rayhit = MakeRayHit(ray);

	return SolvingIntegrator(rayhit, info);
}

GBuffer Integrator::GetSceneGBuffer() {
	GBuffer gbuffer;

	gbuffer.albedoTexture = new vec3[scene->width * scene->height];
	gbuffer.normalTexture = new vec3[scene->width * scene->height];

	omp_set_num_threads(32);//�̸߳���
#pragma omp parallel for
	for (int j = 0; j < scene->height; j++) {
		for (int i = 0; i < scene->width; i++) {
			const float px = static_cast<float>(i) / static_cast<float>(scene->width);
			const float py = static_cast<float>(j) / static_cast<float>(scene->height);

			RTCRay ray = scene->camera->GenerateRay(px, py);
			RTCRayHit rayhit = MakeRayHit(ray);
			IntersectionInfo info;
			scene->Intersect(rayhit);
			if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
				scene->UpdateInfo(rayhit, info);
				gbuffer.normalTexture[j * scene->width + i] = info.normal;
				gbuffer.albedoTexture[j * scene->width + i] = scene->shapes[rayhit.hit.geomID]->material->GetAlbedo(info);
			}
			else {
				if (scene->useEnv) {
					vec3 L = GetRayDir(rayhit);

					gbuffer.albedoTexture[j * scene->width + i] = scene->env->Emitted(L);
				}
				else {
					gbuffer.albedoTexture[j * scene->width + i] = vec3(0.0f);
				}
				gbuffer.normalTexture[j * scene->width + i] = vec3(0.0f);
			}
//			cout << to_string(scene_normal[j * scene->width + i]) << endl;
		}
	}

	return gbuffer;
}

float Integrator::PowerHeuristic(float pdf1, float pdf2, int beta) {
	float p1 = pow(pdf1, beta);
	float p2 = pow(pdf2, beta);

	return p1 / (p1 + p2);
}

vec3 PathTracing::DirectLight(const RTCRayHit& rayhit, const IntersectionInfo& info, const vec3& history) {
	vec3 radiance(0.0f);

	auto hitmat = scene->shapes[rayhit.hit.geomID]->material;
	float bsdf_pdf;
	float light_pdf;

	vec3 V = -GetRayDir(rayhit);
	//ֱ�ӹ��ա���������������������������������������������������
	if (!hitmat->IsDelta()) {
		//HDR������
		if (scene->useEnv) {
			vec4 sample = sampler->Get4();
			HDRSample envsample = scene->env->Sample(info, sample);
			vec3 lightL = envsample.L;

			//HDR������ͼ��Ҫ�Բ���    
			RTCRayHit shadowRayHit = MakeRayHit(info.position, lightL, EPS, INF);

			//����һ���󽻲��ԣ��ж��Ƿ����ڵ�
			if (dot(info.normal, lightL) > EPS) { //��������������p��������ԣ���ΪN dot L < 0            
				//��չ����û���ڵ�������»�������
				if (scene->IsVisibility(shadowRayHit)) {
					//��ȡ��������L�ϵ�: 1.���չ���, 2.������ͼ�ڸ�λ�õ�pdf, 3.BSDF����ֵ, 4.BSDF�ڸ÷����pdf
					vec3 emitted = envsample.radiance;
					light_pdf = envsample.pdf;
					EvalInfo bsdf_info = hitmat->Eval(V, lightL, info);
					bsdf_pdf = bsdf_info.bsdf_pdf;

					if (!(ReasonableTesting(bsdf_pdf) && ReasonableTesting(light_pdf) && ReasonableTesting(bsdf_info.costheta))) {
						return vec3(0.0f);
					}

					float misWeight = PowerHeuristic(light_pdf, bsdf_pdf, 2);

					radiance += misWeight * emitted * history * bsdf_info.bsdf * abs(bsdf_info.costheta) / light_pdf;
				}
			}
		}

		if (scene->lights.size() != 0) {
			//����1. ���ѡ��һ����Դ
			if (traceLightType == RANDOM) {
				int index = sampler->Get1() * scene->lights.size();
				auto light = scene->lights[index];
				vec2 sample = sampler->Get2();
				LightSample light_sample = light->Sample(info, sample);
				light_pdf = light_sample.light_pdf;
				EvalInfo bsdf_info = hitmat->Eval(V, light_sample.light_dir, info);

				//��Ӱ����
				if (dot(light_sample.light_dir, light_sample.light_normal) < -EPS) {
					RTCRayHit shadowRayHit = MakeRayHit(info.position, light_sample.light_dir, EPS, light_sample.dist - EPS);
					if (scene->IsVisibility(shadowRayHit)) {
						auto lightmat = scene->shapes[index]->material;
						vec3 emitted = light_sample.radiance * static_cast<float>(scene->lights.size());
						bsdf_pdf = bsdf_info.bsdf_pdf;

						if (!(ReasonableTesting(bsdf_pdf) && ReasonableTesting(light_pdf) && ReasonableTesting(bsdf_info.costheta))) {
							return vec3(0.0f);
						}

						float misWeight = PowerHeuristic(light_pdf, bsdf_pdf, 2);

						radiance += misWeight * emitted * history * bsdf_info.bsdf * abs(bsdf_info.costheta) / light_pdf;
					}
				}
			}

			//����2. ѡ��ȫ����Դ
			if (traceLightType == ALL) {
				for (int index = 0; index < scene->lights.size(); index++) {
					auto light = scene->lights[index];
					vec2 sample = sampler->Get2();
					LightSample light_sample = light->Sample(info, sample);
					light_pdf = light_sample.light_pdf;
					EvalInfo bsdf_info = hitmat->Eval(V, light_sample.light_dir, info);

					//��Ӱ����
					if (dot(light_sample.light_dir, light_sample.light_normal) < -EPS) {
						RTCRayHit shadowRayHit = MakeRayHit(info.position, light_sample.light_dir, EPS, light_sample.dist - EPS);
						if (scene->IsVisibility(shadowRayHit)) {
							auto lightmat = scene->shapes[index]->material;
							vec3 emitted = light_sample.radiance;
							bsdf_pdf = bsdf_info.bsdf_pdf;

							if (!(ReasonableTesting(bsdf_pdf) && ReasonableTesting(light_pdf) && ReasonableTesting(bsdf_info.costheta))) {
								return vec3(0.0f);
							}

							float misWeight = PowerHeuristic(light_pdf, bsdf_pdf, 2);

							radiance += misWeight * emitted * history * bsdf_info.bsdf * abs(bsdf_info.costheta) / light_pdf;
						}
					}
				}
			}
		}
 	}

	return radiance;
}

vec3 PathTracing::SolvingIntegrator(RTCRayHit& rayhit, IntersectionInfo& info) {
	//��һ�λ���
	scene->Intersect(rayhit);
	if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
		scene->UpdateInfo(rayhit, info);
		//�ж��Ƿ��ǹ�Դ��������򷵻ع�Դ��ɫ
		if (scene->shapes[rayhit.hit.geomID]->material->m_type == MaterialType::DiffuseLight) {
			if (info.frontFace) {
				return scene->shapes[rayhit.hit.geomID]->material->Emitted(info);
			}
			else {
				return vec3(0.0f);
			}
		}
// 		else {
// 			return info.normal * 0.5f + 0.5f;
// 		}
	}
	else {
		//�ж��Ƿ��б���ɫ��������򷵻ر���ɫ�����򷵻�0
		if (scene->useEnv) {
			vec3 L = GetRayDir(rayhit);

			return scene->env->Emitted(L);
		}
		else {
			return vec3(0.0f);
		}
//		return vec3(0.0f);
// 		vec3 L = GetRayDir(rayhit);
// 		float t = 0.5f * (L.y + 1.0f);
// 
// 		return mix(vec3(1.0f), vec3(0.5f, 0.7f, 1.0f), t);
	}

	vec3 radiance(0.0f);
	vec3 history(1.0f);
	for (int i = 0; i < depth; i++) {
		auto hitmat = scene->shapes[rayhit.hit.geomID]->material;

		vec3 V = -GetRayDir(rayhit);
		//ֱ�ӹ��ա���������������������������������������������������
		radiance += DirectLight(rayhit, info, history);

		//��ӹ��ա�����������������������������������������������������������������������������������������������
		//����·��radianceԽСԽ���׶�ʧ
		float prr = hitmat->IsDelta() ? 1.0f : std::min(1.0f, (history[0] + history[1] + history[2]) / 3.0f);
		if (sampler->Get1() > prr) {
			break;
		}

		BsdfSample bsdf_sample = scene->shapes[rayhit.hit.geomID]->material->Sample(V, info, sampler);
		float bsdf_pdf = bsdf_sample.bsdf_pdf;
		vec3 L = bsdf_sample.bsdf_dir;
		EvalInfo bsdf_info = scene->shapes[rayhit.hit.geomID]->material->Eval(V, L, info);

		if (!(ReasonableTesting(bsdf_pdf) && ReasonableTesting(bsdf_info.costheta))) {
			break;
		}

		rayhit = MakeRayHit(info.position, L);

		history *= bsdf_info.bsdf * abs(bsdf_info.costheta) / bsdf_pdf / prr;

		if (IsNan(history)) {
			break;
		}

		IntersectionInfo newInfo;
		scene->Intersect(rayhit);
		if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
			scene->UpdateInfo(rayhit, newInfo);

			//�ж��Ƿ���еƹ�
			if (scene->shapes[rayhit.hit.geomID]->material->m_type == MaterialType::DiffuseLight) {
				if (!newInfo.frontFace) {
					break;
				}

				vec3 emitted = scene->shapes[rayhit.hit.geomID]->material->Emitted(info);
				float misWeight = 1.0f;
				if (!hitmat->IsDelta()) {
					float dist = distance(info.position, newInfo.position);
					float light_pdf = scene->shapes[rayhit.hit.geomID]->Pdf(newInfo, L, dist);

					if (!ReasonableTesting(light_pdf)) {
						break;
					}

					misWeight = PowerHeuristic(bsdf_pdf, light_pdf, 2);
				}

				radiance += misWeight * history * emitted;

				break;
			}
		}
		else {
			//δ�����κ����壬���ر���ɫ
// 			float t = 0.5f * (L.y + 1.0f);
// 			radiance += history * mix(vec3(1.0f), vec3(0.5f, 0.7f, 1.0f), t);
			if (scene->useEnv) {
				vec3 emitted = scene->env->Emitted(L);
				float light_pdf = scene->env->Pdf(L);

				if (!ReasonableTesting(light_pdf)) {
					break;
				}

				float misWeight = PowerHeuristic(bsdf_pdf, light_pdf, 2);

				radiance += misWeight * emitted * history;
			}
			break;
		}

		info = newInfo;
	}

	return radiance;
}

vec3 VolumetricPathTracing::DirectLight(const RTCRayHit& rayhit, const IntersectionInfo& info, const vec3& history) {
	vec3 radiance(0.0f);

	auto hitmat = scene->shapes[rayhit.hit.geomID]->material;
	float bsdf_pdf;
	float light_pdf;

	vec3 V = -GetRayDir(rayhit);
	//ֱ�ӹ��ա���������������������������������������������������
	if (!hitmat->IsDelta()) {
		//HDR������
		if (scene->useEnv) {
			vec4 sample = sampler->Get4();
			HDRSample envsample = scene->env->Sample(info, sample);
			vec3 lightL = envsample.L;

			//HDR������ͼ��Ҫ�Բ���    
			RTCRayHit shadowRayHit = MakeRayHit(info.position, lightL, EPS, INF);

			//����һ���󽻲��ԣ��ж��Ƿ����ڵ�
			if (dot(info.normal, lightL) > EPS) { //��������������p��������ԣ���ΪN dot L < 0            
				//��չ����û���ڵ�������»�������
				if (scene->IsVisibility(shadowRayHit)) {
					//��ȡ��������L�ϵ�: 1.���չ���, 2.������ͼ�ڸ�λ�õ�pdf, 3.BSDF����ֵ, 4.BSDF�ڸ÷����pdf
					vec3 emitted = envsample.radiance;
					light_pdf = envsample.pdf;
					EvalInfo bsdf_info = hitmat->Eval(V, lightL, info);
					bsdf_pdf = bsdf_info.bsdf_pdf;

					if (!(ReasonableTesting(bsdf_pdf) && ReasonableTesting(light_pdf) && ReasonableTesting(bsdf_info.costheta))) {
						return vec3(0.0f);
					}

					float misWeight = PowerHeuristic(light_pdf, bsdf_pdf, 2);

					radiance += misWeight * emitted * history * bsdf_info.bsdf * abs(bsdf_info.costheta) / light_pdf;
				}
			}
		}

		if (scene->lights.size() != 0) {
			//����1. ���ѡ��һ����Դ
			if (traceLightType == RANDOM) {
				int index = sampler->Get1() * scene->lights.size();
				auto light = scene->lights[index];
				vec2 sample = sampler->Get2();
				LightSample light_sample = light->Sample(info, sample);
				light_pdf = light_sample.light_pdf;
				EvalInfo bsdf_info = hitmat->Eval(V, light_sample.light_dir, info);

				//��Ӱ����
				if (dot(light_sample.light_dir, light_sample.light_normal) < -EPS) {
					RTCRayHit shadowRayHit = MakeRayHit(info.position, light_sample.light_dir, EPS, light_sample.dist - EPS);
					if (scene->IsVisibility(shadowRayHit)) {
						auto lightmat = scene->shapes[index]->material;
						vec3 emitted = light_sample.radiance * static_cast<float>(scene->lights.size());
						bsdf_pdf = bsdf_info.bsdf_pdf;

						if (!(ReasonableTesting(bsdf_pdf) && ReasonableTesting(light_pdf) && ReasonableTesting(bsdf_info.costheta))) {
							return vec3(0.0f);
						}

						float misWeight = PowerHeuristic(light_pdf, bsdf_pdf, 2);

						radiance += misWeight * emitted * history * bsdf_info.bsdf * abs(bsdf_info.costheta) / light_pdf;
					}
				}
			}

			//����2. ѡ��ȫ����Դ
			if (traceLightType == ALL) {
				for (int index = 0; index < scene->lights.size(); index++) {
					auto light = scene->lights[index];
					vec2 sample = sampler->Get2();
					LightSample light_sample = light->Sample(info, sample);
					light_pdf = light_sample.light_pdf;
					EvalInfo bsdf_info = hitmat->Eval(V, light_sample.light_dir, info);

					//��Ӱ����
					if (dot(light_sample.light_dir, light_sample.light_normal) < -EPS) {
						RTCRayHit shadowRayHit = MakeRayHit(info.position, light_sample.light_dir, EPS, light_sample.dist - EPS);
						if (scene->IsVisibility(shadowRayHit)) {
							auto lightmat = scene->shapes[index]->material;
							vec3 emitted = light_sample.radiance;
							bsdf_pdf = bsdf_info.bsdf_pdf;

							if (!(ReasonableTesting(bsdf_pdf) && ReasonableTesting(light_pdf) && ReasonableTesting(bsdf_info.costheta))) {
								return vec3(0.0f);
							}

							float misWeight = PowerHeuristic(light_pdf, bsdf_pdf, 2);

							radiance += misWeight * emitted * history * bsdf_info.bsdf * abs(bsdf_info.costheta) / light_pdf;
						}
					}
				}
			}
		}
	}

	return radiance;
}

vec3 VolumetricPathTracing::SolvingIntegrator(RTCRayHit& rayhit, IntersectionInfo& info) {
	vec3 radiance(0.0f);
	vec3 history(1.0f);
	vec3 pre_position = GetRayOrg(rayhit);//��һ���ཻ���λ�ã���ʼ��Ϊ���λ��
	float pre_bsdf_pdf = 0.0f;
	bool pre_isDelta;
	vec3 V(-GetRayDir(rayhit)), L(GetRayDir(rayhit));
	for (int i = 0; i < depth; i++) {
		scene->Intersect(rayhit);
		if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
			scene->UpdateInfo(rayhit, info);

			//�ж��Ƿ���еƹ�
			if (scene->shapes[rayhit.hit.geomID]->material->m_type == MaterialType::DiffuseLight) {
				if (!info.frontFace) {
					break;
				}

				vec3 emitted = scene->shapes[rayhit.hit.geomID]->material->Emitted(info);
				if (i == 0) {
					return emitted;
				}
				float misWeight = 1.0f;
				if (!pre_isDelta) {
					float dist = distance(info.position, pre_position);
					float light_pdf = scene->shapes[rayhit.hit.geomID]->Pdf(info, L, dist);

					if (!ReasonableTesting(light_pdf)) {
						break;
					}

					misWeight = PowerHeuristic(pre_bsdf_pdf, light_pdf, 2);
				}

				radiance += misWeight * history * emitted;

				break;
			}
		}
		else {
			if (scene->useEnv) {
				vec3 emitted = scene->env->Emitted(L);
				if (i == 0) {
					return emitted;
				}
				float light_pdf = scene->env->Pdf(L);

				if (!ReasonableTesting(light_pdf)) {
					break;
				}

				float misWeight = 1.0f;
				if (!pre_isDelta) {
					misWeight = PowerHeuristic(pre_bsdf_pdf, light_pdf, 2);
				}

				radiance += misWeight * emitted * history;
			}
			break;
		}

		auto hitmat = scene->shapes[rayhit.hit.geomID]->material;

		V = -GetRayDir(rayhit);
		//ֱ�ӹ��ա���������������������������������������������������
		radiance += DirectLight(rayhit, info, history);

		//��ӹ��ա�����������������������������������������������������������������������������������������������
		//����·��radianceԽСԽ���׶�ʧ
		float prr = hitmat->IsDelta() ? 1.0f : std::min(1.0f, (history[0] + history[1] + history[2]) / 3.0f);
		if (sampler->Get1() > prr) {
			break;
		}

		BsdfSample bsdf_sample = hitmat->Sample(V, info, sampler);
		float bsdf_pdf = bsdf_sample.bsdf_pdf;
		L = bsdf_sample.bsdf_dir;
		EvalInfo bsdf_info = hitmat->Eval(V, L, info);

		if (!(ReasonableTesting(bsdf_pdf) && ReasonableTesting(bsdf_info.costheta))) {
			break;
		}

		history *= bsdf_info.bsdf * abs(bsdf_info.costheta) / bsdf_pdf / prr;
		if (IsNan(history)) {
			break;
		}

		pre_isDelta = hitmat->IsDelta();
		pre_position = info.position;
		pre_bsdf_pdf = bsdf_pdf;
		rayhit = MakeRayHit(info.position, L);
	}

	return radiance;
}