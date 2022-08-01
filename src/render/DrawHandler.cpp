#include "DrawHandler.h"
#include "PrecisionHandler.h"
#include "Settings.h"
#include "Utils.h"
#include "render/common.h"
#include "render/timer.h"

void DrawHandler::Update(float a_delta)
{
	//GameTime::StepFrameTime();
	if (RE::UI::GetSingleton()->GameIsPaused()) {
		return;
	}

	_timer += a_delta;
}

void DrawHandler::Render(Render::D3DContext& ctx)
{
	//auto playerCamera = RE::PlayerCamera::GetSingleton();
	//auto currentCameraState = playerCamera->currentState;
	//if (!currentCameraState) {
	//	return;
	//}

	//

	//RE::NiPoint3 currentCameraPosition;
	//currentCameraState->GetTranslation(currentCameraPosition);
	//glm::vec3 cameraPosition(currentCameraPosition.x, currentCameraPosition.y, currentCameraPosition.z);

	//RE::NiQuaternion niq;
	//currentCameraState->GetRotation(niq);
	//glm::quat q {niq.w, niq.x, niq.y, niq.z};
	//glm::vec2 cameraRotation;
	//cameraRotation.x = glm::pitch(q) * -1.0f;
	//cameraRotation.y = glm::roll(q) * -1.0f;  // The game stores yaw in the Z axis

	//RE::NiPointer<RE::NiCamera> niCamera = RE::NiPointer<RE::NiCamera>(static_cast<RE::NiCamera*>(playerCamera->cameraRoot->children[0].get()));

	//const auto matProj = Render::GetProjectionMatrix(niCamera->viewFrustum);
	//const auto matView = Render::BuildViewMatrix(cameraPosition, cameraRotation);
	//const auto matWorldToCam = *reinterpret_cast<glm::mat4*>(g_worldToCamMatrix);

	//renderables.cbufPerFrameStaging.matProjView = matProj * matView;

	Locker locker(_lock);

	renderables.cbufPerFrameStaging.curTime = static_cast<float>(GameTime::CurTime());
	renderables.cbufPerFrame->Update(
		&renderables.cbufPerFrameStaging, 0,
		sizeof(decltype(renderables.cbufPerFrameStaging)), ctx);

	// Bind at register b1
	renderables.cbufPerFrame->Bind(Render::PipelineStage::Vertex, 1, ctx);
	renderables.cbufPerFrame->Bind(Render::PipelineStage::Fragment, 1, ctx);

	// Setup depth and blending
	Render::SetDepthState(ctx, true, true, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL);
	Render::SetBlendState(
		ctx, true,
		D3D11_BLEND_OP::D3D11_BLEND_OP_ADD, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
		D3D11_BLEND::D3D11_BLEND_SRC_ALPHA, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA,
		D3D11_BLEND::D3D11_BLEND_ONE, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA);

	if (renderables.lineSegments.size() > 0) {
		renderables.lineDrawer->Submit(renderables.lineSegments);
		//renderables.lineSegments.clear();
	}

	Render::SetDepthState(ctx, true, true, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS);

	if (renderables.drawOnTopLineSegments.size() > 0) {
		renderables.lineDrawer->Submit(renderables.drawOnTopLineSegments);
	}

	//renderables.colliders.erase(std::remove_if(renderables.colliders.begin(), renderables.colliders.end(), [this](Render::Collider collider) { return collider.timestamp + collider.duration < _timer; }), renderables.colliders.end());
	std::erase_if(renderables.colliders, [this](Render::Collider collider) { return collider.duration > 0 && collider.timestamp + collider.duration < _timer; });
	DrawDebugCapsules();

	std::erase_if(renderables.points, [this](Render::DrawnPoint point) { return point.duration > 0 && point.timestamp + point.duration < _timer; });
	std::erase_if(renderables.drawOnTopPoints, [this](Render::DrawnPoint point) { return point.duration > 0 && point.timestamp + point.duration < _timer; });
	DrawPoints();

	// remove expired lines
	//renderables.lineSegments.erase(std::remove_if(renderables.lineSegments.begin(), renderables.lineSegments.end(), [this](Render::Line line) { return line.timestamp + line.duration < _timer; }), renderables.lineSegments.end());
	std::erase_if(renderables.lineSegments, [this](Render::Line line) { return line.timestamp + line.duration < _timer; });
	//renderables.drawOnTopLineSegments.erase(std::remove_if(renderables.drawOnTopLineSegments.begin(), renderables.drawOnTopLineSegments.end(), [this](Render::Line line) { return line.timestamp + line.duration < _timer; }), renderables.drawOnTopLineSegments.end());
	std::erase_if(renderables.drawOnTopLineSegments, [this](Render::Line line) { return line.timestamp + line.duration < _timer; });
}

