#include "ren/GlidabilityShader3D.h"

#include <Corrade/Containers/ArrayViewStl.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/FormatStl.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Tags.h>
#include <Magnum/Math/Vector3.h>
#include <portable-file-dialogs.h>

#include "utils_3d.h"

using namespace Magnum;
using namespace ren;
using namespace utils_3d;

GlidabilityShader3D::GlidabilityShader3D(NoCreateT)
    : AbstractShaderProgram { NoCreate }
{
}

GlidabilityShader3D::GlidabilityShader3D(bool use_instanced_transformation,
    const Corrade::Utility::Resource& resources)
{
    // TODO: Check: Does this shader completely support OpenGL ES 3.0, GLSL ES 3.00 ?
#ifdef DZSIM_WEB_PORT
    const GL::Version SHADER_VERSION = GL::Version::GLES300; // OpenGL ES 3.0
#else
    const GL::Version SHADER_VERSION = GL::Version::GL330; // OpenGL 3.3
#endif

    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(SHADER_VERSION);

    // These constructors add corresponding GLSL #version directive to source
    GL::Shader vert{ SHADER_VERSION, GL::Shader::Type::Vertex };
    GL::Shader frag{ SHADER_VERSION, GL::Shader::Type::Fragment };


    vert.addSource(Utility::formatString(
        "#define POSITION_ATTRIBUTE_LOCATION {}\n"
        "#define NORMAL_ATTRIBUTE_LOCATION {}\n"
        "#define TRANSFORMATION_MATRIX_ATTRIBUTE_LOCATION {}\n",
        Position::Location,
        Normal::Location,
        TransformationMatrix::Location))
        .addSource(use_instanced_transformation ? "#define INSTANCED_TRANSFORMATION\n" : "")
        .addSource(resources.getString("shaders/GlidabilityShader3D.vert"));
    frag.addSource(resources.getString("shaders/GlidabilityShader3D.frag"));

    // Any compiler error messages are printed to error output
    vert.submitCompile();
    frag.submitCompile();
    bool success = vert.checkCompile() && frag.checkCompile();

#ifdef NDEBUG // In Release builds, show user an error message (they have no console)
    if (!success) {
#ifdef DZSIM_WEB_PORT
        // FIXME somehow show the user an error message (not just in console)
#else
        pfd::message("OpenGL shader error",
            "An error occurred while compiling a shader!\n"
            "Please report this to the developer of this app.\n\n"
            "This error might be caused by an old graphics card or an "
            "old OpenGL version that currently isn't accounted for.",
            pfd::choice::ok, pfd::icon::error).result();
#endif
        std::abort();
    }
#else // In Debug builds, use regular asserts
    CORRADE_INTERNAL_ASSERT_OUTPUT(success);
#endif

    attachShader(vert);
    attachShader(frag);

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    _uniform_final_transformation_matrix = uniformLocation("final_transformation_matrix");
    _uniform_enable_diffuse_lighting     = uniformLocation("enable_diffuse_lighting");
    _uniform_enable_color_override       = uniformLocation("enable_color_override");
    _uniform_light_dir         = uniformLocation("light_dir");
    _uniform_override_color    = uniformLocation("override_color");
    _uniform_player_pos        = uniformLocation("player_pos");
    _uniform_player_speed_hori = uniformLocation("player_speed_hori");

    _uniform_slide_success_color     = uniformLocation("slide_success_color");
    _uniform_slide_almost_fail_color = uniformLocation("slide_almost_fail_color");
    _uniform_slide_fail_color        = uniformLocation("slide_fail_color");

    _uniform_gravity = uniformLocation("gravity");
    _uniform_min_no_ground_checks_vel_z
        = uniformLocation("min_no_ground_checks_vel_z");
    _uniform_max_vel = uniformLocation("max_vel");
    _uniform_standable_normal = uniformLocation("standable_normal");

    // Set default values of uniform vars. Must happen outside shader code
    // because the web build does not support in-shader uniform initialization.
    SetFinalTransformationMatrix(Matrix4{ Math::IdentityInit });
    SetDiffuseLightingEnabled(true);
    SetColorOverrideEnabled(true);
    SetLightDirection(Vector3{ 1.0f, 0.2f, -1.0f });
    SetOverrideColor(Color4{ 1.0f, 1.0f, 1.0f, 1.0f });
    SetPlayerPosition(Vector3{ 0.0f, 0.0f, 0.0f });
    SetHorizontalPlayerSpeed(1.0f); // Set to non-zero to avoid division by zero
    SetSlideSuccessColor   (Color4{ 1.0f, 1.0f, 1.0f, 1.0f });
    SetSlideAlmostFailColor(Color4{ 0.7f, 0.7f, 0.7f, 1.0f });
    SetSlideFailColor      (Color4{ 0.3f, 0.3f, 0.3f, 1.0f });
    SetGravity(0.0f);
    SetMinNoGroundChecksVelZ(0.0f);
    SetMaxVelocity(999999.0f);
    SetStandableNormal(0.7f);
}

