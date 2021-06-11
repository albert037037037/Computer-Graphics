#version 330

in vec2 texCoord;

// AS2
in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 aPos_2;

out vec4 fragColor;

// [TODO] passing texture from main.cpp
// Hint: sampler2D
uniform sampler2D textureFromMain;

// AS2
uniform mat4 um4p;
uniform mat4 um4v;
uniform mat4 um4m;
uniform mat4 viewMatrix;
uniform int lightIndex;
uniform float shiness;

uniform int is_perpixel;
uniform int thisIsEye;
uniform float xshift;
uniform float yshift;

struct LightInfo{
    vec3 position;
    vec3 spotDir;
    float spotExponent;
    float spotCutoff;
    vec3 diffuse;
    vec3 Ambient;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    // Not sure
    vec3 specular;
};
uniform LightInfo lightInfo[3];

struct PhongMaterial
{
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;

};
uniform PhongMaterial material;

vec3 directionalLight(vec3 Normal, vec3 V){
    vec4 viewingLight = viewMatrix * vec4(lightInfo[0].position, 1.0f);
    vec3 L = normalize(viewingLight.xyz + V);
    vec3 H = normalize(L + V);
    float dotNL = dot(Normal, L);
    float dotNH = dot(Normal, H);
    if(dotNH < 0) dotNH = 0;
    
    return lightInfo[0].Ambient * material.Ka + dotNL * lightInfo[0].diffuse * material.Kd + pow(dotNH, shiness) * lightInfo[0].specular * material.Ks;
}

vec3 positionLight(vec3 Normal, vec3 V){
    vec4 viewingLight = viewMatrix * vec4(lightInfo[1].position, 1.0f);
    vec4 viewingVertex = um4p * um4v * um4m * vec4(aPos_2.x, aPos_2.y, aPos_2.z, 1.0);
    vec3 L = normalize(viewingLight.xyz + V);
    vec3 H = normalize(L + V);
    float dotNL = dot(Normal, L);
    float dotNH = dot(Normal, H);
    if(dotNH < 0) dotNH = 0;
    
    
    float dist = length(viewingVertex.xyz - viewingLight.xyz);
    float f = 1.0 / (lightInfo[1].constantAttenuation + lightInfo[1].linearAttenuation * dist + lightInfo[1].quadraticAttenuation * dist * dist);
    if(f > 1.0) f = 1.0;
    
    return lightInfo[1].Ambient * material.Ka + f * (dotNL * lightInfo[1].diffuse * material.Kd + pow(dotNH, shiness) * lightInfo[1].specular * material.Ks);
}

vec3 spotLight(vec3 Normal, vec3 V){
    vec4 viewingLight = viewMatrix * vec4(lightInfo[2].position, 1.0f);
    vec4 viewingVertex = um4p * um4v * um4m * vec4(aPos_2.x, aPos_2.y, aPos_2.z, 1.0);
    vec3 L = normalize(viewingLight.xyz + V);
    vec3 H = normalize(L + V);
    float dotNL = dot(Normal, L);
    float dotNH = dot(Normal, H);
    if(dotNH < 0) dotNH = 0;
    

    float dist = length(viewingVertex.xyz - viewingLight.xyz);
    float f = 1.0 / (lightInfo[2].constantAttenuation + lightInfo[2].linearAttenuation * dist + lightInfo[2].quadraticAttenuation * dist * dist);
    if(f > 1.0) f = 1.0;
    
    vec3 normVL = normalize(viewingVertex.xyz - lightInfo[2].position);
    vec3 normSpotdir = normalize(lightInfo[2].spotDir);
    
    float dotVspotD = dot(normVL, normSpotdir);
    float spotEffect;
    if(dotVspotD > cos(lightInfo[2].spotCutoff * 0.017453)) {
        spotEffect = pow(max(dotVspotD, 0), lightInfo[2].spotExponent);
    }
    else {
        spotEffect = 0;
    }

    return lightInfo[2].Ambient * material.Ka + spotEffect * f * (dotNL * lightInfo[2].diffuse * material.Kd + pow(dotNH, shiness) * lightInfo[2].specular * material.Ks);
}


void main() {
    
    vec4 viewingVertex = um4p * um4v * um4m * vec4(aPos_2.x, aPos_2.y, aPos_2.z, 1.0);
    vec3 Normal = normalize(vertex_normal);
    vec3 V = -viewingVertex.xyz;
    vec3 color = vec3(0, 0, 0);

    if(lightIndex == 0) {
        color = directionalLight(Normal, V);
    }
    else if(lightIndex == 1) {
        color = positionLight(Normal, V);
    }
    else {
        color = spotLight(Normal, V);
    }
    fragColor = (is_perpixel == 0) ? vec4(vertex_color, 1.0f) : vec4(color, 1.0f);

	// [TODO] sampleing from texture
	// Hint: texture
    if(thisIsEye == 1) {
        fragColor *= texture(textureFromMain, texCoord + vec2(xshift, yshift));
    }
    else {
        fragColor *= texture(textureFromMain, texCoord);
    }
}
