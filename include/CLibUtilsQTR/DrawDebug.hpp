#pragma once

// Credits: https://github.com/fenix31415/UselessFenixUtils

#include <REX/REX/Singleton.h>
#include <shared_mutex>

namespace DebugAPI_IMPL {
    constexpr int CIRCLE_NUM_SEGMENTS = 32;

    constexpr float DRAW_LOC_MAX_DIF = 5.0f;

    constexpr float ROOT_TWO = std::numbers::sqrt2_v<float>;


    inline std::uint8_t AlphaPct(const float a) // 0..100
    {
        if (a <= 0.0f) return 0;
        if (a >= 1.0f) return 100;
        return static_cast<std::uint8_t>(std::lround(a * 100.0f));
    }


    class DebugAPILine {
    public:
        DebugAPILine(RE::NiPoint3 from, RE::NiPoint3 to, const RE::NiColorA& color, float lineThickness,
                     std::uint64_t destroyTickCount);

        RE::NiPoint3 From;
        RE::NiPoint3 To;
        RE::NiColorA Color;
        std::uint32_t fColor;
        std::uint8_t Alpha;
        float LineThickness;

        std::uint64_t DestroyTickCount;
    };

    class DebugAPI : public REX::Singleton<DebugAPI> {
    public:
        void Update();

        static RE::GPtr<RE::IMenu> GetHUD();

        void DrawLine2D(const RE::GPtr<RE::GFxMovieView>& movie, RE::NiPoint2 from, RE::NiPoint2 to, uint32_t color,
                        float lineThickness, float alpha) const;
        void DrawLine2D(const RE::GPtr<RE::GFxMovieView>& movie, RE::NiPoint2 from, RE::NiPoint2 to,
                        const RE::NiColorA& color,
                        float lineThickness) const;
        void DrawLine3D(const RE::GPtr<RE::GFxMovieView>& movie, RE::NiPoint3 from, RE::NiPoint3 to, uint32_t color,
                        float lineThickness, float alpha) const;
        void DrawLine3D(const RE::GPtr<RE::GFxMovieView>& movie, RE::NiPoint3 from, RE::NiPoint3 to,
                        const RE::NiColorA& color,
                        float lineThickness) const;
        static void ClearLines2D(const RE::GPtr<RE::GFxMovieView>& movie);

        void DrawLineForMS(const RE::NiPoint3& from, const RE::NiPoint3& to, int liftetimeMS = 10,
                           const RE::NiColorA& color = {1.0f, 0.0f, 0.0f, 1.0f}, float lineThickness = 1);
        void DrawSphere(RE::NiPoint3, float radius, int liftetimeMS = 10,
                        const RE::NiColorA& color = {1.0f, 0.0f, 0.0f, 1.0f}, float lineThickness = 1);
        void DrawCircle(RE::NiPoint3, float radius, RE::NiPoint3 eulerAngles, int liftetimeMS = 10,
                        const RE::NiColorA& color = {1.0f, 0.0f, 0.0f, 1.0f}, float lineThickness = 1);

        std::vector<DebugAPILine*> LinesToDraw;
        mutable std::shared_mutex mutex_;

        bool DEBUG_API_REGISTERED;


        static RE::NiPoint2 WorldToScreenLoc(const RE::GPtr<RE::GFxMovieView>& movie, RE::NiPoint3 worldLoc);

        void FastClampToScreen(RE::NiPoint2& point) const;

        bool IsOnScreen(RE::NiPoint2 from, RE::NiPoint2 to) const;
        bool IsOnScreen(RE::NiPoint2 point) const;

        void CacheMenuData();

        bool cachedMenuData;

        float ScreenResX;
        float ScreenResY;

    private:
        DebugAPILine* GetExistingLine(const RE::NiPoint3& from, const RE::NiPoint3& to, const RE::NiColorA& color,
                                      float lineThickness) const;
    };

    class DebugOverlayMenu : RE::IMenu {
    public:
        static constexpr auto MENU_PATH = "BetterThirdPersonSelection/overlay_menu";
        static constexpr auto MENU_NAME = "HUD Menu";

        DebugOverlayMenu();

        static void Register();

        static void Show();
        static void Hide();

        static RE::stl::owner<IMenu*> Creator() { return new DebugOverlayMenu(); }

        void AdvanceMovie(float a_interval, std::uint32_t a_currentTime) override;

