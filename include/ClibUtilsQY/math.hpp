#pragma once

#include <numbers>
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

namespace ClibUtilsQY {
	namespace {
		UINT GetBufferLength(RE::ID3D11Buffer* reBuffer) {
			const auto buffer = reinterpret_cast<ID3D11Buffer*>(reBuffer);
			D3D11_BUFFER_DESC bufferDesc = {};
			buffer->GetDesc(&bufferDesc);
			return bufferDesc.ByteWidth;
		}

		void EachGeometry(const RE::TESObjectREFR* obj, const std::function<void(RE::BSGeometry* o3d, RE::BSGraphics::TriShape*)>& callback) {
			if (const auto d3d = obj->Get3D()) {

				RE::BSVisit::TraverseScenegraphGeometries(d3d, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {

					const auto& model = a_geometry->GetGeometryRuntimeData();

					if (const auto triShape = model.rendererData) {
						callback(a_geometry, triShape);
					}

					return RE::BSVisit::BSVisitControl::kContinue;
				});

			} 
		}
	};

    namespace Math {
		// Credits: https://github.com/Thiago099/Better-Grabbing/blob/main/include/Geometry.h
		class Geometry {
			std::vector<RE::NiPoint3> positions;
			std::vector<uint16_t> indexes;
			const RE::TESObjectREFR* obj;

			void FetchVertices(const RE::BSGeometry* o3d, RE::BSGraphics::TriShape* triShape);


		public:
			static RE::NiPoint3 Rotate(const RE::NiPoint3& A, const RE::NiPoint3& angles);

			~Geometry() = default;
			explicit Geometry(const RE::TESObjectREFR* obj);
			[[nodiscard]] std::pair<RE::NiPoint3, RE::NiPoint3> GetBoundingBox() const;
		};

        namespace LinAlg {
            static RE::NiPoint3 angles2dir(const RE::NiPoint3& angles) {
                RE::NiPoint3 ans;

                const float sinx = sinf(angles.x);
                const float cosx = cosf(angles.x);
                const float sinz = sinf(angles.z);
                const float cosz = cosf(angles.z);

                ans.x = cosx * sinz;
                ans.y = cosx * cosz;
                ans.z = -sinx;

                return ans;
            }

            static RE::NiPoint3 rotate(const RE::NiPoint3& A, const RE::NiPoint3& angles) {
                RE::NiMatrix3 R;
                R.SetEulerAnglesXYZ(angles);
                return R * A;
            }

            static RE::NiPoint3 rotate(const float r, const RE::NiPoint3& angles) { 
                return angles2dir(angles) * r;
            }

            std::array<RE::NiPoint3,3> GetClosest3Vertices(const std::array<RE::NiPoint3, 8>& a_bounding_box, const RE::NiPoint3& outside_point);
            std::array<RE::NiPoint3,3> GetClosest3Vertices(const std::array<RE::NiPoint3, 4>& a_bounded_plane, const RE::NiPoint3& outside_point);
            RE::NiPoint3 GetMiddlePointFromTriangle(const std::array<RE::NiPoint3,3>& a_array);
            RE::NiPoint3 CalculateCenter(const RE::TESObjectREFR* a_obj);
		    RE::NiPoint3 CalculateNormalOfPlane(const RE::NiPoint3& span1, const RE::NiPoint3& span2);
		    RE::NiPoint3 closestPointOnPlane(const RE::NiPoint3& a_point_on_plane,const RE::NiPoint3& a_point_not_on_plane, const RE::NiPoint3& v_normal);
            RE::NiPoint3 intersectLine(const std::array<RE::NiPoint3,3>& vertices, const RE::NiPoint3& outside_plane_point);

            RE::NiPoint3 RotateVector(const RE::NiPoint3& a_vec, const RE::NiQuaternion& a_quat);

            inline RE::NiPoint3 GetForwardVector(const RE::NiQuaternion& a_quat) {
		        return RotateVector({ 0.f, 1.f, 0.f }, a_quat);
	        }
            
        };

