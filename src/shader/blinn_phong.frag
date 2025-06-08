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

out vec4 fragColor;

uniform vec3 uViewPos;
uniform mat4 uModel;
uniform Light_Directional uLightSun;
uniform Light_Spot uLightSpots[4];
uniform Material uMaterial;
uniform samplerCube uSkybox;

vec3 brdf_blinn_phong(vec3 lightDir, vec3 viewDir, vec3 normal, vec3 diffuse, vec3 specular, float shininess)
{
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    return (diff * diffuse) + (spec * specular);
}

void main(void)
{
    vec3 viewDir = normalize(uViewPos - tFragPos);

    // Retrieve the normal from the normal map, transform it to [-1, 1] range and transform it into world space
    vec3 normalMap = normalize(mat3(transpose(inverse(uModel))) * (texture(uMaterial.normal, tUV).rgb * 2.0 - 1.0));

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

        illuminance += uLightSpots[i].color * intensity  * attenuation * brdf_blinn_phong(lightDir, viewDir, normalMap, diffuseColor, specularColor, uMaterial.shininess);
    }

    illuminance += uLightSun.color * brdf_blinn_phong(-normalize(uLightSun.direction), viewDir, normalMap, diffuseColor, specularColor, uMaterial.shininess);

    /**----- Reflection with Environment Mapping ------*/

    vec3 I = normalize(tFragPos - uViewPos);
    vec3 R = reflect(I, normalize(normalMap));
    illuminance += vec3(texture(uSkybox, R)) * specularColor * uLightSun.color;

    fragColor = vec4(illuminance, 1.0);
}