void DrawHandler::DrawDebugCapsules()
{
	for (auto& collider : renderables.colliders) {
		Utils::DrawCollider(collider.node.get(), 0.f, collider.color);
	}
}

void DrawHandler::DrawPoints()
{
	for (auto& point : renderables.points) {
		DrawDebugPoint(RE::NiPoint3(point.point.pos.x, point.point.pos.y, point.point.pos.z), 0.f, point.point.col);
	}

	for (auto& point : renderables.drawOnTopPoints) {
		DrawDebugPoint(RE::NiPoint3(point.point.pos.x, point.point.pos.y, point.point.pos.z), 0.f, point.point.col, true);
	}
}

void DrawHandler::DrawDebugLine(const RE::NiPoint3& a_start, const RE::NiPoint3& a_end, float a_duration, glm::vec4 a_color, bool a_drawOnTop /*= false */)
{
	glm::vec3 start{ a_start.x, a_start.y, a_start.z };
	glm::vec3 end{ a_end.x, a_end.y, a_end.z };
	double timestamp = DrawHandler::GetTime();

	RE::NiCamera::WorldPtToScreenPt3((float(*)[4])g_worldToCamMatrix, *g_viewPort, a_start, start.x, start.y, start.z, 1e-5f);
	start -= 0.5f;
	start *= 2.f;

	if (start.z < 0) {
		return;
	}

	RE::NiCamera::WorldPtToScreenPt3((float(*)[4])g_worldToCamMatrix, *g_viewPort, a_end, end.x, end.y, end.z, 1e-5f);
	end -= 0.5f;
	end *= 2.f;

	if (end.z < 0) {
		return;
	}

	Render::Line line(Render::Point(start, a_color), Render::Point(end, a_color), timestamp, a_duration);

	auto drawHandler = DrawHandler::GetSingleton();

	Locker locker(drawHandler->_lock);

	if (a_drawOnTop) {
		drawHandler->renderables.drawOnTopLineSegments.emplace_back(line);
	} else {
		drawHandler->renderables.lineSegments.emplace_back(line);
	}
}

void DrawHandler::DrawDebugPoint(const RE::NiPoint3& a_position, float a_duration, glm::vec4 a_color, bool a_drawOnTop /*= false*/)
{
	glm::vec3 position{ a_position.x, a_position.y, a_position.z };
	double timestamp = DrawHandler::GetTime();

	RE::NiCamera::WorldPtToScreenPt3((float(*)[4])g_worldToCamMatrix, *g_viewPort, a_position, position.x, position.y, position.z, 1e-5f);
	position -= 0.5f;
	position *= 2.f;

	if (position.z < 0) {
		return;
	}

	constexpr glm::vec3 offset1{ 0.005f, 0.005f, 0.f };
	constexpr glm::vec3 offset2{ 0.005f, -0.005f, 0.f };

	Render::Line line1(Render::Point(position - offset1, a_color), Render::Point(position + offset1, a_color), timestamp, a_duration);
	Render::Line line2(Render::Point(position - offset2, a_color), Render::Point(position + offset2, a_color), timestamp, a_duration);

	auto drawHandler = DrawHandler::GetSingleton();

	Locker locker(drawHandler->_lock);

	if (a_drawOnTop) {
		drawHandler->renderables.drawOnTopLineSegments.emplace_back(line1);
		drawHandler->renderables.drawOnTopLineSegments.emplace_back(line2);
	} else {
		drawHandler->renderables.lineSegments.emplace_back(line1);
		drawHandler->renderables.lineSegments.emplace_back(line2);
	}
}

