#pragma once
#include <DirectXCollision.h>

namespace BoundingBox {
    constexpr float havokToSkyrim = 69.9915f;

    // ---- helpers: hkVector4 -> floats ----
    inline void Store4(const RE::hkVector4& v, float out[4]) { _mm_store_ps(out, v.quad); }

    inline DirectX::XMFLOAT3 ToFloat3(const RE::hkVector4& v) {
        alignas(16) float f[4];
        Store4(v, f);
        return {f[0], f[1], f[2]};
    }

    inline DirectX::XMFLOAT4 ToQuatFloat4(const RE::hkQuaternion& q) {
        alignas(16) float f[4];
        Store4(q.vec, f);
        return {f[0], f[1], f[2], f[3]};
    }

    // ---- helper: build identity hkTransform without assuming methods ----
    inline RE::hkTransform IdentityTransform() {
        RE::hkTransform t;
        t.rotation.col0 = RE::hkVector4(1.f, 0.f, 0.f, 0.f);
        t.rotation.col1 = RE::hkVector4(0.f, 1.f, 0.f, 0.f);
        t.rotation.col2 = RE::hkVector4(0.f, 0.f, 1.f, 0.f);
        t.translation = RE::hkVector4(0.f, 0.f, 0.f, 0.f);
        return t;
    }

    // ---- helper: column-major hkRotation * vector (x,y,z) ----
    inline DirectX::XMFLOAT3 Mul(const RE::hkRotation& R, const DirectX::XMFLOAT3& v) {
        alignas(16) float c0[4], c1[4], c2[4];
        Store4(R.col0, c0);
        Store4(R.col1, c1);
        Store4(R.col2, c2);

        // world = col0*v.x + col1*v.y + col2*v.z
        return {c0[0] * v.x + c1[0] * v.y + c2[0] * v.z, c0[1] * v.x + c1[1] * v.y + c2[1] * v.z,
                c0[2] * v.x + c1[2] * v.y + c2[2] * v.z};
    }

    inline RE::bhkRigidBody* GetRigidBody(const RE::TESObjectREFR* refr) {
        const auto object3D = refr->GetCurrent3D();
        if (!object3D) {
            return nullptr;
        }
        if (const auto body = object3D->GetCollisionObject()) {
            return body->GetRigidBody();
        }
        return nullptr;
    }

    // ---- core: OBB from rigid body transform + SHAPE local AABB ----
    inline bool GetOBB(RE::bhkRigidBody* bhkBody, DirectX::BoundingOrientedBox& out,
                       bool applyHavokToSkyrimScale = true, float tolerance = 0.0f) {
        if (!bhkBody) {
            return false;
        }

        // hkpRigidBody*
        auto rb = bhkBody->GetRigidBody();
        if (!rb) {
            return false;
        }

        // hkpShape*
        auto shape = rb->GetShape();
        if (!shape) {
            return false;
        }

        // body world transform (Havok units)
        RE::hkTransform bodyTr{};
        bhkBody->GetTransform(bodyTr);

        // shape LOCAL AABB (Havok units), via identity transform
        const RE::hkTransform I = IdentityTransform();

        RE::hkAabb localAabb{};
        // NOTE: GetAabbImpl is pure virtual on hkpShape, implemented by concrete shapes.
        shape->GetAabbImpl(I, tolerance, localAabb);

        const auto localMin = ToFloat3(localAabb.min);
        const auto localMax = ToFloat3(localAabb.max);

        const DirectX::XMFLOAT3 localCenter{(localMin.x + localMax.x) * 0.5f, (localMin.y + localMax.y) * 0.5f,
                                            (localMin.z + localMax.z) * 0.5f};

        const DirectX::XMFLOAT3 localHalf{(localMax.x - localMin.x) * 0.5f, (localMax.y - localMin.y) * 0.5f,
                                          (localMax.z - localMin.z) * 0.5f};

        // worldCenter = translation + rotation * localCenter
        const auto t = ToFloat3(bodyTr.translation);
        const auto rc = Mul(bodyTr.rotation, localCenter);

        DirectX::XMFLOAT3 worldCenter{t.x + rc.x, t.y + rc.y, t.z + rc.z};

        const float s = applyHavokToSkyrimScale ? havokToSkyrim : 1.0f;

        out.Center = {worldCenter.x * s, worldCenter.y * s, worldCenter.z * s};
        out.Extents = {localHalf.x * s, localHalf.y * s, localHalf.z * s};

        // Orientation: use Havok quaternion directly (recommended)
        RE::hkQuaternion q{};
        bhkBody->GetRotation(q);
        out.Orientation = ToQuatFloat4(q);

        return true;
    }

