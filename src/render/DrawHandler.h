#pragma once
#include "render/cbuffer.h"
#include "render/line_drawer.h"

#include <glm/gtc/matrix_transform.hpp>

class DrawHandler
{
public:
	static DrawHandler* GetSingleton()
	{
		static DrawHandler singleton;
		return std::addressof(singleton);
	}

	void Update(float a_delta);
	void Render(Render::D3DContext& ctx);

	void DrawDebugCapsules();
	void DrawPoints();

	static void DrawDebugLine(const RE::NiPoint3& a_start, const RE::NiPoint3& a_end, float a_duration, glm::vec4 a_color, bool a_drawOnTop = false);
	static void DrawDebugPoint(const RE::NiPoint3& a_position, float a_duration, glm::vec4 a_color, bool a_drawOnTop = false);
	static void DrawCircle(const RE::NiPoint3& a_base, const RE::NiPoint3& a_X, const RE::NiPoint3& a_Y, float a_radius, uint8_t a_numSides, float a_duration, glm::vec4 a_color, bool a_drawOnTop = false);
	static void DrawHalfCircle(const RE::NiPoint3& a_base, const RE::NiPoint3& a_X, const RE::NiPoint3& a_Y, float a_radius, uint8_t a_numSides, float a_duration, glm::vec4 a_color, bool a_drawOnTop = false);
	static void DrawDebugCapsule(const RE::NiPoint3& a_vertexA, const RE::NiPoint3& a_vertexB, float a_radius, float a_duration, glm::vec4 a_color, bool a_drawOnTop = false);
	static void DrawDebugSphere(const RE::NiPoint3& a_center, float a_radius, float a_duration, glm::vec4 a_color, bool a_drawOnTop = false);

	static void DrawSweptCapsule(const RE::NiPoint3& a_a, const RE::NiPoint3& a_b, const RE::NiPoint3& a_c, const RE::NiPoint3& a_d, float a_radius, const RE::NiMatrix3& a_rotation, float a_duration, glm::vec4 a_color, bool a_drawOnTop = false);

	static void AddCollider(const RE::NiPointer<RE::NiAVObject> a_node, float a_duration, glm::vec4 a_color);
	static void RemoveCollider(const RE::NiPointer<RE::NiAVObject> a_node);

	static void AddPoint(const RE::NiPoint3& a_point, float a_duration, glm::vec4 a_color, bool a_bDrawOnTop = false);

	static double GetTime();

	void OnPreLoadGame();
	void OnSettingsUpdated();

private:
	using Lock = std::recursive_mutex;
	using Locker = std::lock_guard<Lock>;

	DrawHandler();
	DrawHandler(const DrawHandler&) = delete;
	DrawHandler(DrawHandler&&) = delete;
	virtual ~DrawHandler() = default;

	DrawHandler& operator=(const DrawHandler&) = delete;
	DrawHandler& operator=(DrawHandler&&) = delete;

	mutable Lock _lock;

	void Initialize();

	bool _bDXHooked = false;

	// Renderable objects
	struct
	{
		// Data which should really only change once each frame
		struct VSMatricesCBuffer
		{
			glm::mat4 matProjView = glm::identity<glm::mat4>();
			float curTime = 0.0f;
			float pad[3] = { 0.0f, 0.0f, 0.0f };
		};
		static_assert(sizeof(VSMatricesCBuffer) % 16 == 0);

		// Data which is likely to change each draw call
		struct VSPerObjectCBuffer
		{
			glm::mat4 model = glm::identity<glm::mat4>();
		};
		static_assert(sizeof(VSMatricesCBuffer) % 16 == 0);

		VSMatricesCBuffer cbufPerFrameStaging = {};
		VSPerObjectCBuffer cbufPerObjectStaging = {};
		std::shared_ptr<Render::CBuffer> cbufPerFrame;
		std::shared_ptr<Render::CBuffer> cbufPerObject;

		// Resources for drawing the debug line
		Render::LineList lineSegments;
		Render::LineList drawOnTopLineSegments;
		std::unique_ptr<Render::LineDrawer> lineDrawer;

		Render::PointList points;
		Render::PointList drawOnTopPoints;
		Render::ColliderList colliders;

		void ClearLists()
		{
			lineSegments.clear();
			drawOnTopLineSegments.clear();
			points.clear();
			drawOnTopPoints.clear();
			colliders.clear();
		}

		// D3D expects resources to be released in a certain order
		void release()
		{
			lineDrawer.reset();
			cbufPerObject.reset();
			cbufPerFrame.reset();
		}
	} renderables;

	double _timer = 0.0;
};