void DrawHandler::DrawCircle(const RE::NiPoint3& a_base, const RE::NiPoint3& a_X, const RE::NiPoint3& a_Y, float a_radius, uint8_t a_numSides, float a_duration, glm::vec4 a_color, bool a_drawOnTop /*= false */)
{
	const float angleDelta = 2.0f * glm::pi<float>() / a_numSides;
	RE::NiPoint3 lastVertex = a_base + a_X * a_radius;

	for (int i = 0; i < a_numSides; i++) {
		const RE::NiPoint3 vertex = a_base + (a_X * cosf(angleDelta * (i + 1)) + a_Y * sinf(angleDelta * (i + 1))) * a_radius;
		DrawDebugLine(lastVertex, vertex, a_duration, a_color, a_drawOnTop);
		lastVertex = vertex;
	}
}

void DrawHandler::DrawHalfCircle(const RE::NiPoint3& a_base, const RE::NiPoint3& a_X, const RE::NiPoint3& a_Y, float a_radius, uint8_t a_numSides, float a_duration, glm::vec4 a_color, bool a_drawOnTop /*= false */)
{
	const float angleDelta = 2.0f * glm::pi<float>() / a_numSides;
	RE::NiPoint3 lastVertex = a_base + a_X * a_radius;

	for (int i = 0; i < (a_numSides / 2); i++) {
		const RE::NiPoint3 vertex = a_base + (a_X * cosf(angleDelta * (i + 1)) + a_Y * sinf(angleDelta * (i + 1))) * a_radius;
		DrawDebugLine(lastVertex, vertex, a_duration, a_color, a_drawOnTop);
		lastVertex = vertex;
	}
}

//void DrawHandler::DrawDebugCapsule(const RE::NiPoint3& a_center, float a_halfHeight, float a_radius, const RE::NiMatrix3& a_rotation, float a_duration, glm::vec4 a_color)
//{
//	constexpr int32_t collisionSides = 16;
//
//	/*glm::quat q{ a_rotation.w, a_rotation.x, a_rotation.y, a_rotation.z };
//	glm::mat4 axes = glm::mat4_cast(q);
//	RE::NiPoint3 xAxis{ axes[0][0], axes[0][1], axes[0][2] };
//	RE::NiPoint3 yAxis{ axes[1][0], axes[1][1], axes[1][2] };
//	RE::NiPoint3 zAxis{ axes[2][0], axes[2][1], axes[2][2] };*/
//	RE::NiPoint3 xAxis{ a_rotation.entry[0][0], a_rotation.entry[0][1], a_rotation.entry[0][2] };
//	RE::NiPoint3 yAxis{ a_rotation.entry[1][0], a_rotation.entry[1][1], a_rotation.entry[1][2] };
//	RE::NiPoint3 zAxis{ a_rotation.entry[2][0], a_rotation.entry[2][1], a_rotation.entry[2][2] };
//
//	// draw top and bottom circles
//	float halfAxis = fmax(a_halfHeight - a_radius, 1.f);
//	RE::NiPoint3 topEnd = a_center + zAxis * halfAxis;
//	RE::NiPoint3 bottomEnd = a_center - zAxis * halfAxis;
//
//	DrawCircle(topEnd, xAxis, yAxis, a_radius, collisionSides, a_duration, a_color);
//	DrawCircle(bottomEnd, xAxis, yAxis, a_radius, collisionSides, a_duration, a_color);
//
//	// draw caps
//	DrawHalfCircle(topEnd, yAxis, zAxis, a_radius, collisionSides, a_duration, a_color);
//	DrawHalfCircle(topEnd, xAxis, zAxis, a_radius, collisionSides, a_duration, a_color);
//
//	RE::NiPoint3 negZAxis = -zAxis;
//
//	DrawHalfCircle(bottomEnd, yAxis, negZAxis, a_radius, collisionSides, a_duration, a_color);
//	DrawHalfCircle(bottomEnd, xAxis, negZAxis, a_radius, collisionSides, a_duration, a_color);
//
//	// draw connected lines
//	DrawDebugLine(topEnd + xAxis * a_radius, bottomEnd + xAxis * a_radius, a_duration, a_color, true);
//	DrawDebugLine(topEnd - xAxis * a_radius, bottomEnd - xAxis * a_radius, a_duration, a_color, true);
//	DrawDebugLine(topEnd + yAxis * a_radius, bottomEnd + yAxis * a_radius, a_duration, a_color, true);
//	DrawDebugLine(topEnd - yAxis * a_radius, bottomEnd - yAxis * a_radius, a_duration, a_color, true);
//}

