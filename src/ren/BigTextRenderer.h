#ifndef REN_BIGTEXTRENDERER_H_
#define REN_BIGTEXTRENDERER_H_

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Shaders/DistanceFieldVectorGL.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>
#include <Magnum/Text/Renderer.h>

#ifdef DZSIM_WEB_PORT
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif

namespace ren {

    class BigTextRenderer {
    public:
#ifdef DZSIM_WEB_PORT
        typedef Magnum::Platform::EmscriptenApplication Application;
#else
        typedef Magnum::Platform::Sdl2Application Application;
#endif

    public:
        BigTextRenderer(Application& app,
            Corrade::PluginManager::Manager<Magnum::Text::AbstractFont>& font_plugin_mgr);

        void InitWithOpenGLContext(
            const Corrade::Containers::ArrayView<const char>& raw_font_data);

        void HandleViewportEvent(
            const Application::ViewportEvent& event);

        void DrawDisclaimer(float gui_scaling);

        // pos is between (-0.5, -0.5) to (0.5, 0.5)
        void DrawNumber(int number, const Magnum::Color4& col, float scaling, Magnum::Vector2 pos);

    private:
        Application& _app;
        Corrade::PluginManager::Manager<Magnum::Text::AbstractFont>& _font_plugin_mgr;

        Magnum::Shaders::DistanceFieldVectorGL2D _shader{ Magnum::NoCreate };

        Corrade::Containers::Pointer<Magnum::Text::AbstractFont> _font;
        Magnum::Text::DistanceFieldGlyphCache _cache{ Magnum::NoCreate };

        Magnum::GL::Buffer _vertices{ Magnum::NoCreate };
        Magnum::GL::Buffer _indices{ Magnum::NoCreate };
        Magnum::GL::Mesh _disclaimer_text_mesh{ Magnum::NoCreate };

        Corrade::Containers::Pointer<Magnum::Text::Renderer2D> _number_text;
        Magnum::Matrix3 _transformation_projection_number_text;
    };

} // namespace ren

#endif // REN_BIGTEXTRENDERER_H_
