#pragma once
#include "Offsets.h"
#include "render/shader.h"
#include "render/timer.h"
#include "render/vertex_buffer.h"

namespace Render
{
	typedef struct Point
	{
		glm::vec4 pos;
		glm::vec4 col;

		explicit Point(glm::vec3 position, glm::vec4 color) :
			col(color)
		{
			pos = { position.x, position.y, position.z, 1.0f };
		}
	} Point;

	typedef struct DrawnPoint
	{
		Point point;
		float duration;
		double timestamp;

		DrawnPoint(Point&& a_point, double& a_timestamp, float& a_duration) :
			point(a_point), timestamp(a_timestamp), duration(a_duration){};
	} DrawnPoint;

	typedef struct Line
	{
		Point start;
		Point end;
		float duration;
		double timestamp;
		Line(Point&& a_start, Point&& a_end, double& a_timestamp, float& a_duration) :
			start(a_start), end(a_end), timestamp(a_timestamp), duration(a_duration){
				//timestamp = static_cast<float>(GameTime::CurTime());
			};
	} Line;

	typedef struct Collider
	{
		RE::NiPointer<RE::NiAVObject> node;
		float duration;
		double timestamp;
		glm::vec4 color;
		Collider(RE::NiPointer<RE::NiAVObject> node, double& timestamp, float& duration, glm::vec4& color) :
			node(node), timestamp(timestamp), duration(duration), color(color){};

		[[nodiscard]] friend bool operator==(const Collider& a_lhs, const Collider& a_rhs) noexcept
		{
			return a_lhs.node == a_rhs.node;
		}

		[[nodiscard]] friend bool operator==(const Collider& a_this, const RE::NiPointer<RE::NiAVObject> a_node) noexcept
		{
			return a_this.node == a_node;
		}
	} Collider;

	using LineList = std::vector<Line>;
	using PointList = std::vector<DrawnPoint>;
	using ColliderList = std::vector<Collider>;

	// Number of points we can submit in a single draw call
	constexpr size_t LineDrawPointBatchSize = 64;
	// Number of buffers to use
	constexpr size_t NumBuffers = 2;

	class LineDrawer
	{
	public:
		explicit LineDrawer(D3DContext& ctx);
		~LineDrawer();
		LineDrawer(const LineDrawer&) = delete;
		LineDrawer(LineDrawer&&) noexcept = delete;
		LineDrawer& operator=(const LineDrawer&) = delete;
		LineDrawer& operator=(LineDrawer&&) noexcept = delete;

		// Submit a list of lines for drawing
		void Submit(const LineList& lines) noexcept;

	protected:
		std::shared_ptr<Render::Shader> vs;
		std::shared_ptr<Render::Shader> ps;

	private:
		std::array<std::unique_ptr<Render::VertexBuffer>, NumBuffers> vbo;

		void CreateObjects(D3DContext& ctx);
		void DrawBatch(uint32_t bufferIndex, LineList::const_iterator& begin, LineList::const_iterator& end);
	};
}