void DrawHandler::DrawDebugCapsule(const RE::NiPoint3& a_origin, const RE::NiPoint3& a_firstPoint, const RE::NiPoint3& a_secondPoint, float a_radius, const RE::NiMatrix3& a_rotation, float a_duration, glm::vec4 a_color, bool a_drawOnTop /*= false */)
{
	constexpr int32_t collisionSides = 16;

	RE::NiPoint3 x{ 1.f, 0.f, 0.f };
	RE::NiPoint3 y{ 0.f, 1.f, 0.f };
	RE::NiPoint3 z{ 0.f, 0.f, 1.f };

	// draw top and bottom circles
	auto rotatedFirstPoint = Utils::TransformVectorByMatrix(a_firstPoint, a_rotation);
	RE::NiPoint3 topEnd = a_origin + rotatedFirstPoint;
	auto rotatedSecondPoint = Utils::TransformVectorByMatrix(a_secondPoint, a_rotation);
	RE::NiPoint3 bottomEnd = a_origin + rotatedSecondPoint;

	x = Utils::TransformVectorByMatrix(x, a_rotation);
	y = Utils::TransformVectorByMatrix(y, a_rotation);
	z = Utils::TransformVectorByMatrix(z, a_rotation);

	// fix debug display for horizontal capsules
	auto xAxis = x;
	auto yAxis = y;
	auto zAxis = rotatedFirstPoint - rotatedSecondPoint;
	zAxis.Unitize();
	if (fabs(zAxis.Dot(xAxis)) > 0.95f) {
		xAxis = zAxis.Cross(yAxis);
		xAxis.Unitize();
	} else if (fabs(zAxis.Dot(yAxis)) > 0.95f) {
		yAxis = zAxis.Cross(xAxis);
		yAxis.Unitize();
	}

	DrawCircle(topEnd, xAxis, yAxis, a_radius, collisionSides, a_duration, a_color, a_drawOnTop);
	DrawCircle(bottomEnd, xAxis, yAxis, a_radius, collisionSides, a_duration, a_color, a_drawOnTop);

	// draw caps
	DrawHalfCircle(topEnd, yAxis, zAxis, a_radius, collisionSides, a_duration, a_color, a_drawOnTop);
	DrawHalfCircle(topEnd, xAxis, zAxis, a_radius, collisionSides, a_duration, a_color, a_drawOnTop);
	DrawHalfCircle(bottomEnd, yAxis, -zAxis, a_radius, collisionSides, a_duration, a_color, a_drawOnTop);
	DrawHalfCircle(bottomEnd, xAxis, -zAxis, a_radius, collisionSides, a_duration, a_color, a_drawOnTop);

	// draw connected lines
	DrawDebugLine(topEnd + xAxis * a_radius, bottomEnd + xAxis * a_radius, a_duration, a_color, a_drawOnTop);
	DrawDebugLine(topEnd - xAxis * a_radius, bottomEnd - xAxis * a_radius, a_duration, a_color, a_drawOnTop);
	DrawDebugLine(topEnd + yAxis * a_radius, bottomEnd + yAxis * a_radius, a_duration, a_color, a_drawOnTop);
	DrawDebugLine(topEnd - yAxis * a_radius, bottomEnd - yAxis * a_radius, a_duration, a_color, a_drawOnTop);

	//draw bone axis
	auto capsuleCenter = (topEnd + bottomEnd) / 2;
	constexpr glm::vec4 xColor{ 1, 0, 0, 1 };
	constexpr glm::vec4 yColor{ 0, 1, 0, 1 };
	constexpr glm::vec4 zColor{ 0, 0, 1, 1 };
	DrawDebugLine(capsuleCenter, capsuleCenter + x * 5.f, a_duration, xColor, a_drawOnTop);
	DrawDebugLine(capsuleCenter, capsuleCenter + y * 5.f, a_duration, yColor, a_drawOnTop);
	DrawDebugLine(capsuleCenter, capsuleCenter + z * 5.f, a_duration, zColor, a_drawOnTop);
}

