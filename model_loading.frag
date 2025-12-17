#version 460 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;
uniform sampler2D groundTex;

uniform vec3  lightDir;
uniform vec3  overrideColor;
uniform mat4  lightSpaceMatrix;
uniform int   useTexture;     


uniform bool  qteVisible;
uniform float qteInnerRadius;
uniform float qteOuterRadius;
uniform vec3  qteInnerColor;
uniform vec3  qteOuterColor;
uniform float qteAspect;   // NEW
uniform vec2 qteScreenSize;

uniform int   isUI;         


float StarMask(vec2 p)
{
    // scale
    p *= 1.1;

    const float PI = 3.14159265;
    float an = atan(p.y, p.x);
    float r  = length(p);

    //rotate
    an += PI * 0.5;

    // 5 points = 2*pi/5 sector
    float k = 5.0;
    float sector = 2.0 * PI / k;

    //bring angle into [0, sector)
    an = mod(an, sector);

    float m = abs(an - sector * 0.5) / (sector * 0.5);

    //inner vs outer radius
    float R  = 1.0;   // outer radius
    float Ri = 0.45;  // inner radius (star "valleys")

    //star radius at this angle
    float starR = mix(R, Ri, m);

    //signed distance >0 outside <0 inside
    float d = r - starR;

    // hard edge star 1 inside 0 outside
    float mask = step(d, 0.0);

    //small feather to avoid aliasing
    float feather = 0.01;
    return smoothstep(feather, -feather, d) * mask;
}


//shadow calculation
float CalcShadow(vec4 lightSpacePos)
{
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float shadow = 0.0;
    float bias = 0.0025;
    float currentDepth = projCoords.z;

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float pcfDepth = texture(shadowMap,
                                     projCoords.xy + vec2(x,y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth ? 1.0 : 0.0);
        }
    }

    shadow /= 9.0;
    shadow = clamp(shadow, 0.0, 0.8);
    return shadow;
}

void main()
{
    vec3 baseColor;

    if (overrideColor.x >= 0.0)
    {
        baseColor = overrideColor;
    }
    else if (useTexture == 1)
    {
        baseColor = texture(groundTex, TexCoords).rgb;
    }
    else
    {
        vec3 texColor = texture(texture_diffuse1, TexCoords).rgb;
        if (texColor == vec3(0.0))
            baseColor = vec3(1.0);
        else
            baseColor = texColor;
    }

    vec3 N = normalize(Normal);
    vec3 L = normalize(-lightDir);

    float NdotL = max(dot(N, L), 0.0);
    float shadow = CalcShadow(lightSpaceMatrix * vec4(FragPos, 1.0));

    vec3 ambient = baseColor * 0.3;
    float lit = NdotL * (1.0 - shadow);
    vec3 color = ambient + baseColor * lit;

    if (qteVisible)
    {
        //centered circle overlay
        vec2 ndc = (gl_FragCoord.xy / qteScreenSize) * 2.0 - 1.0; 
        ndc.x *= qteAspect;                                      //keep circle round

        float r = length(ndc);

        if (r <= qteInnerRadius)
        {
            color = mix(color, qteInnerColor, 0.8);
        }
        else if (r <= qteOuterRadius)
        {
            float t = smoothstep(qteInnerRadius, qteOuterRadius, r);
            t = 1.0 - t;
            color = mix(color, qteOuterColor, t * 0.8);
        }
    }

    //UI star shaping
    if (isUI == 1)
    {
        vec2 uv = TexCoords;
        if (uv == vec2(0.0))
        {
            uv = vec2(0.5);
        }

        vec2 p = uv * 2.0 - 1.0;

        float m = StarMask(p);
        if (m <= 0.01)
            discard;

        vec3 gold = vec3(1.2, 1.0, 0.3);
        color = mix(color, gold, m);
    }

    FragColor = vec4(color, 1.0);
}