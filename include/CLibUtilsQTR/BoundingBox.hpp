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

    // ---- core: OBB from rigid body transform + SHAPE local AABB ----
    inline bool GetOBB(RE::bhkRigidBody* bhkBody, DirectX::BoundingOrientedBox& out,
                                     bool applyHavokToSkyrimScale = true, float tolerance = 0.0f) {
        if (!bhkBody) {
            return false;
        }

        // hkpRigidBody*
        auto* rb = bhkBody->GetRigidBody();
        if (!rb) {
            return false;
        }

        // hkpShape*
        auto* shape = rb->GetShape();
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

        return DirectX::XMFLOAT4(qx, qy, qz, qw); // DirectX = (x,y,z,w)
    }

    inline bool GetOBB(const RE::TESObjectREFR* obj, DirectX::BoundingOrientedBox& out) {
        if (!obj) return false;

        // Local bounds (gameplay/model bounds)
        RE::NiPoint3 minLocal = obj->GetBoundMin();
        RE::NiPoint3 maxLocal = obj->GetBoundMax();

        // If bounds are degenerate, you can optionally fallback to worldBound radius if you want
        // (some refs report zero bounds depending on type/load state)
        const RE::NiPoint3 size = maxLocal - minLocal;
        if (std::abs(size.x) < 1e-5f && std::abs(size.y) < 1e-5f && std::abs(size.z) < 1e-5f) {
            if (const auto node = obj->GetCurrent3D()) {
                const auto& wb = node->worldBound; // NiBound
                const float r = wb.radius;
                if (r > 0.f) {
                    out.Center = {wb.center.x, wb.center.y, wb.center.z};
                    out.Extents = {r, r, r};
                    out.Orientation = {0.f, 0.f, 0.f, 1.f};
                    return true;
                }
            }
            // last resort: still return something minimal
            const auto p = obj->GetPosition();
            out.Center = {p.x, p.y, p.z};
            out.Extents = {1.f, 1.f, 1.f};
            out.Orientation = {0.f, 0.f, 0.f, 1.f};
            return true;
        }

        // Apply scale (gameplay bounds are local model bounds)
        const float s = obj->GetScale();
        minLocal *= s;
        maxLocal *= s;

        // World transform
        RE::NiMatrix3 R;
        RE::NiPoint3 T;

        if (const auto node = obj->GetCurrent3D()) {
            R = node->world.rotate;
            T = node->world.translate;
        } else {
            R.SetEulerAnglesXYZ(obj->GetAngle());
            T = obj->GetPosition();
        }

        const RE::NiPoint3 localCenter = (minLocal + maxLocal) * 0.5f;
        const RE::NiPoint3 halfLocal = (maxLocal - minLocal) * 0.5f;
        const RE::NiPoint3 worldCenter = T + R * localCenter;

        out.Center = {worldCenter.x, worldCenter.y, worldCenter.z};
        out.Extents = {halfLocal.x, halfLocal.y, halfLocal.z};
        out.Orientation = MatrixToDXQuaternion(R);
        return true;
    }
}