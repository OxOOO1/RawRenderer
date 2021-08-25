
#include "UniformBuffer.hlsli"
#include "LightDesc.h"


void Fresnel( inout float3 specular, inout float3 diffuse, float3 lightDir, float3 halfVec )
{
    float fresnel = pow(1.0 - saturate(dot(lightDir, halfVec)), 5.0);
    specular = lerp(specular, 1, fresnel);
    diffuse = lerp(diffuse, 0, fresnel);
}


float3 CalculateGlobalLight(const float3 inColor, const float3 normal)
{
	//diffuse intensity
	float3 Diffuse = inColor * GGlobalLight.Color * GGlobalLight.Intensity;
    return Diffuse * max(GGlobalLight.AmbientLightIntensity, dot(-GGlobalLight.DirectionViewSpace, normal));
}

float3 CalculatePointLight(LightDesc lightDesc, const float3 position, const float3 inColor, const float3 normal, const float specularPower)
{
	//Dir
    float3 vToLight = lightDesc.Position - position;
    float vToLightLength = sqrt(dot(vToLight, vToLight));
    vToLight /= vToLightLength; //normalize

	//Fade
	float DistanceFalloff = saturate(1.f - pow(vToLightLength / lightDesc.Radius, 2));

	//Lambert
    float3 diffuse = inColor * lightDesc.Color * lightDesc.Intensity * max(0.f, dot(vToLight, normal));

    //Phong
    float3 vToCam = normalize(GCameraPosition - position);
    float3 halfVec = normalize(vToLight + vToCam);
    float specular = saturate(dot(halfVec, normal));
    specular = pow(specular, specularPower) * (specularPower + 2) / 8;

    //Fresnel(inColor, inColor, vToLight, halfVec );

    return DistanceFalloff * (diffuse);// + specular);

}

float3 CalculateSpotLight(LightDesc lightDesc, const float3 position, const float3 inColor, const float3 normal, const float specularPower)
{
	//Dir
    float3 vToLight = lightDesc.Position - position;
    float vToLightLength = sqrt(dot(vToLight, vToLight));
    vToLight /= vToLightLength; //normalize

	//Fade
	float DistanceFalloff = 1.f - pow(vToLightLength / lightDesc.Radius, 2);

	//Spotlight fade
    float2 SpotlightAngles = lightDesc.SpotlightAngles;
    float SpotlightFalloff = dot(-vToLight, lightDesc.Direction);

     // x = 1.0f / (cos(coneInner) - cos(coneOuter)), y = cos(coneOuter)
    SpotlightAngles.x = 1.f / (SpotlightAngles.x - SpotlightAngles.y);
    SpotlightFalloff = saturate((SpotlightFalloff - SpotlightAngles.y) * SpotlightAngles.x);

	//Lambert
    float3 diffuse = inColor * lightDesc.Color * lightDesc.Intensity * max(0.f, dot(vToLight, normal));

    //Phong
    float3 vToCam = normalize(GCameraPosition - position);
    float3 halfVec = normalize(vToLight + vToCam);
    float specular = saturate(dot(halfVec, normal));
    specular = pow(specular, specularPower) * (specularPower + 2) / 8;

    //Fresnel(inColor, inColor, vToLight, halfVec );

    return SpotlightFalloff * DistanceFalloff * (diffuse);// + specular);
} 