void DrawHandler::DrawDebugSphere(const RE::NiPoint3& a_center, float a_radius, float a_duration, glm::vec4 a_color, bool a_drawOnTop /*= false */)
{
	constexpr int32_t collisionSides = 16;

	constexpr RE::NiPoint3 xAxis{ 1.f, 0.f, 0.f };
	constexpr RE::NiPoint3 yAxis{ 0.f, 1.f, 0.f };
	constexpr RE::NiPoint3 zAxis{ 0.f, 0.f, 1.f };

	DrawCircle(a_center, xAxis, yAxis, a_radius, collisionSides, a_duration, a_color, a_drawOnTop);
	DrawCircle(a_center, xAxis, zAxis, a_radius, collisionSides, a_duration, a_color, a_drawOnTop);
	DrawCircle(a_center, yAxis, zAxis, a_radius, collisionSides, a_duration, a_color, a_drawOnTop);
}

void DrawHandler::DrawSweptCapsule(const RE::NiPoint3& a_a, const RE::NiPoint3& a_b, const RE::NiPoint3& a_c, const RE::NiPoint3& a_d, [[maybe_unused]] float a_radius, [[maybe_unused]] const RE::NiMatrix3& a_rotation, float a_duration, glm::vec4 a_color, bool a_drawOnTop /*= false */)
{
	DrawDebugLine(a_a, a_b, a_duration, a_color, a_drawOnTop);
	DrawDebugLine(a_b, a_c, a_duration, a_color, a_drawOnTop);
	DrawDebugLine(a_c, a_d, a_duration, a_color, a_drawOnTop);
	DrawDebugLine(a_d, a_a, a_duration, a_color, a_drawOnTop);

	//constexpr int32_t collisionSides = 16;

	//RE::NiPoint3 x{ 1.f, 0.f, 0.f };
	//RE::NiPoint3 y{ 0.f, 1.f, 0.f };
	//RE::NiPoint3 z{ 0.f, 0.f, 1.f };
	//x = Utils::TransformVectorByMatrix(x, a_rotation);
	//y = Utils::TransformVectorByMatrix(y, a_rotation);
	//z = Utils::TransformVectorByMatrix(z, a_rotation);

	//// top
	//DrawDebugLine(a_a + z * a_radius, a_b + z * a_radius, a_duration, a_color, true);
	//DrawDebugLine(a_b + z * a_radius, a_c + z * a_radius, a_duration, a_color, true);
	//DrawDebugLine(a_c + z * a_radius, a_d + z * a_radius, a_duration, a_color, true);
	//DrawDebugLine(a_d + z * a_radius, a_a + z * a_radius, a_duration, a_color, true);

	//// bottom
	//DrawDebugLine(a_a - z * a_radius, a_b - z * a_radius, a_duration, a_color, true);
	//DrawDebugLine(a_b - z * a_radius, a_c - z * a_radius, a_duration, a_color, true);
	//DrawDebugLine(a_c - z * a_radius, a_d - z * a_radius, a_duration, a_color, true);
	//DrawDebugLine(a_d - z * a_radius, a_a - z * a_radius, a_duration, a_color, true);

	//// sides
	//DrawDebugLine(a_a - x * a_radius, a_b - x * a_radius, a_duration, a_color, true);
	//DrawDebugLine(a_b + y * a_radius, a_c + y * a_radius, a_duration, a_color, true);
	//DrawDebugLine(a_c + x * a_radius, a_d + x * a_radius, a_duration, a_color, true);
	//DrawDebugLine(a_d - y * a_radius, a_a - y * a_radius, a_duration, a_color, true);
}