		namespace WorldObjects {
			std::array<RE::NiPoint3, 8> GetBoundingBox(const RE::TESObjectREFR* a_obj);
			bool IsInBoundingBox(const std::array<RE::NiPoint3, 8>& a_box, const RE::NiPoint3& a_point);
			RE::NiPoint3 GetClosestPoint(const RE::TESObjectREFR* a_obj_from, const RE::TESObjectREFR* a_obj_to);
			RE::NiPoint3 GetClosestPoint(const RE::NiPoint3& a_point_from , const RE::TESObjectREFR* a_obj_to);
		};
    };
};


std::array<RE::NiPoint3, 3> ClibUtilsQY::Math::LinAlg::GetClosest3Vertices(const std::array<RE::NiPoint3, 8>& a_bounding_box, const RE::NiPoint3& outside_point)
{
    std::array<RE::NiPoint3, 3> result;

    std::vector<std::pair<float,size_t>> distances;
    for (size_t i = 0; i < a_bounding_box.size(); ++i) {
		float dist = outside_point.GetDistance(a_bounding_box[i]);
        distances.push_back({dist,i});
    }
    std::ranges::sort(distances);
    for (size_t i = 0; i < 3 && i < distances.size(); ++i) {
        const size_t index = distances[i].second;
		result[i] = a_bounding_box[index];
    }
    
	return result;
}

std::array<RE::NiPoint3, 3> ClibUtilsQY::Math::LinAlg::GetClosest3Vertices(const std::array<RE::NiPoint3, 4>& a_bounded_plane, const RE::NiPoint3& outside_point){
	std::array<RE::NiPoint3, 3> result;

    std::vector<std::pair<float,size_t>> distances;
    for (size_t i = 0; i < a_bounded_plane.size(); ++i) {
		float dist = outside_point.GetDistance(a_bounded_plane[i]);
        distances.push_back({dist,i});
    }
    std::ranges::sort(distances);
    for (size_t i = 0; i < 3 && i < distances.size(); ++i) {
        const size_t index = distances[i].second;
		result[i] = a_bounded_plane[index];
    }
    
	return result;
}

RE::NiPoint3 ClibUtilsQY::Math::LinAlg::GetMiddlePointFromTriangle(const std::array<RE::NiPoint3,3>& a_array)
{
    const RE::NiPoint3 AtoB = a_array[1] - a_array[0];
	const RE::NiPoint3 BtoC = a_array[2] - a_array[1];
	const RE::NiPoint3 CtoA = a_array[0] - a_array[2];
	// calculate magnitude of each vector
	float magAtoB = AtoB.Length();
	float magBtoC = BtoC.Length();
	float magCtoA = CtoA.Length();

    const std::array vertices = {AtoB,BtoC,CtoA};
	const std::vector<std::pair<float, size_t>> distances = {
				{ magAtoB,0 },
	            { magBtoC,1 },
	            { magCtoA,2 }
	};
    size_t index = 0;
	for (size_t i = 0; i < distances.size(); ++i) {
		if (distances[i].first > distances[index].first) {
			index = i;
		}
	}
	//const auto longest = vertices[index];
	return vertices[index]/2 + a_array[index];
}

RE::NiPoint3 ClibUtilsQY::Math::LinAlg::CalculateCenter(const RE::TESObjectREFR* a_obj)
{
    const auto vertices = WorldObjects::GetBoundingBox(a_obj);
	RE::NiPoint3 sum = { 0,0,0 };
	for (const auto& vertex : vertices) {
		sum += vertex;
	}
	return sum / static_cast<float>(vertices.size());
}

RE::NiPoint3 ClibUtilsQY::Math::LinAlg::CalculateNormalOfPlane(const RE::NiPoint3& span1, const RE::NiPoint3& span2)
{
    const auto crossed = span1.Cross(span2);
	const auto length = crossed.Length();
	if (fabs(length)<EPSILON) {
		return { 0,0,0 };
	}
	return crossed / length;
}

RE::NiPoint3 ClibUtilsQY::Math::LinAlg::closestPointOnPlane(const RE::NiPoint3& a_point_on_plane, const RE::NiPoint3& a_point_not_on_plane, const RE::NiPoint3& v_normal)
{
    const auto distance = (a_point_not_on_plane - a_point_on_plane).Dot(v_normal);
	return a_point_not_on_plane - v_normal * distance;
}

