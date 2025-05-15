#include <Novice.h>
#include <cstdint>
#include <imgui.h>
#define _USE_MATH_DEFINES
#include <cmath>

const char kWindowTitle[] = "LC1C_24_マキノハルト_タイトル";

struct Matrix4x4
{
	float m[4][4];
};
struct Vector3
{
	float x, y, z;
};
struct Vector4
{
	float x, y, z, w;
};
struct Sphere
{
	Vector3 center;
	float radius;
};
struct Vector2
{
	float x, y;
};

static const int kWindowWidth = 1280;
static const int kWindowHeight = 720;
Vector3 gGridPosition = { 0.0f, 4.5f, 22.0f }; // ← グリッドの位置

Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b) {
	Matrix4x4 r{};
	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < 4; ++col) {
			for (int k = 0; k < 4; ++k) {
				r.m[row][col] += a.m[row][k] * b.m[k][col];
			}
		}
	}
	return r;
}
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
	Matrix4x4 m{};
	float halfW = width / 2.0f;
	float halfH = height / 2.0f;
	float depth = maxDepth - minDepth;

	m.m[0][0] = halfW;
	m.m[1][1] = -halfH;
	m.m[2][2] = depth;

	m.m[3][0] = left + halfW;
	m.m[3][1] = top + halfH;
	m.m[3][2] = minDepth;
	m.m[3][3] = 1.0f;

	return m;
}
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	float cosX = cosf(rotate.x), sinX = sinf(rotate.x);
	float cosY = cosf(rotate.y), sinY = sinf(rotate.y);
	float cosZ = cosf(rotate.z), sinZ = sinf(rotate.z);

	Matrix4x4 m{};

	m.m[0][0] = scale.x * (cosY * cosZ);
	m.m[0][1] = scale.x * (cosY * sinZ);
	m.m[0][2] = scale.x * (-sinY);
	m.m[0][3] = translate.x;

	m.m[1][0] = scale.y * (sinX * sinY * cosZ - cosX * sinZ);
	m.m[1][1] = scale.y * (sinX * sinY * sinZ + cosX * cosZ);
	m.m[1][2] = scale.y * (sinX * cosY);
	m.m[1][3] = translate.y;

	m.m[2][0] = scale.z * (cosX * sinY * cosZ + sinX * sinZ);
	m.m[2][1] = scale.z * (cosX * sinY * sinZ - sinX * cosZ);
	m.m[2][2] = scale.z * (cosX * cosY);
	m.m[2][3] = translate.z;

	m.m[3][0] = 0.0f;
	m.m[3][1] = 0.0f;
	m.m[3][2] = 0.0f;
	m.m[3][3] = 1.0f;

	return m;
}
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspect, float nearZ, float farZ) {
	Matrix4x4 m{};
	float yScale = 1.0f / tanf(fovY / 2.0f);
	float xScale = yScale / aspect;

	m.m[0][0] = xScale;
	m.m[1][1] = yScale;
	m.m[2][2] = -(farZ + nearZ) / (farZ - nearZ);
	m.m[2][3] = -1.0f;
	m.m[3][2] = -(2 * farZ * nearZ) / (farZ - nearZ);

	return m;
}
Vector3 TransformWithW(const Vector3& v, const Matrix4x4& m) {
	float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
	float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
	float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
	float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];

	if (w != 0.0f) {
		x /= w;
		y /= w;
		z /= w;
	}

	return { x, y, z };
}
Vector3 Transform(const Vector3& v, const Matrix4x4& m) {
	Vector3 result;
	result.x = v.x * m.m[0][0] + v.y * m.m[0][1] + v.z * m.m[0][2] + m.m[0][3];
	result.y = v.x * m.m[1][0] + v.y * m.m[1][1] + v.z * m.m[1][2] + m.m[1][3];
	result.z = v.x * m.m[2][0] + v.y * m.m[2][1] + v.z * m.m[2][2] + m.m[2][3];
	return result;
}
Vector3 Cross(const Vector3& v1, const Vector3& v2) {
	Vector3 result;
	result.x = v1.y * v2.z - v1.z * v2.y;
	result.y = v1.z * v2.x - v1.x * v2.z;
	result.z = v1.x * v2.y - v1.y * v2.x;
	return result;
}
Matrix4x4 MakeLookAtMatrix(const Vector3& eye, const Vector3& target, const Vector3& up) {
	Vector3 zAxis = {
	eye.x - target.x,
	eye.y - target.y,
	eye.z - target.z
	};

	// 正規化
	float lengthZ = sqrtf(zAxis.x * zAxis.x + zAxis.y * zAxis.y + zAxis.z * zAxis.z);
	zAxis = { zAxis.x / lengthZ, zAxis.y / lengthZ, zAxis.z / lengthZ };

	// X軸 = Up × Z
	Vector3 xAxis = Cross(up, zAxis);
	float lengthX = sqrtf(xAxis.x * xAxis.x + xAxis.y * xAxis.y + xAxis.z * xAxis.z);
	xAxis = { xAxis.x / lengthX, xAxis.y / lengthX, xAxis.z / lengthX };

	// Y軸 = Z × X
	Vector3 yAxis = Cross(zAxis, xAxis);

	Matrix4x4 view{};
	view.m[0][0] = xAxis.x;
	view.m[1][0] = xAxis.y;
	view.m[2][0] = xAxis.z;
	view.m[3][0] = -(xAxis.x * eye.x + xAxis.y * eye.y + xAxis.z * eye.z);

	view.m[0][1] = yAxis.x;
	view.m[1][1] = yAxis.y;
	view.m[2][1] = yAxis.z;
	view.m[3][1] = -(yAxis.x * eye.x + yAxis.y * eye.y + yAxis.z * eye.z);

	view.m[0][2] = zAxis.x;
	view.m[1][2] = zAxis.y;
	view.m[2][2] = zAxis.z;
	view.m[3][2] = -(zAxis.x * eye.x + zAxis.y * eye.y + zAxis.z * eye.z);

	view.m[0][3] = 0.0f;
	view.m[1][3] = 0.0f;
	view.m[2][3] = 0.0f;
	view.m[3][3] = 1.0f;

	return view;
}
void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix) {
	const float kGridHalfWidth = 20.0f;
	const uint32_t kSubdivision = 20;
	const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

	for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
		float x = -kGridHalfWidth + kGridEvery * xIndex;
		Vector3 worldStart = { x + gGridPosition.x, gGridPosition.y, -kGridHalfWidth + gGridPosition.z };
		Vector3 worldEnd = { x + gGridPosition.x, gGridPosition.y,  kGridHalfWidth + gGridPosition.z };

		Vector3 screenStart = TransformWithW(TransformWithW(worldStart, viewProjectionMatrix), viewportMatrix);
		Vector3 screenEnd = TransformWithW(TransformWithW(worldEnd, viewProjectionMatrix), viewportMatrix);

		uint32_t color = (abs(x) < 0.001f) ? 0x000000FF : 0xAAAAAAFF;
		Novice::DrawLine(int(screenStart.x + 0.5f), int(screenStart.y + 0.5f),
			int(screenEnd.x + 0.5f), int(screenEnd.y + 0.5f), color);
	}

	for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
		float z = -kGridHalfWidth + kGridEvery * zIndex;
		Vector3 worldStart = { -kGridHalfWidth + gGridPosition.x, gGridPosition.y, z + gGridPosition.z };
		Vector3 worldEnd = { kGridHalfWidth + gGridPosition.x, gGridPosition.y, z + gGridPosition.z };

		Vector3 screenStart = TransformWithW(TransformWithW(worldStart, viewProjectionMatrix), viewportMatrix);
		Vector3 screenEnd = TransformWithW(TransformWithW(worldEnd, viewProjectionMatrix), viewportMatrix);

		uint32_t color = (abs(z) < 0.001f) ? 0x000000FF : 0xAAAAAAFF;
		Novice::DrawLine(int(screenStart.x + 0.5f), int(screenStart.y + 0.5f),
			int(screenEnd.x + 0.5f), int(screenEnd.y + 0.5f), color);
	}
}
Matrix4x4 MakeRotationMatrix(const Vector3& rotate) {
	float cosX = cosf(rotate.x), sinX = sinf(rotate.x);
	float cosY = cosf(rotate.y), sinY = sinf(rotate.y);
	float cosZ = cosf(rotate.z), sinZ = sinf(rotate.z);

	Matrix4x4 m{};

	m.m[0][0] = cosY * cosZ;
	m.m[0][1] = cosY * sinZ;
	m.m[0][2] = -sinY;
	m.m[0][3] = 0.0f;

	m.m[1][0] = sinX * sinY * cosZ - cosX * sinZ;
	m.m[1][1] = sinX * sinY * sinZ + cosX * cosZ;
	m.m[1][2] = sinX * cosY;
	m.m[1][3] = 0.0f;

	m.m[2][0] = cosX * sinY * cosZ + sinX * sinZ;
	m.m[2][1] = cosX * sinY * sinZ - sinX * cosZ;
	m.m[2][2] = cosX * cosY;
	m.m[2][3] = 0.0f;

	m.m[3][0] = 0.0f;
	m.m[3][1] = 0.0f;
	m.m[3][2] = 0.0f;
	m.m[3][3] = 1.0f;

	return m;
}
void DrawSphere(const Sphere& sphere, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
	const uint32_t kSubdivision = 16;
	const float kLonEvery = 2.0f * float(M_PI) / float(kSubdivision);
	const float kLatEvery = float(M_PI) / float(kSubdivision);

	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		float lat = -float(M_PI) / 2.0f + kLatEvery * (latIndex + 0.5f);

		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			float lon = lonIndex * kLonEvery;

			// world座標上の点a, b, cを計算
			Vector3 a = {
	            sphere.center.x + sphere.radius * cosf(lat) * cosf(lon),
	            sphere.center.y + sphere.radius * sinf(lat), // ←Yが上下方向
	            sphere.center.z + sphere.radius * cosf(lat) * sinf(lon)
			};

			Vector3 b = {
				sphere.center.x + sphere.radius * cosf(lat + kLatEvery) * cosf(lon),
				sphere.center.y + sphere.radius * sinf(lat + kLatEvery),
				sphere.center.z + sphere.radius * cosf(lat + kLatEvery) * sinf(lon)
			};
			Vector3 c = {
				sphere.center.x + sphere.radius * cosf(lat) * cosf(lon + kLonEvery),
				sphere.center.y + sphere.radius * sinf(lat),
				sphere.center.z + sphere.radius * cosf(lat) * sinf(lon + kLonEvery)
			};

			// a, b, c をスクリーン座標へ直接変換
			Vector3 screenA = TransformWithW(TransformWithW(a, viewProjectionMatrix), viewportMatrix);
			Vector3 screenB = TransformWithW(TransformWithW(b, viewProjectionMatrix), viewportMatrix);
			Vector3 screenC = TransformWithW(TransformWithW(c, viewProjectionMatrix), viewportMatrix);

			// 線を描画
			Novice::DrawLine(int(screenA.x), int(screenA.y), int(screenB.x), int(screenB.y), color);
			Novice::DrawLine(int(screenA.x), int(screenA.y), int(screenC.x), int(screenC.y), color);
		}
	}
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, 1280, 720);

	// キー入力結果を受け取る箱
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	// カメラ
	Vector3 cameraTranslate{ 0.0f, 1.9f, -6.49f };
	Vector3 cameraTarget{ 0.0f, 0.0f, 0.0f };
	Vector3 cameraUp{ 0.0f, 1.0f, 0.0f };
	Vector3 cameraRotate{ 0.0f,0.0f,0.0f };

	// 球の情報
	Sphere testSphere = { {0.0f, 1.0f, 10.0f}, 1.0f };

	// ウィンドウの×ボタンが押されるまでループ
	while (Novice::ProcessMessage() == 0) {
		// フレームの開始
		Novice::BeginFrame();

		// キー入力を受け取る
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		///
		/// ↓更新処理ここから
		///

		Matrix4x4 cameraMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, cameraTranslate);
		Matrix4x4 rotationMatrix = MakeRotationMatrix(cameraRotate);
		Matrix4x4 viewMatrix = MakeLookAtMatrix(cameraTranslate, cameraTarget, cameraUp);
		viewMatrix = Multiply(viewMatrix, rotationMatrix);
		Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kWindowWidth) / float(kWindowHeight), 0.1f, 100.0f);
		Matrix4x4 viewportMatrix = MakeViewportMatrix(0, 0, float(kWindowWidth), float(kWindowHeight), 0.0f, 1.0f);
		Matrix4x4 viewProjectionMatrix = Multiply(projectionMatrix, viewMatrix);

		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///

		ImGui::Begin("Window");
		ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.01f);
		ImGui::DragFloat3("CameraRotate", &cameraRotate.x, 0.01f);
		ImGui::DragFloat3("Center", &testSphere.center.x, 0.1f);    
		ImGui::DragFloat("Radius", &testSphere.radius, 0.1f, 0.1f);
		ImGui::End();

		DrawGrid(Multiply(projectionMatrix, viewMatrix), viewportMatrix);
		DrawSphere(testSphere, viewProjectionMatrix, viewportMatrix, 0xFFFFFFFF); 
		///
		/// ↑描画処理ここまで
		///

		// フレームの終了
		Novice::EndFrame();

		// ESCキーが押されたらループを抜ける
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	// ライブラリの終了
	Novice::Finalize();
	return 0;
}