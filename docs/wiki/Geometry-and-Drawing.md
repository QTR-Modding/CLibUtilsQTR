# Geometry and drawing

## Oriented bounding boxes

An oriented bounding box stores a center, half-size extents, and rotation:

```cpp
#include <DirectXCollision.h>
#include <CLibUtilsQTR/BoundingBox.hpp>

// With your engine PCH in scope:
bool ReadBounds(RE::TESObjectREFR* object,
                DirectX::BoundingOrientedBox& bounds) {
    return object && BoundingBox::GetOBB(object, bounds);
}
```

Use `skyrim` and provide DirectXMath/DirectXCollision headers in your build. The helper needs engine declarations from your PCH.

The reference overload derives bounds from the local bounds, scale, and world transform. Without current 3D it uses the reference's position and angles. False means useful bounds were not obtained.

The optional third argument is named `allowAABB`. When true, it first tries `GetOBBFromHavok()` before falling back to reference bounds. A separate `GetOBB(RE::bhkRigidBody*, ...)` uses the body's shape and transform; its default converts Havok units to Skyrim units.

`ClosestPointOnOBB()` can return an interior point. `ClosestPointOnOBBSurface()` finds a surface point. Both use a box you already obtained.

## Diagnostic shapes

Once your plugin has an appropriate debug overlay running, enqueue a line:

```cpp
#include <DirectXCollision.h>
#include <CLibUtilsQTR/DrawDebug.hpp>

constexpr int lifetimeMilliseconds = 1000;
DebugAPI_IMPL::DebugAPI::GetSingleton()->DrawLineForMS(
    from, to, lifetimeMilliseconds,
    DebugAPI_IMPL::DrawDebug::Colors::RED);
```

`from` and `to` are `RE::NiPoint3` world positions. Circles and spheres are also available; `DebugAPI_IMPL::DrawDebug` contains capsule and oriented-box helpers.

This is existing overlay integration code, not a standalone renderer. `DebugOverlayMenu` currently uses movie path `BetterThirdPersonSelection/overlay_menu` and registers under `"HUD Menu"`. The headers do not supply that movie. Account for those choices before installing an overlay; do not blindly register over an existing menu.

`Register()` registers and requests showing the menu. Its `AdvanceMovie()` calls `DebugAPI::Update()` to render and expire lines. Enqueuing lines alone does not create a working overlay. `Show()` and `Hide()` send menu messages.

Keep engine/UI operations in the appropriate game-thread lifecycle. Queue locking does not make arbitrary engine/UI access safe from workers.
