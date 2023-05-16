#include <Camera.h>

PinholeCamera::PinholeCamera(vec3 lookfrom, vec3 lookat, vec3 vup, float znear, float vfov, float aspect) {
	//��������ϵ
	origin = lookfrom;

	float theta = radians(vfov);

	//�����ƽ��Ŀ�͸ߣ���͸�������
	float half_height = abs(znear) * tan(theta / 2.0f);
	float half_width = aspect * half_height;
	w = normalize(lookfrom - lookat);//�����z��
	u = normalize(cross(vup, w));//�����x��
	v = normalize(cross(w, u));//�����y��

	//ȷ�����ͶӰƽ��������ռ�����½�λ��
	lower_left_corner = origin - half_width * u - half_height * v - abs(znear) * w;

	horizontal = 2.0f * half_width * u;//��ƽ��Ŀ�����
	vertical = 2.0f * half_height * v;//��ƽ��ĸ�����
}

auto PinholeCamera::GenerateRay(float x, float y) const noexcept -> RTCRay {
	RTCRay ray;

	vec3 direction = normalize(lower_left_corner + x * horizontal + y * vertical - origin);

	ray.org_x = origin.x;
	ray.org_y = origin.y;
	ray.org_z = origin.z;

	ray.dir_x = direction.x;
	ray.dir_y = direction.y;
	ray.dir_z = direction.z;

	ray.tnear = EPS;
	ray.tfar = INF;
	ray.time = 0.0f;

	return ray;
}