void DrawHandler::AddCollider(const RE::NiPointer<RE::NiAVObject> a_node, float a_duration, glm::vec4 a_color)
{
	double timestamp = DrawHandler::GetTime();
	Render::Collider collider(a_node, timestamp, a_duration, a_color);
	auto drawHandler = DrawHandler::GetSingleton();

	Locker locker(drawHandler->_lock);
	if (std::find(drawHandler->renderables.colliders.begin(), drawHandler->renderables.colliders.end(), collider) == drawHandler->renderables.colliders.end()) {
		drawHandler->renderables.colliders.emplace_back(collider);
	}
}

void DrawHandler::RemoveCollider(const RE::NiPointer<RE::NiAVObject> a_node)
{
	auto drawHandler = DrawHandler::GetSingleton();

	Locker locker(drawHandler->_lock);
	auto it = std::find(drawHandler->renderables.colliders.begin(), drawHandler->renderables.colliders.end(), a_node);
	if (it != drawHandler->renderables.colliders.end()) {
		drawHandler->renderables.colliders.erase(it);
	}
}

void DrawHandler::AddPoint(const RE::NiPoint3& a_point, float a_duration, glm::vec4 a_color, bool a_bDrawOnTop /*= false*/)
{
	double timestamp = DrawHandler::GetTime();
	Render::DrawnPoint point(Render::Point(glm::vec3(a_point.x, a_point.y, a_point.z), a_color), timestamp, a_duration);
	auto drawHandler = DrawHandler::GetSingleton();

	Locker locker(drawHandler->_lock);
	if (a_bDrawOnTop) {
		drawHandler->renderables.drawOnTopPoints.emplace_back(point);
	} else {
		drawHandler->renderables.points.emplace_back(point);
	}
}

double DrawHandler::GetTime()
{
	return DrawHandler::GetSingleton()->_timer;
}

void DrawHandler::OnPreLoadGame()
{
	renderables.ClearLists();
}

void DrawHandler::OnSettingsUpdated()
{
	// Hook only if debug display is enabled
	if (Settings::bDebug && (Settings::bDisplayWeaponCapsule || Settings::bDisplayHitNodeCollisions || Settings::bDisplayHitLocations || Settings::bDisplayIframeHits || Settings::bDisplayRecoilCollisions || Settings::bDisplaySkeletonColliders)) {
		if (!_bDXHooked) {
			Render::InstallHooks();
			if (Render::HasContext()) {
				_bDXHooked = true;
				Initialize();
			} else {
				logger::critical("Precision: Failed to hook DirectX, Rendering features will be disabled. Try running with overlay software disabled if this warning keeps occurring.");
			}
		}
	}
}

DrawHandler::DrawHandler() :
	_lock()
{}

void DrawHandler::Initialize()
{
	if (!_bDXHooked)
		return;
	auto& ctx = Render::GetContext();

	// Per-object data, changing each draw call (model)
	Render::CBufferCreateInfo perObj;
	perObj.bufferUsage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
	perObj.cpuAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
	perObj.size = sizeof(decltype(renderables.cbufPerObjectStaging));
	perObj.initialData = &renderables.cbufPerObjectStaging;
	renderables.cbufPerObject = std::make_shared<Render::CBuffer>(perObj, ctx);

	// Per-frame data, shared among many objects (view, projection)
	Render::CBufferCreateInfo perFrane;
	perFrane.bufferUsage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
	perFrane.cpuAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
	perFrane.size = sizeof(decltype(renderables.cbufPerFrameStaging));
	perFrane.initialData = &renderables.cbufPerFrameStaging;
	renderables.cbufPerFrame = std::make_shared<Render::CBuffer>(perFrane, ctx);

	renderables.lineDrawer = std::make_unique<Render::LineDrawer>(ctx);

	if (Render::HasContext()) {
		Render::OnPresent(std::bind(&DrawHandler::Render, this, std::placeholders::_1));
	}
}
