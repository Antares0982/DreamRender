# DreamRender
——————————————————————————

该分支没有bug了，并且删除了冗余代码。

——————————————————————————

这是我写的第一个离线渲染器，起源于raytracing三部曲，但已经被我改了看不出来了😄，未来会更新更多的渲染算法(参考了github上的大量开源项目，构建非常慢，资产在release的压缩包里，想运行可以去下release版本的:https://github.com/GraphicsEnthusiast/DreamRender/releases/tag/v1.0 )。

glfw, glad, glm, nlohmann_json库用vcpkg安装，构建时记得修改vcpkg路径

![image](https://github.com/qaz123w/DreamRender/assets/75780167/99953c96-80ea-4e0d-a902-e892995be9d0)

构建后还需将所需dll移动到exe文件所在的目录，shader需要移动到exe文件所在目录的上一级。

- 渲染算法
  - 路径追踪(PathTracing，实现了多重重要性采样)

- 几何
  - TriangleMesh
  - Sphere
  - Quad

- 加速结构
  - 使用embree3进行光线求交

- 降噪
  - 可选择是否开启oidn降噪

- 材质
  - Disney BSDF
  - 粗糙材质(GGX Microfacet BSDF，包括金属，电介质，塑料，以及kulla-conty方法，采样VNDF)
  - Lambertian
  - Oren-Nayer
  - 平滑材质(包括金属，电介质，塑料)
  - 薄的电介质(ThinDielectric)
  - 金属工作流(MetalWorkflow，即Cook-Torrance BRDF)
  - ClearcoatedConductor(在粗糙金属表面涂一层清漆)

- 相机
  - PinholeCamera
  - ThinlensCamera

- 采样器
  - Independent
  - SimpleSobol

- 滤波器
  - GaussianFilter
  - BoxFilter
  - TriangleFilter
  - TentFilter

- 光源
  - 点光源
  - 面光源(Quad，Sphere)
  - 平行光
  - HDR环境光

- 场景描述
  - json场景解析

- 截图

BreakfastRoom
![breakfast](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/cda01ee4-c6dd-4a0b-8c03-5fc7725063f8)
Staircase
![staircase](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/d9781eb6-ff2a-4fba-bef3-a218d73d51e9)
Mis
![mis](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/6e73c199-8480-47d2-bffb-7d9ac00e99ba)
CornellBox
![cor](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/2efdb7fe-576d-4bfc-b228-7a76de41cdf2)
![cornellbox](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/8902ff4f-bdf0-45db-bc53-3dee55575d04)
Hyperion
![hyperion](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/499beb77-3a91-4f0f-af07-27fe8a001447)
Teapot
![teapot](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/a9f9ef80-7535-45bf-ab5a-98365393f1c4)
Boy
![boy](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/47a5acd6-dfe9-4734-80a5-7b8cf9847caf)
![boy2](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/5002aaa3-679d-4a4c-8914-705d77fbe813)