RE::NiPoint3 ClibUtilsQY::Math::LinAlg::intersectLine(const std::array<RE::NiPoint3, 3>& vertices, const RE::NiPoint3& outside_plane_point)
{
    const RE::NiPoint3 AtoB = vertices[1] - vertices[0];
	const RE::NiPoint3 BtoC = vertices[2] - vertices[1];
	const RE::NiPoint3 CtoA = vertices[0] - vertices[2];
	// calculate magnitude of each vector
	float magAtoB = AtoB.Length();
	float magBtoC = BtoC.Length();
	float magCtoA = CtoA.Length();

    const std::vector<RE::NiPoint3> edges = {AtoB,BtoC,CtoA};
	const std::vector<std::pair<float, size_t>> distances = {
				{ magAtoB,0 },
	            { magBtoC,1 },
	            { magCtoA,2 }
	};

    size_t index = 0;
	for (size_t i = 0; i < distances.size(); ++i) {
		if (distances[i].first > distances[index].first) {
			index = i;
		}
	}

	[[maybe_unused]] const auto& hypotenuse = edges[index];
	const auto index1 = (index + 1) % 3;
	const auto index2 = (index + 2) % 3;

	const auto& orthogonal_vertex = vertices[index2]; // B

	for (const auto a_index : {index,index1}) {
	    const auto& hypotenuse_vertex = vertices[a_index]; // C or A depending on closed loop orientation
	    const auto distance_vector = outside_plane_point - hypotenuse_vertex;
	    auto temp = orthogonal_vertex-hypotenuse_vertex;
		auto other_hypotenuse_vertex = a_index == index1 ? vertices[index] : vertices[index1];
		auto temp2 = other_hypotenuse_vertex - orthogonal_vertex;
		const auto theta_max = atan(temp2.Length()/temp.Length());
	    const auto theta = acos(distance_vector.Dot(temp) / (distance_vector.Length() * temp.Length()));
	    if (0.f <= theta && theta <= theta_max) {
			if (temp2.Dot(distance_vector) > 0) {
			    const auto a_span_size = tan(theta)*temp.Length();
			    const auto intersect = temp + temp2/temp2.Length() * a_span_size;
				if (intersect.Length()>distance_vector.Length()) {
					return outside_plane_point; // it is inside the triangle
				}
				const auto normal_distance = (outside_plane_point-orthogonal_vertex).Dot(temp2/temp2.Length());
				return temp2 / temp2.Length() * normal_distance + orthogonal_vertex;
			}
	    }
	    
	}

	return orthogonal_vertex;
}

RE::NiPoint3 ClibUtilsQY::Math::LinAlg::RotateVector(const RE::NiPoint3& a_vec, const RE::NiQuaternion& a_quat)
{
	//http://people.csail.mit.edu/bkph/articles/Quaternions.pdf
	const RE::NiPoint3 Q{ a_quat.x, a_quat.y, a_quat.z };
	const RE::NiPoint3 T = Q.Cross(a_vec) * 2.f;
	return a_vec + (T * a_quat.w) + Q.Cross(T);
}

void ClibUtilsQY::Math::Geometry::FetchVertices(const RE::BSGeometry * o3d, RE::BSGraphics::TriShape * triShape)
{
	if (const uint8_t* vertexData = triShape->rawVertexData) {
        const uint32_t stride = triShape->vertexDesc.GetSize();
        const auto numPoints = GetBufferLength(triShape->vertexBuffer);
        const auto numPositions = numPoints / stride;
        positions.reserve(positions.size() + numPositions);
        for (uint32_t i = 0; i < numPoints; i += stride) {
            const uint8_t* currentVertex = vertexData + i;

            const auto position =
                reinterpret_cast<const float*>(currentVertex + triShape->vertexDesc.GetAttributeOffset(
                                                                   RE::BSGraphics::Vertex::Attribute::VA_POSITION));

            auto pos = RE::NiPoint3{position[0], position[1], position[2]};
            pos = o3d->local * pos;
            positions.push_back(pos);
        }
    }
}

inline RE::NiPoint3 ClibUtilsQY::Math::Geometry::Rotate(const RE::NiPoint3& A, const RE::NiPoint3& angles)
{
	RE::NiMatrix3 R;
    R.SetEulerAnglesXYZ(angles);
    return R * A;
}