    inline DirectX::XMFLOAT4 MatrixToDXQuaternion(const RE::NiMatrix3& R) {
        // NiMatrix3 is row-major (entry[row][col]) in CommonLib style.
        const float m00 = R.entry[0][0], m01 = R.entry[0][1], m02 = R.entry[0][2];
        const float m10 = R.entry[1][0], m11 = R.entry[1][1], m12 = R.entry[1][2];
        const float m20 = R.entry[2][0], m21 = R.entry[2][1], m22 = R.entry[2][2];

        float qw, qx, qy, qz;

        const float trace = m00 + m11 + m22;
        if (trace > 0.0f) {
            const float s = std::sqrt(trace + 1.0f) * 2.0f;
            qw = 0.25f * s;
            qx = (m21 - m12) / s;
            qy = (m02 - m20) / s;
            qz = (m10 - m01) / s;
        } else if (m00 > m11 && m00 > m22) {
            const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
            qw = (m21 - m12) / s;
            qx = 0.25f * s;
            qy = (m01 + m10) / s;
            qz = (m02 + m20) / s;
        } else if (m11 > m22) {
            const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
            qw = (m02 - m20) / s;
            qx = (m01 + m10) / s;
            qy = 0.25f * s;
            qz = (m12 + m21) / s;
        } else {
            const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
            qw = (m10 - m01) / s;
            qx = (m02 + m20) / s;
            qy = (m12 + m21) / s;
            qz = 0.25f * s;
        }

        // Normalize (safety)
        const float lenSq = qx * qx + qy * qy + qz * qz + qw * qw;
        if (lenSq > 0.0f) {
            const float invLen = 1.0f / std::sqrt(lenSq);
            qx *= invLen;
            qy *= invLen;
            qz *= invLen;
            qw *= invLen;
        }

        return DirectX::XMFLOAT4(qx, qy, qz, qw);  // DirectX = (x,y,z,w)
    }

