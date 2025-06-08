#version 330 core

struct Light_Directional
{
    vec3 direction;
    vec3 ambient;
    vec3 color;
};

struct Light_Spot
{
    vec3 position;
    vec3 direction;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
    float cutoff;
    bool enabled;
};

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
    sampler2D ambient;
    float shininess;
};

in vec3 tNormal;
in vec3 tFragPos;
in vec2 tUV;

out vec4 FragColor;

uniform mat4 uProj;
uniform mat4 uView;
uniform vec3 uViewPos;
uniform mat4 uModel;
uniform Light_Directional uLightSun;
uniform Light_Spot uLightSpots[4];
uniform Material uMaterial;
uniform samplerCube uSkybox;
uniform sampler2D uBoatColor;
uniform sampler2D uBoatDepth;
uniform bool uUseBinarySearch;

vec3 brdf_blinn_phong(vec3 lightDir, vec3 viewDir, vec3 normal, vec3 diffuse, vec3 specular, float shininess)
{
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    return (diff * diffuse) + (spec * specular);
}
vec2 transformViewSpaceToUV(vec3 position) {
    vec4 transformed = uProj * vec4(position, 1.f);
    transformed.xy = (transformed.xy / transformed.w) * 0.5 + 0.5;
    return transformed.xy;
}
float getBoatDepth(sampler2D depthTexture, vec2 uv) {
    float depth = texture(depthTexture, uv).x;
//    create corresponding point in normalized device coordinates
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.f);
//    transform back to view space
    vec4 inversed = inverse(uProj) * ndc;
    return inversed.z / inversed.w;
}

// see: https://github.com/RoundedGlint585/ScreenSpaceReflection/blob/master/shaders/SSRFragment.glsl
vec3 SSR(vec3 position, vec3 reflection) {
//    Transform position and reflection into view space so that depth value (marchingPosition.z) can be compared
    position = vec3(uView * vec4(position, 1.0));
    reflection = normalize(vec3(uView * vec4(reflection, 0.0)));
    vec3 specularColor = texture(uMaterial.specular, tUV).xyz;

    int maxSteps = 150;
    float stepSize = 0.2f;
    float distanceBias = 0.6f;
    float distanceBias2 = 0.05f;
    float delta;
    float boatDepth;
    vec2 uv;
    vec3 step;
    vec3 marchingPosition;
    vec3 resColor = vec3(0.0);

    step = stepSize * reflection;
    marchingPosition = position + step;
    bool hit = false;

    for (int i = 0; i < maxSteps; i++) {
        uv = transformViewSpaceToUV(marchingPosition);
//        in OpenGL z value in view space is negative -> take absolute value for all depths
        boatDepth = abs(getBoatDepth(uBoatDepth, uv));
        delta = abs(marchingPosition.z) - boatDepth;

        if (abs(delta) < distanceBias) {
            hit = true;
            resColor =  texture(uBoatColor, uv).xyz * specularColor;
            break;
        } else {
            marchingPosition += step;
        }
    }
    if (!hit || !uUseBinarySearch) {
        return resColor;
    }
//    refine result with binary search
    step *= 0.5;
//    if delta is negative (= marching position is in front of the boat), move along reflection
    marchingPosition += step * -sign(delta);
    for (int i = 0; i < 150; i++) {
        uv = transformViewSpaceToUV(marchingPosition);
        boatDepth = abs(getBoatDepth(uBoatDepth, uv));
        delta = abs(marchingPosition.z) - boatDepth;

        if (abs(delta) < distanceBias2) {
            resColor = texture(uBoatColor, uv).xyz * specularColor;
            break;
        } else {
            step *= 0.5;
            marchingPosition += step * -sign(delta);
        }
    }
    return resColor;

}

void main(void)
{
    vec3 viewDir = normalize(uViewPos - tFragPos);

    // Retrieve the normal from the normal map, transform it to [-1, 1] range and transform it into world space
    vec3 normalMap = normalize(mat3(transpose(inverse(uModel))) * (texture(uMaterial.normal, tUV).rgb * 2.0 - 1.0));

    // Compute the final normal for the water surface
    vec3 waterSurfaceNormal = normalize(0.25 * normalMap + tNormal);

    // Use texture maps for material properties
    vec3 ambientColor = texture(uMaterial.ambient, tUV).rgb;
    vec3 diffuseColor = texture(uMaterial.diffuse, tUV).rgb;
    vec3 specularColor = texture(uMaterial.specular, tUV).rgb;

    vec3 illuminance = uLightSun.ambient * diffuseColor * ambientColor;

    for(int i = 0; i < 4; i++)
    {
        if(uLightSpots[i].enabled == false) continue;

        vec3 lightDir = normalize(uLightSpots[i].position - tFragPos);
        float distance = length(uLightSpots[i].position - tFragPos);
        float attenuation = 1.0 / (uLightSpots[i].constant + uLightSpots[i].linear * distance + uLightSpots[i].quadratic * (distance * distance));

        vec3 spotDir = normalize(-uLightSpots[i].direction);
        float angle = dot(spotDir, lightDir);
        float intensity = (angle > cos(uLightSpots[i].cutoff)) ? 1.0 : 0.0;

        illuminance += uLightSpots[i].color * intensity  * attenuation * brdf_blinn_phong(lightDir, viewDir, waterSurfaceNormal, diffuseColor, specularColor, uMaterial.shininess);
    }

    illuminance += uLightSun.color * brdf_blinn_phong(-normalize(uLightSun.direction), viewDir, waterSurfaceNormal, diffuseColor, specularColor, uMaterial.shininess);

    /**----- Reflection with Environment Mapping ------*/

    vec3 I = normalize(tFragPos - uViewPos);
    vec3 R = normalize(reflect(I, normalize(waterSurfaceNormal)));
    illuminance += vec3(texture(uSkybox, R)) * specularColor * uLightSun.color;

    /**----------- Screen Space Reflection -----------*/

    illuminance += SSR(tFragPos, R);

    FragColor = vec4(illuminance, 1.0);
}