GlidabilityShader3D&
GlidabilityShader3D::SetFinalTransformationMatrix(const Matrix4& matrix)
{
    setUniform(_uniform_final_transformation_matrix, matrix);
    return *this;
}

GlidabilityShader3D&
GlidabilityShader3D::SetDiffuseLightingEnabled(bool enabled)
{
    setUniform(_uniform_enable_diffuse_lighting, enabled);
    return *this;
}

GlidabilityShader3D&
GlidabilityShader3D::SetColorOverrideEnabled(bool enabled)
{
    setUniform(_uniform_enable_color_override, enabled);
    return *this;
}

GlidabilityShader3D&
GlidabilityShader3D::SetLightDirection(const Vector3& light_dir)
{
    if(light_dir.isNormalized())
        setUniform(_uniform_light_dir, light_dir);
    else
        setUniform(_uniform_light_dir, GetNormalized(light_dir));
    return *this;
}

GlidabilityShader3D&
GlidabilityShader3D::SetOverrideColor(const Color4& c)
{
    setUniform(_uniform_override_color, c);
    return *this;
}

GlidabilityShader3D&
GlidabilityShader3D::SetPlayerPosition(const Vector3& player_pos)
{
    setUniform(_uniform_player_pos, player_pos);
    return *this;
}

GlidabilityShader3D&
GlidabilityShader3D::SetHorizontalPlayerSpeed(float player_speed_hori)
{
    setUniform(_uniform_player_speed_hori, player_speed_hori);
    return *this;
}

GlidabilityShader3D& GlidabilityShader3D::SetSlideSuccessColor(const Magnum::Color4& c)
{
    setUniform(_uniform_slide_success_color, c);
    return *this;
}

GlidabilityShader3D& GlidabilityShader3D::SetSlideAlmostFailColor(const Magnum::Color4& c)
{
    setUniform(_uniform_slide_almost_fail_color, c);
    return *this;
}

GlidabilityShader3D& GlidabilityShader3D::SetSlideFailColor(const Magnum::Color4& c)
{
    setUniform(_uniform_slide_fail_color, c);
    return *this;
}

GlidabilityShader3D&
GlidabilityShader3D::SetGravity(float gravity)
{
    setUniform(_uniform_gravity, gravity);
    return *this;
}

GlidabilityShader3D&
GlidabilityShader3D::SetMinNoGroundChecksVelZ(float min_vel_z)
{
    setUniform(_uniform_min_no_ground_checks_vel_z, min_vel_z);
    return *this;
}

GlidabilityShader3D&
GlidabilityShader3D::SetMaxVelocity(float max_v_per_axis)
{
    setUniform(_uniform_max_vel, max_v_per_axis);
    return *this;
}

GlidabilityShader3D&
GlidabilityShader3D::SetStandableNormal(float normal_z)
{
    setUniform(_uniform_standable_normal, normal_z);
    return *this;
}