inline ClibUtilsQY::Math::Geometry::Geometry(const RE::TESObjectREFR* obj)
{
	this->obj = obj;
    EachGeometry(obj, [this](const RE::BSGeometry* o3d, RE::BSGraphics::TriShape* triShape) -> void {
        FetchVertices(o3d, triShape);
        //FetchIndexes(triShape);
    });

    if (positions.size() == 0) {
        auto from = obj->GetBoundMin();
        auto to = obj->GetBoundMax();

        if ((to - from).Length() < 1) {
            from = {-5, -5, -5};
            to = {5, 5, 5};
        }
        positions.push_back(RE::NiPoint3(from.x, from.y, from.z));
        positions.push_back(RE::NiPoint3(to.x, from.y, from.z));
        positions.push_back(RE::NiPoint3(to.x, to.y, from.z));
        positions.push_back(RE::NiPoint3(from.x, to.y, from.z));

        positions.push_back(RE::NiPoint3(from.x, from.y, to.z));
        positions.push_back(RE::NiPoint3(to.x, from.y, to.z));
        positions.push_back(RE::NiPoint3(to.x, to.y, to.z));
        positions.push_back(RE::NiPoint3(from.x, to.y, to.z));
    }
}

inline std::pair<RE::NiPoint3, RE::NiPoint3> ClibUtilsQY::Math::Geometry::GetBoundingBox() const
{
	auto min = RE::NiPoint3{0, 0, 0};
    auto max = RE::NiPoint3{0, 0, 0};

    for (auto i = 0; i < positions.size(); i++) {
        //const auto p1 = Rotate(positions[i] * scale, angle);
        const auto p1 = positions[i] * obj->GetScale();

        if (p1.x < min.x) {
            min.x = p1.x;
        }
        if (p1.x > max.x) {
            max.x = p1.x;
        }
        if (p1.y < min.y) {
            min.y = p1.y;
        }
        if (p1.y > max.y) {
            max.y = p1.y;
        }
        if (p1.z < min.z) {
            min.z = p1.z;
        }
        if (p1.z > max.z) {
            max.z = p1.z;
        }
    }

    return std::pair(min, max);
}

std::array<RE::NiPoint3,8> ClibUtilsQY::Math::WorldObjects::GetBoundingBox(const RE::TESObjectREFR * a_obj)
{
	//using namespace Math::LinAlg;
    const auto center = a_obj->GetPosition();
	const Geometry geometry(a_obj);
	auto [min, max] = geometry.GetBoundingBox();

	min = center + min;
	max = center + max;

	const auto obj_angle = a_obj->GetAngle();

	const auto v1 = Geometry::Rotate(RE::NiPoint3(min.x, min.y, min.z) - center, obj_angle) + center;
	const auto v2 = Geometry::Rotate(RE::NiPoint3(max.x, min.y, min.z) - center, obj_angle) + center;
	const auto v3 = Geometry::Rotate(RE::NiPoint3(max.x, max.y, min.z) - center, obj_angle) + center;
	const auto v4 = Geometry::Rotate(RE::NiPoint3(min.x, max.y, min.z) - center, obj_angle) + center;

	const auto v5 = Geometry::Rotate(RE::NiPoint3(min.x, min.y, max.z) - center, obj_angle) + center;
	const auto v6 = Geometry::Rotate(RE::NiPoint3(max.x, min.y, max.z) - center, obj_angle) + center;
	const auto v7 = Geometry::Rotate(RE::NiPoint3(max.x, max.y, max.z) - center, obj_angle) + center;
	const auto v8 = Geometry::Rotate(RE::NiPoint3(min.x, max.y, max.z) - center, obj_angle) + center;

	//logger::trace("v1 ({},{},{}), v2 ({},{},{}), v3 ({},{},{}), v4 ({},{},{}), v5 ({},{},{}), v6 ({},{},{}), v7 ({},{},{}), v8 ({},{},{})",
	//		v1.x, v1.y, v1.z,
	//		v2.x, v2.y, v2.z,
	//		v3.x, v3.y, v3.z,
	//		v4.x, v4.y, v4.z,
	//		v5.x, v5.y, v5.z,
	//		v6.x, v6.y, v6.z,
	//		v7.x, v7.y, v7.z,
	//	    v8.x, v8.y, v8.z
	//);

	return { v1,v2,v3,v4,v5,v6,v7,v8 };
}

