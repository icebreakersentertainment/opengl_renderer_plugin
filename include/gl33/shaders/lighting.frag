// Source: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/8.1.deferred_shading/8.1.deferred_shading.fs
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gMetallicRoughnessAmbientOcclusion;
uniform sampler2D shadowMap;

struct Light
{
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
};

struct DirectionalLight
{
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

const int NR_LIGHTS = 6;
const int NR_DIRECTIONAL_LIGHTS = 1;
uniform Light lights[NR_LIGHTS];
uniform DirectionalLight directionalLights[NR_DIRECTIONAL_LIGHTS];
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform mat4 lightSpaceMatrix;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
/*
vec3 tangentNormalSpaceToWorldSpace(vec3 tangentNormal, const vec3 WorldPos, const vec3 TexCoords)
{
    tangentNormal = tangentNormal * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}
*/
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
// ----------------------------------------------------------------------------
float ShadowCalculation(const vec4 fragPosLightSpace, const vec3 surfaceNormal, const vec3 lightDirection)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0)
    {
        return 0.0;
	}
    
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    float bias = max(0.05 * (1.0 - dot(surfaceNormal, lightDirection)), 0.005);
    
    // check whether current frag pos is in shadow
    
    float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for(int x = -1; x <= 1; ++x)
	{
	    for(int y = -1; y <= 1; ++y)
	    {
	        float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
	        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
	    }    
	}
	shadow /= 9.0;
    
    //float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    return shadow;
}
/*
float ShadowCalculation(const vec4 fragPosLightSpace, const vec3 surfaceNormal, const vec3 lightDirection)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0)
    {
        return 0.0;
	}
    
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    float bias = max(0.05 * (1.0 - dot(surfaceNormal, lightDirection)), 0.005);
    
    // check whether current frag pos is in shadow
    
    float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for(int x = -1; x <= 1; ++x)
	{
	    for(int y = -1; y <= 1; ++y)
	    {
	        float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
	        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
	    }    
	}
	shadow /= 9.0;
    
    //float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

vec3 calculatePointLight(const vec3 FragPos, const vec3 Normal, const vec3 Diffuse, const float Specular)
{
	vec3 lighting = vec3(0);
	vec3 viewDir  = normalize(viewPos - FragPos);
	
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // diffuse
        vec3 lightDir = normalize(lights[i].Position - FragPos);
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].Color;
        
        // specular
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
        vec3 specular = lights[i].Color * spec * Specular;
        
        // attenuation
        float distance = length(lights[i].Position - FragPos);
        float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
        
        diffuse *= attenuation;
        specular *= attenuation;
        lighting += diffuse + specular;        
    }
    
    return lighting;
}

vec3 calculateDirectionalLight(const vec3 FragPos, const vec3 Normal, const vec3 Diffuse, const float Specular)
{
	vec3 lighting = vec3(0);

	for(int i = 0; i < NR_DIRECTIONAL_LIGHTS; ++i)
	{
	    float shininess = 16.0f;
		
		vec3 norm = normalize(Normal);
	    vec3 lightDir = normalize(-directionalLights[i].direction);  
	    
		// diffuse 
	    float diff = max(dot(norm, lightDir), 0.0);
	    vec3 diffuse = directionalLights[i].diffuse * diff * Diffuse.rgb;  
	    
	    // specular
	    vec3 viewDir = normalize(viewPos - FragPos);
	    vec3 reflectDir = reflect(-lightDir, norm);  
	    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	    vec3 specular = directionalLights[i].specular * spec * Specular;//.rgb;  
		
		// Shadow mapping
	    vec4 FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
	    float shadow = ShadowCalculation(FragPosLightSpace, Normal, lightDir);
	    //lighting -= (0.2 * vec3(shadow, shadow, shadow));
    
        lighting += (1.0 - shadow) * (diffuse + specular);
        
		//lighting += diffuse + specular;
	}
	
    return lighting;
}
*/
void main()
{             
    // retrieve data from gbuffer
    vec3 WorldPos = texture(gPosition, TexCoords).rgb;
    vec3 tangentNormal = texture(gNormal, TexCoords).rgb;
    vec3 albedo = pow(texture(gAlbedoSpec, TexCoords).rgb, vec3(2.2));
    float metallic  = texture(gMetallicRoughnessAmbientOcclusion, TexCoords).r;
    float roughness = texture(gMetallicRoughnessAmbientOcclusion, TexCoords).g;
	float ao = texture(gMetallicRoughnessAmbientOcclusion, TexCoords).b;
	
	vec3 N = tangentNormal;//tangentNormalSpaceToWorldSpace(tangentNormal, WorldPos, TexCoords);
    vec3 V = normalize(viewPos - WorldPos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // calculate per-light radiance
        vec3 L = normalize(lights[i].Position - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(lights[i].Position - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lights[i].Color * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 nominator    = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // 0.001 to prevent divide by zero.
        vec3 specular = nominator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    for(int i = 0; i < NR_DIRECTIONAL_LIGHTS; ++i)
    {
        // calculate per-light radiance
        //vec3 L = normalize(directionalLights[i].Position - WorldPos);
        vec3 L = normalize(-directionalLights[i].direction);
        vec3 H = normalize(V + L);
        //float distance = length(directionalLights[i].Position - WorldPos);
        float attenuation = 1.0;// / (distance * distance);
        vec3 radiance = directionalLights[i].ambient * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 nominator    = NDF * G * F; 
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // 0.001 to prevent divide by zero.
        vec3 specular = nominator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        //Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again

        // Shadow mapping
	    vec4 FragPosLightSpace = lightSpaceMatrix * vec4(WorldPos, 1.0);
	    float shadow = ShadowCalculation(FragPosLightSpace, normalize(tangentNormal), L);
	    //lighting -= (0.2 * vec3(shadow, shadow, shadow));
    
        vec3 lightingWithShadow = (1.0 - shadow) * ((kD * albedo / PI + specular) * radiance * NdotL); // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
        Lo += lightingWithShadow;
    }
    
    // ambient lighting (note that the next IBL tutorial Iwill replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.003) * albedo * ao;
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));

	FragColor = vec4(color, 1.0);
    
    /*
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    
    // then calculate lighting as usual
    vec3 lighting  = Diffuse * 0.001; // hard-coded ambient component
    
    lighting += calculatePointLight(FragPos, Normal, Diffuse, Specular);
    lighting += calculateDirectionalLight(FragPos, Normal, Diffuse, Specular);
    
    FragColor = vec4(lighting, 1.0);
    */
}