    // ---- helper: build OBB from Havok world AABB (fast path) ----
    inline bool GetOBBFromHavok(const RE::TESObjectREFR* obj, DirectX::BoundingOrientedBox& out) {
        if (const auto body = GetRigidBody(obj)) {
            RE::hkAabb aabb;
            body->GetAabbWorldspace(aabb);

            float minComp[4]{};
            float maxComp[4]{};
            _mm_store_ps(minComp, aabb.min.quad);
            _mm_store_ps(maxComp, aabb.max.quad);

            // Same factor you used before
            constexpr float havokToSkyrim = 69.9915f;

            const RE::NiPoint3 minWorld{minComp[0] * havokToSkyrim, minComp[1] * havokToSkyrim,
                                        minComp[2] * havokToSkyrim};
            const RE::NiPoint3 maxWorld{maxComp[0] * havokToSkyrim, maxComp[1] * havokToSkyrim,
                                        maxComp[2] * havokToSkyrim};

            const RE::NiPoint3 center = (minWorld + maxWorld) * 0.5f;
            const RE::NiPoint3 half = (maxWorld - minWorld) * 0.5f;

            out.Center = DirectX::XMFLOAT3(center.x, center.y, center.z);
            out.Extents = DirectX::XMFLOAT3(half.x, half.y, half.z);
            // Havok gave us a world AABB, so orientation is identity
            out.Orientation = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);

            return true;
        }
        return false;
    }

    inline bool GetOBB(const RE::TESObjectREFR* obj, DirectX::BoundingOrientedBox& out, const bool allowAABB = false) {
        if (allowAABB && GetOBBFromHavok(obj, out)) {
            return true;
        }

        RE::NiPoint3 minLocal = obj->GetBoundMin();
        RE::NiPoint3 maxLocal = obj->GetBoundMax();

        const RE::NiPoint3 size = maxLocal - minLocal;
        if (std::abs(size.x) < 1e-5f && std::abs(size.y) < 1e-5f && std::abs(size.z) < 1e-5f) {
            return false;
        }

        const float scale = obj->GetScale();
        minLocal *= scale;
        maxLocal *= scale;

        RE::NiMatrix3 rotation;
        RE::NiPoint3 translation;

        if (const auto node = obj->GetCurrent3D()) {
            rotation = node->world.rotate;
            translation = node->world.translate;
        } else {
            rotation.SetEulerAnglesXYZ(obj->GetAngle());
            translation = obj->GetPosition();
        }

        const RE::NiPoint3 localCenter = (minLocal + maxLocal) * 0.5f;
        const RE::NiPoint3 halfExtents = (maxLocal - minLocal) * 0.5f;
        const RE::NiPoint3 worldCenter = translation + rotation * localCenter;

        out.Center = {worldCenter.x, worldCenter.y, worldCenter.z};
        out.Extents = {halfExtents.x, halfExtents.y, halfExtents.z};
        out.Orientation = MatrixToDXQuaternion(rotation);

        return true;
    }

    inline float clampf(const float v, const float lo, const float hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

    // Returns closest point on/in OBB. If inside, returns pWorld unchanged.
    inline DirectX::XMVECTOR ClosestPointOnOBB(const DirectX::BoundingOrientedBox& obb,
                                               DirectX::XMVECTOR pWorld)  // xyz used, w ignored
    {
        using namespace DirectX;

        const XMVECTOR C = XMLoadFloat3(&obb.Center);

        const XMVECTOR q = XMLoadFloat4(&obb.Orientation);
        const XMMATRIX R = XMMatrixRotationQuaternion(q);

        // Unit axes (assumes quaternion normalized)
        const XMVECTOR axisX = R.r[0];
        const XMVECTOR axisY = R.r[1];
        const XMVECTOR axisZ = R.r[2];

        const XMVECTOR d = pWorld - C;

        const float lx = XMVectorGetX(XMVector3Dot(d, axisX));
        const float ly = XMVectorGetX(XMVector3Dot(d, axisY));
        const float lz = XMVectorGetX(XMVector3Dot(d, axisZ));

        const float ex = obb.Extents.x;
        const float ey = obb.Extents.y;
        const float ez = obb.Extents.z;

        // Inside => return original point
        if (std::fabs(lx) <= ex && std::fabs(ly) <= ey && std::fabs(lz) <= ez) {
            return pWorld;
        }

        const float qx = clampf(lx, -ex, ex);
        const float qy = clampf(ly, -ey, ey);
        const float qz = clampf(lz, -ez, ez);

        XMVECTOR Pclosest = C;
        Pclosest = XMVectorMultiplyAdd(axisX, XMVectorReplicate(qx), Pclosest);
        Pclosest = XMVectorMultiplyAdd(axisY, XMVectorReplicate(qy), Pclosest);
        Pclosest = XMVectorMultiplyAdd(axisZ, XMVectorReplicate(qz), Pclosest);

        return Pclosest;
    }

    inline DirectX::XMVECTOR ClosestPointOnOBBSurface(const DirectX::BoundingOrientedBox& obb,
                                                      DirectX::XMVECTOR pWorld) {
        using namespace DirectX;

        const XMVECTOR C = XMLoadFloat3(&obb.Center);

        const XMVECTOR q = XMLoadFloat4(&obb.Orientation);
        const XMMATRIX R = XMMatrixRotationQuaternion(q);

        const XMVECTOR axisX = R.r[0];
        const XMVECTOR axisY = R.r[1];
        const XMVECTOR axisZ = R.r[2];

        const XMVECTOR d = pWorld - C;

        const float lx = XMVectorGetX(XMVector3Dot(d, axisX));
        const float ly = XMVectorGetX(XMVector3Dot(d, axisY));
        const float lz = XMVectorGetX(XMVector3Dot(d, axisZ));

        const float ex = obb.Extents.x;
        const float ey = obb.Extents.y;
        const float ez = obb.Extents.z;

        float qx = clampf(lx, -ex, ex);
        float qy = clampf(ly, -ey, ey);
        float qz = clampf(lz, -ez, ez);

        const bool inside = (std::fabs(lx) <= ex) && (std::fabs(ly) <= ey) && (std::fabs(lz) <= ez);

        if (inside) {
            const float mx = ex - std::fabs(lx);
            const float my = ey - std::fabs(ly);
            const float mz = ez - std::fabs(lz);

            if (mx <= my && mx <= mz) {
                qx = std::copysign(ex, (lx == 0.f) ? 1.f : lx);
                qy = ly;
                qz = lz;
            } else if (my <= mz) {
                qx = lx;
                qy = std::copysign(ey, (ly == 0.f) ? 1.f : ly);
                qz = lz;
            } else {
                qx = lx;
                qy = ly;
                qz = std::copysign(ez, (lz == 0.f) ? 1.f : lz);
            }
        }

        XMVECTOR Pclosest = C;
        Pclosest = XMVectorMultiplyAdd(axisX, XMVectorReplicate(qx), Pclosest);
        Pclosest = XMVectorMultiplyAdd(axisY, XMVectorReplicate(qy), Pclosest);
        Pclosest = XMVectorMultiplyAdd(axisZ, XMVectorReplicate(qz), Pclosest);

        return Pclosest;
    }
}