bool ClibUtilsQY::Math::WorldObjects::IsInBoundingBox(const std::array<RE::NiPoint3, 8>& a_box, const RE::NiPoint3& a_point)
{
	//const auto v1 = rotate(RE::NiPoint3(min.x, min.y, min.z) - center, obj_angle) + center;
	//const auto v2 = rotate(RE::NiPoint3(max.x, min.y, min.z) - center, obj_angle) + center;
	//const auto v3 = rotate(RE::NiPoint3(max.x, max.y, min.z) - center, obj_angle) + center;
	//const auto v4 = rotate(RE::NiPoint3(min.x, max.y, min.z) - center, obj_angle) + center;

	//const auto v5 = rotate(RE::NiPoint3(min.x, min.y, max.z) - center, obj_angle) + center;
	//const auto v6 = rotate(RE::NiPoint3(max.x, min.y, max.z) - center, obj_angle) + center;
	//const auto v7 = rotate(RE::NiPoint3(max.x, max.y, max.z) - center, obj_angle) + center;
	//const auto v8 = rotate(RE::NiPoint3(min.x, max.y, max.z) - center, obj_angle) + center;

	const auto v_x = a_box[1] - a_box[0];
	const auto v_y = a_box[3] - a_box[0];
	const auto v_z = a_box[4] - a_box[0];
	const auto width = v_x.Length();
	const auto length = v_y.Length();
	const auto height = v_z.Length();
	const auto n_x = v_x / width;
	const auto n_y = v_y / length;
	const auto n_z = v_z / height;

	const auto v_rel = a_point - a_box[0];

	if (const auto comp = n_x.Dot(v_rel); comp > width || comp<0.f) {
		return false;
	}
	if (const auto comp = n_y.Dot(v_rel); comp > length || comp < 0.f) {
		return false;
	}
	if (const auto comp = n_z.Dot(v_rel); comp > height || comp < 0.f) {
		return false;
	}

	return true;
}

RE::NiPoint3 ClibUtilsQY::Math::WorldObjects::GetClosestPoint(const RE::TESObjectREFR* a_obj_from, const RE::TESObjectREFR* a_obj_to)
{
	const auto a_obj_from_pos = a_obj_from->GetPosition() + RE::NiPoint3(0.f,0.f, a_obj_from->GetHeight()/2.f);
	const auto bounding_box = GetBoundingBox(a_obj_to);
	if (Is2DBoundingBox(bounding_box)) {
	    const std::array a_2d_bounded_plane = {bounding_box[0],bounding_box[1],bounding_box[2],bounding_box[3]};
        return GetClosestPoint2D(a_2d_bounded_plane,a_obj_from_pos);
	}
	if (IsInBoundingBox(bounding_box, a_obj_from_pos)) {
		return a_obj_from_pos;
	}
	const auto closest = Math::LinAlg::GetClosest3Vertices(bounding_box,a_obj_from_pos);
	const auto v_normal = Math::LinAlg::CalculateNormalOfPlane(closest[1] - closest[0], closest[2] - closest[0]);
	const auto closest_point = Math::LinAlg::closestPointOnPlane(closest[0], a_obj_from_pos, v_normal);

	return Math::LinAlg::intersectLine(closest,closest_point);
}

RE::NiPoint3 ClibUtilsQY::Math::WorldObjects::GetClosestPoint(const RE::NiPoint3& a_point_from, const RE::TESObjectREFR* a_obj_to)
{
	const auto bounding_box = GetBoundingBox(a_obj_to);
	if (Is2DBoundingBox(bounding_box)) {
	    const std::array a_2d_bounded_plane = {bounding_box[0],bounding_box[1],bounding_box[2],bounding_box[3]};
        return GetClosestPoint2D(a_2d_bounded_plane,a_point_from);
	}
	if (IsInBoundingBox(bounding_box, a_point_from)) {
		return a_point_from;
	}
	const auto closest = Math::LinAlg::GetClosest3Vertices(bounding_box,a_point_from);
	const auto v_normal = Math::LinAlg::CalculateNormalOfPlane(closest[1] - closest[0], closest[2] - closest[0]);
	const auto closest_point = Math::LinAlg::closestPointOnPlane(closest[0], a_point_from, v_normal);

	return Math::LinAlg::intersectLine(closest,closest_point);
}
