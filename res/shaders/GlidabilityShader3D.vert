// The attribute location and GLSL version preprocessor directives are added
// programmatically

layout(location = POSITION_ATTRIBUTE_LOCATION) in highp vec4 position;
layout(location =   NORMAL_ATTRIBUTE_LOCATION) in highp vec3 normal;

#ifdef INSTANCED_TRANSFORMATION
layout(location = TRANSFORMATION_MATRIX_ATTRIBUTE_LOCATION)
in highp mat4 instanced_model_transformation;
#endif

out lowp vec4 interpolated_color;

uniform highp mat4 final_transformation_matrix = mat4(1.0);
uniform bool enable_diffuse_lighting = true;
uniform bool enable_color_override = true;
uniform highp vec3 light_dir = normalize(vec3(1.0, 0.2, -1.0)); // Normalized
uniform lowp vec4 override_color = vec4(1.0, 1.0, 1.0, 1.0);
uniform highp vec3 player_pos = vec3(0.0, 0.0, 0.0);
uniform highp float player_speed_hori = 1.0;

uniform highp float gravity = 0.0; // sv_gravity
uniform highp float min_no_ground_checks_vel_z = 0.0;
uniform highp float max_vel = 999999.0; // sv_maxvelocity
uniform highp float standable_normal = 0.7; // sv_standable_normal


// Returns brightness value between 0 and 1 from diffuse lighting
float CalcDiffuseLight(in vec3 surface_normal) {
    const float AMBIENT_LIGHT = 0.3;
    float brightness = dot(surface_normal, light_dir);
    // Shift and scale cosine curve to achieve a minimum brightness
    return 0.5 + 0.5 * AMBIENT_LIGHT + 0.5 * (1.0 - AMBIENT_LIGHT) * brightness;
}

// ClipVelocity taken from source-sdk-2013/<...>/src/game/shared/gamemovement.cpp
vec3 ClipVelocity(in vec3 obj_vel, in vec3 surface_normal) {
    const float overbounce = 1.0;
    
    // Determine how far along plane to slide based on incoming direction.
    float backoff = dot(obj_vel, surface_normal) * overbounce;
    vec3 clipped_vel = obj_vel - (backoff * surface_normal);

    // Iterate once to make sure we aren't still moving through the plane
    float adjust = dot(clipped_vel, surface_normal);
    if (adjust < 0.0)
        clipped_vel -= adjust * surface_normal;

    return clipped_vel;
}
// end of source-sdk-2013 code

// Send vertex color to fragment shader, optionally darkened by diffuse lighting
void UseVertexColor(in vec4 col, in vec3 surface_normal) {
    if (enable_diffuse_lighting)
        interpolated_color = vec4(CalcDiffuseLight(surface_normal) * col.rgb, col.a);
    else
        interpolated_color = col;
}

// csgo_vert_pos and csgo_vert_normal must describe a vertex in CSGO coordinate
// space, i.e. how the triangle's face is positioned and oriented just like in
// CSGO.
void CalcVertexColor(in vec4 csgo_vert_pos, in vec3 csgo_vert_normal) {
    if (enable_color_override) {
        UseVertexColor(override_color, csgo_vert_normal);
        return;
    }

    const vec4 irrelevant_col = vec4(0.4, 0.4, 0.4, 1.0);

    // Calculate trajectory of player ending up in the vertex position
    vec2 hori_vec = csgo_vert_pos.xy - player_pos.xy;
    float vert_dist = csgo_vert_pos.z - player_pos.z;
    float hori_dist = length(hori_vec);
    if (hori_dist < 0.01) { // Avoid division by zero
        UseVertexColor(irrelevant_col, csgo_vert_normal);
        return;
    }
    float impact_time = hori_dist / player_speed_hori;

    float initial_upwards_speed =
        (vert_dist + 0.5 * gravity * impact_time * impact_time)
        / impact_time;

    // Abort if vertical speed becomes illegally high
    if (abs(initial_upwards_speed) > max_vel) {
        UseVertexColor(irrelevant_col, csgo_vert_normal);
        return;
    }

    vec2 vel_hori = player_speed_hori * normalize(hori_vec);
    float impact_speed_vert = -gravity * impact_time + initial_upwards_speed;
    vec3 impact_vec = vec3(vel_hori.x, vel_hori.y, impact_speed_vert);

    // If cosine of the angle between surface normal and incoming player
    // direction is positive, the player approaches the surface from behind
    if (dot(impact_vec, csgo_vert_normal) > 0.0) {
        UseVertexColor(irrelevant_col, csgo_vert_normal);
        return;
    }

    // Velocity vector of player after the collision
    vec3 post_coll_vel = ClipVelocity(impact_vec, csgo_vert_normal);

    // We want to move upwards after surface collisions
    if (post_coll_vel.z < 0.0) {
        UseVertexColor(irrelevant_col, csgo_vert_normal);
        return;
    }

    float warn_amount = 0.0;

    // If ground is standable
    if (csgo_vert_normal.z > standable_normal) {
        // If our z velocity is too small, the rampslide fails
        if (post_coll_vel.z < min_no_ground_checks_vel_z) {
            UseVertexColor(irrelevant_col, csgo_vert_normal);
            return;
        }
        // If we are close to being ground checked -> warning color
        const float WARN_ZONE = 0.6;
        if (post_coll_vel.z < (1.0 + WARN_ZONE) * min_no_ground_checks_vel_z) {
            warn_amount = 1.0 -
                ((post_coll_vel.z - min_no_ground_checks_vel_z)
                / (WARN_ZONE * min_no_ground_checks_vel_z));
        }
    }

    float glidability = length(post_coll_vel) / length(impact_vec);
    if (glidability > 1.0)
        glidability = 1.0;
    if (glidability < 0.2)
        glidability = 0.2;
    
    UseVertexColor(vec4(0.7 * warn_amount, glidability, 0.0, 1.0), csgo_vert_normal);
}

void main() {
    // Determine world position and normal of current vertex

#ifdef INSTANCED_TRANSFORMATION
    // Apply model transformation to position
    vec4 csgo_vert_position = instanced_model_transformation * position;

    // Transform normal using the instance's model transformation. We assume
    // that the model transformation has a uniform scaling (X,Y and Z scalings
    // are identical). This method is incorrect for non-uniform transformations.
    vec3 csgo_vert_normal =
        normalize((instanced_model_transformation * vec4(normal, 0.0)).xyz);
#else
    vec4 csgo_vert_position = position;
    vec3 csgo_vert_normal   = normal;
#endif

    // Apply view and projection transformation
    gl_Position = final_transformation_matrix * csgo_vert_position;

    CalcVertexColor(csgo_vert_position, csgo_vert_normal);
}
