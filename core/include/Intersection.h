#pragma once

#include <Utils.h>

class Material;
class Medium;
class IntersectionInfo {
public:
	IntersectionInfo() = default;

	void SetFaceNormal(const vec3& dir, const vec3& outward_normal);

public:
	float t;//�ӹ�����㵽������ľ���
	vec2 uv;//��������������
	vec3 position;//������ռ�����
	vec3 normal;//�����㷨��
	bool frontFace;//�������Ľ������Ƿ�λ�ھ����ڲ�
	vec2 pixel_ndc;//��ǰ���ص�ndc����
};