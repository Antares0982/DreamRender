#pragma once

#include <Utils.h>
#include <SobolMatrices1024x52.h>

//������ 
inline uint GrayCode(uint i) {
	return i ^ (i >> 1);
}

//���ɵ�dά�ȵĵ�i��Sobol��
inline float Sobol(uint d, uint i) {
	uint result = 0;
	uint offset = d * SobolMatricesSize;
	for (uint j = 0; i != 0; i >>= 1, j++) {
		if ((i & 1) != 0) {
			result ^= SobolMatrices[j + offset];
		}
	}

	return float(result) * (1.0f / float(0xFFFFFFFFU));
}

//���ɵ�i֡�ĵ�b�η�����Ҫ�Ķ�ά�������
inline vec2 SobolVec2(uint i, uint b) {
	float u = Sobol(b * 2, GrayCode(i));
	float v = Sobol(b * 2 + 1, GrayCode(i));

	return vec2(u, v);
}

inline vec2 CranleyPattersonRotation(vec2 p, vec2 pix) {
	uint pseed = uint(
		uint(pix.x) * uint(1973) +
		uint(pix.y) * uint(9277) +
		uint(114514 / 1919) * uint(26699)) | uint(1);

	float u = RandomFloat();
	float v = RandomFloat();

	p.x += u;
	if (p.x > 1.0f) {
		p.x -= 1.0f;
	}
	if (p.x < 0) {
		p.x += 1.0f;
	}

	p.y += v;
	if (p.y > 1) {
		p.y -= 1.0f;
	}
	if (p.y < 0) {
		p.y += 1.0f;
	}

	return p;
}