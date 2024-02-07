#include "ren/BigTextRenderer.h"

#include <string>

#include <Tracy.hpp>

#include <Corrade/Utility/FormatStl.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Complex.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Text/Renderer.h>

using namespace Magnum;
using namespace Math::Literals;
using namespace ren;

#define DISCLAIMER_MSG "THIS IS NOT A CHEAT\n(requires sv_cheats 1)"
#define NUMBER_TEXT_GLYPHS "0123456789"

BigTextRenderer::BigTextRenderer(Application& app,
    PluginManager::Manager<Text::AbstractFont>& font_plugin_mgr)
    : _app { app }
    , _font_plugin_mgr {font_plugin_mgr}
{}

void BigTextRenderer::InitWithOpenGLContext(
    const Containers::ArrayView<const char>& raw_font_data)
{
    ZoneScoped;

    // Delayed member construction here (not in constructor) because they
    // require a GL context
    _vertices = GL::Buffer{ GL::Buffer::TargetHint::Array        };
    _indices  = GL::Buffer{ GL::Buffer::TargetHint::ElementArray };
    _shader = Shaders::DistanceFieldVectorGL2D{};

    // @Optimization If the creation of the glyph cache takes too long, take a
    //               look at this conversation incl. Magnum's author:
    // https://matrix.to/#/!expMvWuzfDKKWnhbss:gitter.im/$23lvK_El1JM6OklpNkM3fJ55NrwVezRyhCtp6xu8_q4?via=gitter.im&via=integrations.ems.host&via=phanerox.com
    // I pray this link still works and points to the right message in the future.

    // Unscaled glyph cache texture size
    const Vector2i original_cache_tex_size = Vector2i{ 2048 };

    // Actual glyph cache texture size
    const Vector2i cache_tex_size = Vector2i{ 512 };

    // Distance field computation radius. This influences outline thickness!
    const UnsignedInt dist_field_radius = 16;

    _cache = Text::DistanceFieldGlyphCache(original_cache_tex_size,
                                           cache_tex_size, dist_field_radius);

    // Load a TrueTypeFont plugin and open the font
    _font = _font_plugin_mgr.loadAndInstantiate("TrueTypeFont");
    if (!_font || !_font->openData(raw_font_data, 192))
        Fatal{} << "ERROR: BigTextRenderer: Failed to open font file";

    // List of all glyphs that may be rendered. Duplicate entries are no problem,
    // they get sorted out in fillGlyphCache() with neglible performance impact.
    // Keep the number of unique glyphs as low as possible!
    std::string drawable_chars =
        DISCLAIMER_MSG
        NUMBER_TEXT_GLYPHS
        //"abcdefghijklmnopqrstuvwxyz"
        //"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        //"0123456789 :-+,.!°%&/()=?`*'#~;_<>|"
    ;
    _font->fillGlyphCache(_cache, drawable_chars);

    std::tie(_disclaimer_text_mesh, std::ignore) = Text::Renderer2D::render(
        *_font,
        _cache,
        100.0f,
        DISCLAIMER_MSG,
        _vertices,
        _indices,
        GL::BufferUsage::StaticDraw,
        Text::Alignment::TopRight
    );
    
    // @Optimization Can we move the cache into the renderer constructor?
    size_t number_text_glyph_cnt = std::strlen(NUMBER_TEXT_GLYPHS);
    _number_text.reset(
        new Text::Renderer2D(*_font, _cache, 50.0f, Text::Alignment::MiddleCenter));
    _number_text->reserve(number_text_glyph_cnt, GL::BufferUsage::DynamicDraw,
        GL::BufferUsage::StaticDraw);
    _transformation_projection_number_text =
        Matrix3::projection(Vector2{ _app.windowSize() }) *
        Matrix3::translation(Vector2{ _app.windowSize() }*0.5f);
}

void BigTextRenderer::HandleViewportEvent(
    const Application::ViewportEvent& /*event*/)
{
    // ...
}

void BigTextRenderer::DrawDisclaimer(float gui_scaling)
{
    Vector2i window_size = _app.windowSize();
    
    float disclaimer_text_scale = 1.0f * gui_scaling;

    // Position in pixels. (0,0) is top left screen corner
    float disclaimer_text_margin = 10.0f * gui_scaling;
    Vector2 disclaimer_text_pos = { // Put text in top right screen corner
        (float)window_size.x() - disclaimer_text_margin,
        disclaimer_text_margin
    };

    // Adding rotation to this wouldn't be easy! (Text would get scaled wrong)
    // Let's just stick to non-rotated text.
    Matrix3 disclaimer_text_matrix =
        Matrix3::translation(Vector2{
            // Convert pixel coordinates: topleft=(0,0), bottomright=(w,h)
            // to: topleft=(-1,1), bottomright=(1,-1)
            ((disclaimer_text_pos.x() / window_size.x()) - 0.5f) * 2.0f,
            ((disclaimer_text_pos.y() / window_size.y()) - 0.5f) * -2.0f,
        })
        *
        Matrix3::scaling(disclaimer_text_scale / Vector2{ _app.windowSize() });
        

    // Too high smoothness makes edges blurry, too small value makes edges aliased.
    float smoothness = 0.1f / gui_scaling; // Bigger text needs less smoothing

    const Color4 disclaimer_col = 0xff0000_rgbf;
    _shader
        .bindVectorTexture(_cache.texture())
        .setTransformationProjectionMatrix(disclaimer_text_matrix)
        .setColor(disclaimer_col)
        .setOutlineColor(disclaimer_col)
        .setOutlineRange(0.5f, 0.3f) // make text bold with same-color outline 
        .setSmoothness(smoothness)
        .draw(_disclaimer_text_mesh);

    
}

void BigTextRenderer::DrawNumber(
    int number, const Magnum::Color4& col, float scaling, Vector2 pos)
{
    Vector2i window_size = _app.windowSize();

    Vector2 offset = {
        window_size.x() * pos.x(),
        window_size.y() * pos.y()
    };

    _transformation_projection_number_text =
        Matrix3::projection(Vector2{ window_size }) *
        Matrix3::translation(offset) *
        Matrix3::scaling(Vector2{ scaling });

    _number_text->render(std::to_string(number));

    // Too high smoothness makes edges blurry, too small value makes edges aliased.
    float smoothness = 0.15f / scaling; // Bigger text needs less smoothing

    _shader
        .bindVectorTexture(_cache.texture())
        .setTransformationProjectionMatrix(_transformation_projection_number_text)
        .setColor(col)
        .setOutlineColor(col)
        .setOutlineRange(0.5f, 0.4f) // make text *slightly* bold with same-color outline 
        .setSmoothness(smoothness)
        .draw(_number_text->mesh());
}
