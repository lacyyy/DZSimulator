#ifndef RENDERING_BIGTEXTRENDERER_H_
#define RENDERING_BIGTEXTRENDERER_H_

#include <memory>

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/DistanceFieldVectorGL.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>
#include <Magnum/Text/Renderer.h>

namespace rendering {

    class BigTextRenderer {
    public:
        BigTextRenderer(Magnum::Platform::Sdl2Application& app,
            Corrade::PluginManager::Manager<Magnum::Text::AbstractFont>& font_plugin_mgr);

        void Init(const Corrade::Containers::ArrayView<const char>& raw_font_data);

        void HandleViewportEvent(
            const Magnum::Platform::Sdl2Application::ViewportEvent& event);

        void DrawDisclaimer(float gui_scaling);

    private:
        Magnum::Platform::Sdl2Application& _app;
        Corrade::PluginManager::Manager<Magnum::Text::AbstractFont>& _font_plugin_mgr;

        Magnum::Shaders::DistanceFieldVectorGL2D _shader{ Magnum::NoCreate };

        Corrade::Containers::Pointer<Magnum::Text::AbstractFont> _font;
        // TODO use Corrade::Containers::Pointer instead of std::unique_ptr here ??
        std::unique_ptr<Magnum::Text::DistanceFieldGlyphCache> _cache;

        Magnum::GL::Buffer _vertices{ Magnum::NoCreate };
        Magnum::GL::Buffer _indices{ Magnum::NoCreate };
        Magnum::GL::Mesh _disclaimer_text_mesh{ Magnum::NoCreate };

        //Corrade::Containers::Pointer<Magnum::Text::Renderer2D> _text_dynamic;
        //Magnum::Matrix3 _transformation_projection_text_dynamic;
    };

} // namespace rendering

#endif // RENDERING_BIGTEXTRENDERER_H_
