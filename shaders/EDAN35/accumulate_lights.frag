#version 410
#define M_PI 3.1415926535897932384626433832795
uniform sampler2D depth_texture;
uniform sampler2D normal_texture;
uniform sampler2DShadow shadow_texture;

uniform vec2 inv_res;

uniform mat4 view_projection_inverse;
uniform vec3 camera_position;
uniform mat4 shadow_view_projection;

uniform vec3 light_color;
uniform vec3 light_position;
uniform vec3 light_direction;
uniform float light_intensity;
uniform float light_angle_falloff;

uniform vec2 shadowmap_texel_size;

layout (location = 0) out vec4 light_diffuse_contribution;
layout (location = 1) out vec4 light_specular_contribution;


void main()
{
	float pi = 3.1415926;
	vec2 texCoord = inv_res*gl_FragCoord.xy;
	vec3 Nn = (2*texture(normal_texture,texCoord)-1).xyz;
	//
	vec4 depth = texture(depth_texture,texCoord);
	vec4 worldPosition = view_projection_inverse*vec4(texCoord*2-1,2*depth.x-1,1.0);
	worldPosition /= worldPosition.w;
	vec3 L =  light_position - worldPosition.xyz;
	vec3 V =  camera_position - worldPosition.xyz;

	vec3 Ln = normalize(L);
	vec3 Vn = normalize(V);
	vec3 Rn = reflect(-Ln,Nn);

	float diffuse = max(dot(Ln,Nn),0.0);
	float specular = pow(max(dot(Rn,Vn),0.0),10.0);

	float dist = max(distance(light_position,worldPosition.xyz),0.01);
	float angle = acos(dot(normalize(light_direction),-Ln));
	float ang_fall = max(1.0 - (angle*4.0/pi),0.0);
	//float ang_fall = smoothstep(0,pi/4,angle);
	float li = ang_fall / pow(dist,2);
	
	//Shadow calculation
	//worldspace to shadow_projection space
	vec4 shadowPos =  shadow_view_projection * worldPosition;
	shadowPos /= shadowPos.w;

	float cosTheta = clamp(dot(Nn,Ln),0.0,1.0);
	float bias = clamp(0.000005*tan(acos(cosTheta)),0.0,0.00001);
	//bias = 0.00001;
	vec3 shadowTexCoord = vec3(shadowPos.xy,shadowPos.z)*0.5+0.5;
	shadowTexCoord.z -= bias;
	//shadowTexCoord.z -= bias;
	float shadowDepth = texture(shadow_texture,shadowTexCoord);
	float visibility = 1.0;


	//float accu_dep = 0.0;
	for (int i = -2;i<3;i++){
		for(int j = -2; j<3; j++){
			 shadowDepth = texture(shadow_texture,vec3(shadowTexCoord.x+shadowmap_texel_size.x*i,shadowTexCoord.y+shadowmap_texel_size.y*j,shadowTexCoord.z));
			 	if (shadowDepth == 0.0){				
					visibility -= 1.0/25.0;
				}
		}
	}

	//accu_dep/=9;
	//float bias = 0.1;

	
	light_diffuse_contribution  = vec4(visibility*li*light_intensity*light_color * diffuse,1.0);
	light_specular_contribution = vec4(visibility*li*light_intensity*light_color * specular, 1.0);
}