    private:
        class Logger : public RE::GFxLog {
        public:
            void LogMessageVarg(LogMessageType, const char* a_fmt, const std::va_list a_argList) override {
                std::string fmt(a_fmt ? a_fmt : "");
                while (!fmt.empty() && fmt.back() == '\n') {
                    fmt.pop_back();
                }

                std::va_list args;
                va_copy(args, a_argList);
                std::vector<char> buf(static_cast<std::size_t>(std::vsnprintf(nullptr, 0, fmt.c_str(), a_argList) + 1));
                std::vsnprintf(buf.data(), buf.size(), fmt.c_str(), args);
                va_end(args);

                logger::info("{}", buf.data());
            }
        };
    };

    namespace DrawDebug {
        namespace Colors {
            static constexpr auto RED = RE::NiColorA(1.0f, 0.0f, 0.0f, 1.0f);
            static constexpr auto GRN = RE::NiColorA(0.0f, 1.0f, 0.0f, 1.0f);
            static constexpr auto BLU = RE::NiColorA(0.0f, 0.0f, 1.0f, 1.0f);
        }

        template <int time = 100>
        void draw_line(const RE::NiPoint3& _from, const RE::NiPoint3& _to, const float size = 5.0f,
                       const RE::NiColorA Color = Colors::RED) {
            DebugAPI::GetSingleton()->DrawLineForMS(_from, _to, time, Color, size);
        }


        template <RE::NiColorA Color = Colors::RED>
        void draw_sphere(const RE::NiPoint3& a_center, const float r = 5.0f, const float size = 5.0f,
                         const int time = 3000) {
            DebugAPI::GetSingleton()->DrawSphere(a_center, r, time, Color, size);
        }

        inline void DrawOBB(const DirectX::BoundingOrientedBox& obb) {
            DirectX::XMFLOAT3 c[8];
            obb.GetCorners(c);

            auto P = [&](const int i) { return RE::NiPoint3{c[i].x, c[i].y, c[i].z}; };

            // bottom face
            draw_line(P(0), P(1));
            draw_line(P(1), P(2));
            draw_line(P(2), P(3));
            draw_line(P(3), P(0));

            // top face
            draw_line(P(4), P(5));
            draw_line(P(5), P(6));
            draw_line(P(6), P(7));
            draw_line(P(7), P(4));

            // vertical edges
            draw_line(P(0), P(4));
            draw_line(P(1), P(5));
            draw_line(P(2), P(6));
            draw_line(P(3), P(7));
        }
    }

    inline RE::NiPoint3 NormalizeVector(RE::NiPoint3 p) {
        p.Unitize();
        return p;
    }

    inline RE::NiPoint3 RotateVector(const RE::NiQuaternion quatIn, const RE::NiPoint3 vecIn) {
        const float num = quatIn.x * 2.0f;
        const float num2 = quatIn.y * 2.0f;
        const float num3 = quatIn.z * 2.0f;
        const float num4 = quatIn.x * num;
        const float num5 = quatIn.y * num2;
        const float num6 = quatIn.z * num3;
        const float num7 = quatIn.x * num2;
        const float num8 = quatIn.x * num3;
        const float num9 = quatIn.y * num3;
        const float num10 = quatIn.w * num;
        const float num11 = quatIn.w * num2;
        const float num12 = quatIn.w * num3;
        RE::NiPoint3 result;
        result.x = (1.0f - (num5 + num6)) * vecIn.x + (num7 - num12) * vecIn.y + (num8 + num11) * vecIn.z;
        result.y = (num7 + num12) * vecIn.x + (1.0f - (num4 + num6)) * vecIn.y + (num9 - num10) * vecIn.z;
        result.z = (num8 - num11) * vecIn.x + (num9 + num10) * vecIn.y + (1.0f - (num4 + num5)) * vecIn.z;
        return result;
    }
    
    inline RE::NiPoint3 RotateVector(const RE::NiPoint3& eulerRad, const RE::NiPoint3& vecIn) {
        RE::NiMatrix3 m;
        m.SetEulerAnglesXYZ(eulerRad);
        return m * vecIn;
    }

    inline RE::NiPoint3 GetForwardVector(const RE::NiQuaternion quatIn) {
        return RotateVector(quatIn, RE::NiPoint3(0.0f, 1.0f, 0.0f));
    }

    inline RE::NiPoint3 GetForwardVector(const RE::NiPoint3 eulerIn) {
        const float pitch = eulerIn.x;
        const float yaw = eulerIn.z;

        return RE::NiPoint3(sin(yaw) * cos(pitch), cos(yaw) * cos(pitch), sin(pitch));
    }

    constexpr int FIND_COLLISION_MAX_RECURSION = 2;

