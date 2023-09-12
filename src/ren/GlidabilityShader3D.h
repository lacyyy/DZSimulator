#ifndef REN_GLIDABILITYSHADER3D_H_
#define REN_GLIDABILITYSHADER3D_H_

#include <Corrade/Utility/Resource.h>
#include <Magnum/Magnum.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/Shaders/GenericGL.h>

namespace ren {

    class GlidabilityShader3D : public Magnum::GL::AbstractShaderProgram {
    public:
        typedef Magnum::Shaders::GenericGL3D::Position Position;
        typedef Magnum::Shaders::GenericGL3D::Normal Normal;
        typedef Magnum::Shaders::GenericGL3D::TransformationMatrix TransformationMatrix;

        // Construct without creating the underlying OpenGL object, can be safely
        // used for constructing objects without any OpenGL context being active.
        // Useful in cases where you will overwrite the instance later anyway.
        explicit GlidabilityShader3D(Magnum::NoCreateT);

        // This constructor requires an OpenGL context to be active.
        // @param use_instanced_transformation If set to true, ...
        explicit GlidabilityShader3D(bool use_instanced_transformation,
            const Corrade::Utility::Resource& resources);


        // Uniform setters

        GlidabilityShader3D& SetFinalTransformationMatrix(const Magnum::Matrix4& matrix);

        GlidabilityShader3D& SetDiffuseLightingEnabled(bool enabled = true);
        GlidabilityShader3D& SetColorOverrideEnabled(bool enabled = true);

        GlidabilityShader3D& SetLightDirection(const Magnum::Vector3& light_dir);
        GlidabilityShader3D& SetOverrideColor(const Magnum::Color4& c);
        GlidabilityShader3D& SetPlayerPosition(const Magnum::Vector3& player_pos);
        GlidabilityShader3D& SetHorizontalPlayerSpeed(float player_speed_hori);

        GlidabilityShader3D& SetSlideSuccessColor(const Magnum::Color4& c);
        GlidabilityShader3D& SetSlideAlmostFailColor(const Magnum::Color4& c);
        GlidabilityShader3D& SetSlideFailColor(const Magnum::Color4& c);

        GlidabilityShader3D& SetGravity(float gravity);
        GlidabilityShader3D& SetMinNoGroundChecksVelZ(float min_vel_z);
        GlidabilityShader3D& SetMaxVelocity(float max_v_per_axis);
        GlidabilityShader3D& SetStandableNormal(float normal_z);


    private:
        // Shader uniform locations
        Magnum::Int _uniform_final_transformation_matrix = -1;

        Magnum::Int _uniform_enable_diffuse_lighting = -1;
        Magnum::Int _uniform_enable_color_override = -1;

        Magnum::Int _uniform_light_dir = -1;
        Magnum::Int _uniform_override_color = -1;

        Magnum::Int _uniform_player_pos = -1;
        Magnum::Int _uniform_player_speed_hori = -1;

        Magnum::Int _uniform_slide_success_color = -1;
        Magnum::Int _uniform_slide_almost_fail_color = -1;
        Magnum::Int _uniform_slide_fail_color = -1;

        Magnum::Int _uniform_gravity = -1;
        Magnum::Int _uniform_min_no_ground_checks_vel_z = -1;
        Magnum::Int _uniform_max_vel = -1;
        Magnum::Int _uniform_standable_normal = -1;

    };

} // namespace ren

#endif // REN_GLIDABILITYSHADER3D_H_
