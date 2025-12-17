#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTex;
uniform float colorSteps;
uniform float wobbleAmount;

void main()
{
    vec2 uv = TexCoords;

    if (wobbleAmount > 0.0)
    {
        float wx = sin(uv.y * 480.0) * wobbleAmount;
        float wy = cos(uv.x * 640.0) * wobbleAmount;
        uv += vec2(wx, wy) * 0.001;
    }

    vec3 col = texture(sceneTex, uv).rgb;

    float steps = max(colorSteps, 1.0);
    col = floor(col * steps) / steps;

    FragColor = vec4(col, 1.0);
}