    inline bool IsRoughlyEqual(const float first, const float second, const float maxDif) {
        return abs(first - second) <= maxDif;
    }

    inline RE::NiPoint3 GetCameraPos() {
        const auto playerCam = RE::PlayerCamera::GetSingleton();
        const auto pos = playerCam->GetRuntimeData2().pos;
        return RE::NiPoint3(pos.x, pos.y, pos.z);
    }

    inline RE::NiQuaternion GetCameraRot() {
        const auto playerCam = RE::PlayerCamera::GetSingleton();

        const auto cameraState = playerCam->currentState.get();
        if (!cameraState) return RE::NiQuaternion();

        RE::NiQuaternion niRotation;
        cameraState->GetRotation(niRotation);

        return niRotation;
    }

    inline bool IsPosBehindPlayerCamera(const RE::NiPoint3 pos) {
        const auto cameraPos = GetCameraPos();
        const auto cameraRot = GetCameraRot();

        const auto toTarget = NormalizeVector(pos - cameraPos);
        const auto cameraForward = NormalizeVector(GetForwardVector(cameraRot));

        const auto angleDif = abs((toTarget - cameraForward).Length());

        return angleDif > ROOT_TWO;
    }

    inline RE::NiPoint3 GetPointOnRotatedCircle(const RE::NiPoint3 origin, const float radius, const float i,
                                                const float maxI,
                                                const RE::NiPoint3 eulerAngles) {
        const float currAngle = i / maxI * RE::NI_TWO_PI;

        const RE::NiPoint3 targetPos(radius * cos(currAngle), radius * sin(currAngle), 0.0f);

        const auto targetPosRotated = RotateVector(eulerAngles, targetPos);

        return RE::NiPoint3(targetPosRotated.x + origin.x, targetPosRotated.y + origin.y,
                            targetPosRotated.z + origin.z);
    }

    inline DebugAPILine::DebugAPILine(const RE::NiPoint3 from, const RE::NiPoint3 to, const RE::NiColorA& color,
                                      const float lineThickness,
                                      const std::uint64_t destroyTickCount) {
        From = from;
        To = to;
        Color = color;
        RE::NiColor a_rgb;
        a_rgb = color;
        fColor = a_rgb.ToInt();
        Alpha = AlphaPct(color.alpha);
        LineThickness = lineThickness;
        DestroyTickCount = destroyTickCount;
    }

    inline void DebugAPI::Update() {
        const auto hud = GetHUD();
        if (!hud || !hud->uiMovie) return;

        CacheMenuData();
        ClearLines2D(hud->uiMovie);

        std::unique_lock lock(mutex_);
        for (int i = 0; i < LinesToDraw.size(); i++) {
            const DebugAPILine* line = LinesToDraw[i];

            DrawLine3D(hud->uiMovie, line->From, line->To, line->fColor, line->LineThickness, line->Alpha);

            if (GetTickCount64() > line->DestroyTickCount) {
                LinesToDraw.erase(LinesToDraw.begin() + i);
                delete line;
                i--;
            }
        }
    }

    inline RE::GPtr<RE::IMenu> DebugAPI::GetHUD() {
        RE::GPtr<RE::IMenu> hud = RE::UI::GetSingleton()->GetMenu(DebugOverlayMenu::MENU_NAME);
        return hud;
    }

    inline void DebugAPI::DrawLine2D(const RE::GPtr<RE::GFxMovieView>& movie, RE::NiPoint2 from, RE::NiPoint2 to,
                                     const uint32_t color,
                                     const float lineThickness, const float alpha) const {
        if (!IsOnScreen(from, to)) return;

        FastClampToScreen(from);
        FastClampToScreen(to);

        const RE::GFxValue argsLineStyle[3]{static_cast<double>(lineThickness), static_cast<double>(color),
                                            static_cast<double>(alpha)};
        movie->Invoke("lineStyle", nullptr, argsLineStyle, 3);

        const RE::GFxValue argsStartPos[2]{from.x, from.y};
        movie->Invoke("moveTo", nullptr, argsStartPos, 2);

        const RE::GFxValue argsEndPos[2]{to.x, to.y};
        movie->Invoke("lineTo", nullptr, argsEndPos, 2);

        movie->Invoke("endFill", nullptr, nullptr, 0);
    }

    inline void DebugAPI::DrawLine2D(const RE::GPtr<RE::GFxMovieView>& movie, const RE::NiPoint2 from,
                                     const RE::NiPoint2 to,
                                     const RE::NiColorA& color,
                                     const float lineThickness) const {
        RE::NiColor a_rgb;
        a_rgb = color;
        DrawLine2D(movie, from, to, a_rgb.ToInt(), lineThickness, AlphaPct(color.alpha));
    }

