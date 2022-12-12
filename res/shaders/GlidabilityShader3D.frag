// The GLSL version preprocessor directive is added programmatically

in lowp vec4 interpolated_color;

out lowp vec4 fragmentColor;

void main() {
    fragmentColor = interpolated_color;
}