    inline void DebugAPI::DrawLine3D(const RE::GPtr<RE::GFxMovieView>& movie, const RE::NiPoint3 from,
                                     const RE::NiPoint3 to,
                                     const uint32_t color,
                                     const float lineThickness, const float alpha) const {
        if (IsPosBehindPlayerCamera(from) && IsPosBehindPlayerCamera(to)) return;

        const RE::NiPoint2 screenLocFrom = WorldToScreenLoc(movie, from);
        const RE::NiPoint2 screenLocTo = WorldToScreenLoc(movie, to);
        DrawLine2D(movie, screenLocFrom, screenLocTo, color, lineThickness, alpha);
    }

    inline void DebugAPI::DrawLine3D(const RE::GPtr<RE::GFxMovieView>& movie, const RE::NiPoint3 from,
                                     const RE::NiPoint3 to,
                                     const RE::NiColorA& color,
                                     const float lineThickness) const {
        RE::NiColor a_rgb;
        a_rgb = color;
        DrawLine3D(movie, from, to, a_rgb.ToInt(), lineThickness, AlphaPct(color.alpha));
    }

    inline void DebugAPI::ClearLines2D(const RE::GPtr<RE::GFxMovieView>& movie) {
        movie->Invoke("clear", nullptr, nullptr, 0);
    }

    inline void DebugAPI::DrawLineForMS(const RE::NiPoint3& from, const RE::NiPoint3& to, const int liftetimeMS,
                                        const RE::NiColorA& color,
                                        const float lineThickness) {
        if (DebugAPILine* oldLine = GetExistingLine(from, to, color, lineThickness)) {
            oldLine->From = from;
            oldLine->To = to;
            oldLine->DestroyTickCount = GetTickCount64() + liftetimeMS;
            oldLine->LineThickness = lineThickness;
            return;
        }

        const auto newLine = new DebugAPILine(from, to, color, lineThickness, GetTickCount64() + liftetimeMS);
        std::unique_lock lock(mutex_);
        LinesToDraw.push_back(newLine);
    }

    inline void DebugAPI::DrawSphere(const RE::NiPoint3 origin, const float radius, const int liftetimeMS,
                                     const RE::NiColorA& color,
                                     const float lineThickness) {
        DrawCircle(origin, radius, RE::NiPoint3(0.0f, 0.0f, 0.0f), liftetimeMS, color, lineThickness);
        DrawCircle(origin, radius, RE::NiPoint3(RE::NI_HALF_PI, 0.0f, 0.0f), liftetimeMS, color, lineThickness);
    }

    inline void DebugAPI::DrawCircle(const RE::NiPoint3 origin, const float radius, const RE::NiPoint3 eulerAngles,
                                     const int liftetimeMS,
                                     const RE::NiColorA& color, const float lineThickness) {
        RE::NiPoint3 lastEndPos =
            GetPointOnRotatedCircle(origin, radius, CIRCLE_NUM_SEGMENTS, static_cast<float>(CIRCLE_NUM_SEGMENTS - 1),
                                    eulerAngles);

        for (int i = 0; i <= CIRCLE_NUM_SEGMENTS; i++) {
            RE::NiPoint3 currEndPos =
                GetPointOnRotatedCircle(origin, radius, static_cast<float>(i),
                                        static_cast<float>(CIRCLE_NUM_SEGMENTS - 1), eulerAngles);

            DrawLineForMS(lastEndPos, currEndPos, liftetimeMS, color, lineThickness);

            lastEndPos = currEndPos;
        }
    }

    inline DebugAPILine* DebugAPI::GetExistingLine(const RE::NiPoint3& from, const RE::NiPoint3& to,
                                                   const RE::NiColorA& color,
                                                   const float lineThickness) const {
        std::shared_lock lock(mutex_);
        for (int i = 0; i < LinesToDraw.size(); i++) {
            DebugAPILine* line = LinesToDraw[i];

            if (IsRoughlyEqual(from.x, line->From.x, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(from.y, line->From.y, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(from.z, line->From.z, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(to.x, line->To.x, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(to.y, line->To.y, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(to.z, line->To.z, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(lineThickness, line->LineThickness, DRAW_LOC_MAX_DIF) && color == line->Color) {
                return line;
            }
        }

        return nullptr;
    }

    constexpr float CLAMP_MAX_OVERSHOOT = 10000.0f;

    inline void DebugAPI::FastClampToScreen(RE::NiPoint2& point) const {
        if (point.x < 0.0) {
            const float overshootX = abs(point.x);
            if (overshootX > CLAMP_MAX_OVERSHOOT) point.x += overshootX - CLAMP_MAX_OVERSHOOT;
        } else if (point.x > ScreenResX) {
            const float overshootX = point.x - ScreenResX;
            if (overshootX > CLAMP_MAX_OVERSHOOT) point.x -= overshootX - CLAMP_MAX_OVERSHOOT;
        }

        if (point.y < 0.0) {
            const float overshootY = abs(point.y);
            if (overshootY > CLAMP_MAX_OVERSHOOT) point.y += overshootY - CLAMP_MAX_OVERSHOOT;
        } else if (point.y > ScreenResY) {
            const float overshootY = point.y - ScreenResY;
            if (overshootY > CLAMP_MAX_OVERSHOOT) point.y -= overshootY - CLAMP_MAX_OVERSHOOT;
        }
    }

    inline RE::NiPoint2
    DebugAPI::WorldToScreenLoc(const RE::GPtr<RE::GFxMovieView>& movie, const RE::NiPoint3 worldLoc) {
        static uintptr_t g_worldToCamMatrix = RELOCATION_ID(519579, 406126).address();
        static auto g_viewPort =
            reinterpret_cast<RE::NiRect<float>*>(RELOCATION_ID(519618, 406160).address());

        RE::NiPoint2 screenLocOut;

        float zVal;

        RE::NiCamera::WorldPtToScreenPt3(reinterpret_cast<float (*)[4]>(g_worldToCamMatrix), *g_viewPort, worldLoc,
                                         screenLocOut.x,
                                         screenLocOut.y, zVal, 1e-5f);
        const RE::GRectF rect = movie->GetVisibleFrameRect();

        screenLocOut.x = rect.left + (rect.right - rect.left) * screenLocOut.x;
        screenLocOut.y = 1.0f - screenLocOut.y;
        screenLocOut.y = rect.top + (rect.bottom - rect.top) * screenLocOut.y;

        return screenLocOut;
    }

    inline DebugOverlayMenu::DebugOverlayMenu() {
        const auto scaleformManager = RE::BSScaleformManager::GetSingleton();

        inputContext = Context::kNone;
        depthPriority = 127;

        menuFlags.set(RE::UI_MENU_FLAGS::kRequiresUpdate);
        menuFlags.set(RE::UI_MENU_FLAGS::kAllowSaving);
        menuFlags.set(RE::UI_MENU_FLAGS::kCustomRendering);

        scaleformManager->LoadMovieEx(this, MENU_PATH, [](RE::GFxMovieDef* a_def) -> void {
            a_def->SetState(RE::GFxState::StateType::kLog, RE::make_gptr<Logger>().get());
        });
    }

    inline void DebugOverlayMenu::Register() {
        if (const auto ui = RE::UI::GetSingleton()) {
            ui->Register(MENU_NAME, Creator);

            Show();
        }
    }

    inline void DebugOverlayMenu::Show() {
        if (const auto msgQ = RE::UIMessageQueue::GetSingleton()) {
            msgQ->AddMessage(MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
        }
    }

    inline void DebugOverlayMenu::Hide() {
        if (const auto msgQ = RE::UIMessageQueue::GetSingleton()) {
            msgQ->AddMessage(MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
        }
    }

    inline void DebugAPI::CacheMenuData() {
        if (cachedMenuData) return;

        const RE::GPtr<RE::IMenu> menu = RE::UI::GetSingleton()->GetMenu(DebugOverlayMenu::MENU_NAME);
        if (!menu || !menu->uiMovie) return;

        const RE::GRectF rect = menu->uiMovie->GetVisibleFrameRect();

        ScreenResX = abs(rect.left - rect.right);
        ScreenResY = abs(rect.top - rect.bottom);

        cachedMenuData = true;
    }

    inline bool DebugAPI::IsOnScreen(const RE::NiPoint2 from, const RE::NiPoint2 to) const {
        return IsOnScreen(from) || IsOnScreen(to);
    }

    inline bool DebugAPI::IsOnScreen(const RE::NiPoint2 point) const {
        return point.x <= ScreenResX && point.x >= 0.0 && point.y <= ScreenResY && point.y >= 0.0;
    }

    inline void DebugOverlayMenu::AdvanceMovie(const float a_interval, const std::uint32_t a_currentTime) {
        IMenu::AdvanceMovie(a_interval, a_currentTime);

        DebugAPI::GetSingleton()->Update();
    }
}