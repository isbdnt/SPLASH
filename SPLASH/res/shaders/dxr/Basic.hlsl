#define HLSL
#include "Common_hlsl_cpp.hlsl"

RaytracingAccelerationStructure gRtScene 			: register(t0);
Texture2D<float4> sys_brdfLUT 						: register(t5);
Texture2D<float4> sys_inTex_normals 				: register(t10);
Texture2D<float4> sys_inTex_albedo 					: register(t11);
Texture2D<float4> sys_inTex_texMetalnessRoughnessAO : register(t12);
Texture2D<float>  sys_inTex_depth 					: register(t13);

RWTexture2D<float4> lOutput : register(u0);

ConstantBuffer<SceneCBuffer> CB_SceneData : register(b0, space0);
ConstantBuffer<MeshCBuffer> CB_MeshData : register(b1, space0);
StructuredBuffer<Vertex> vertices : register(t1, space0);
StructuredBuffer<uint> indices : register(t1, space1);

// Texture2DArray<float4> textures : register(t2, space0);
Texture2D<float4> sys_texAlbedo : register(t2);
Texture2D<float4> sys_texNormal : register(t3);
Texture2D<float4> sys_texMetalnessRoughnessAO : register(t4);

SamplerState ss : register(s0);

#include "Utils.hlsl"
#include "Shading.hlsl"

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void generateCameraRay(uint2 index, out float3 origin, out float3 direction) {
	float2 xy = index + 0.5f; // center in the middle of the pixel.
	float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates.
	screenPos.y = -screenPos.y;

	// Unproject the pixel coordinate into a ray.
	float4 world = mul(CB_SceneData.projectionToWorld, float4(screenPos, 0, 1));

	world.xyz /= world.w;
	origin = CB_SceneData.cameraPosition.xyz;
	direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void rayGen() {
	uint2 launchIndex = DispatchRaysIndex().xy;

#define TRACE_FROM_GBUFFERS
#ifdef TRACE_FROM_GBUFFERS
	float2 screenTexCoord = ((float2)launchIndex + 0.5f) / DispatchRaysDimensions().xy;

	// Use G-Buffers to calculate/get world position, normal and texture coordinates for this screen pixel
	// G-Buffers contain data in world space
	float3 worldNormal = sys_inTex_normals.SampleLevel(ss, screenTexCoord, 0).rgb * 2.f - 1.f;
	float3 albedoColor = sys_inTex_albedo.SampleLevel(ss, screenTexCoord, 0).rgb;
	// float3 albedoColor = pow(sys_inTex_albedo.SampleLevel(ss, screenTexCoord, 0).rgb, 2.2f);
	float3 metalnessRoughnessAO = sys_inTex_texMetalnessRoughnessAO.SampleLevel(ss, screenTexCoord, 0).rgb;
	float metalness = metalnessRoughnessAO.r;
	float roughness = metalnessRoughnessAO.g;
	float ao = metalnessRoughnessAO.b;

	// ---------------------------------------------------
	// --- Calculate world position from depth texture ---

	// TODO: move calculations to cpu
	float projectionA = CB_SceneData.farZ / (CB_SceneData.farZ - CB_SceneData.nearZ);
    float projectionB = (-CB_SceneData.farZ * CB_SceneData.nearZ) / (CB_SceneData.farZ - CB_SceneData.nearZ);

	float depth = sys_inTex_depth.SampleLevel(ss, screenTexCoord, 0);
	float linearDepth = projectionB / (depth - projectionA);

	float2 screenPos = screenTexCoord * 2.0f - 1.0f;
	screenPos.y = -screenPos.y; // Invert Y for DirectX-style coordinates.
	
    float3 screenVS = mul(CB_SceneData.clipToView, float4(screenPos, 0.f, 1.0f)).xyz;
	float3 viewRay = float3(screenVS.xy / screenVS.z, 1.f);

	// float3 viewRay = normalize(float3(screenPos, 1.0f));
	float4 vsPosition = float4(viewRay * linearDepth, 1.0f);

	float3 worldPosition = mul(CB_SceneData.viewToWorld, vsPosition).xyz;
	// ---------------------------------------------------

	RayPayload payload;
	payload.recursionDepth = 1;
	payload.hit = 0;
	payload.color = float4(0,0,0,0);
	shade(worldPosition, worldNormal, albedoColor, metalness, roughness, ao, payload);
	lOutput[launchIndex] = payload.color;


	// float2 integratedBRDF = IntegrateBRDF(screenTexCoord.x, 1.0f - screenTexCoord.y);
    // lOutput[launchIndex] = float4(integratedBRDF / 2.0f, 0.f, 1.0f);	


#else
	// Fully RT

	float3 rayDir;
	float3 origin;
	// Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
	generateCameraRay(launchIndex, origin, rayDir);

	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = rayDir;
	// Set TMin to a non-zero small value to avoid aliasing issues due to floating point errors
	// TMin should be kept small to prevent missing geometry at close contact areas
	ray.TMin = 0.00001;
	ray.TMax = 10000.0;

	RayPayload payload;
	payload.recursionDepth = 0;
	payload.hit = 0;
	payload.color = float4(0,0,0,0);
	TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);

	lOutput[launchIndex] = payload.color;
#endif
}

[shader("miss")]
void miss(inout RayPayload payload) {
	payload.color = float4(0.01f, 0.01f, 0.01f, 1.0f);
}

float3 getAlbedo(MeshData data, float2 texCoords) {
	float3 color = data.color.rgb;
	if (data.flags & MESH_HAS_ALBEDO_TEX)
		color *= sys_texAlbedo.SampleLevel(ss, texCoords, 0).rgb;
		// color *= pow(sys_texAlbedo.SampleLevel(ss, texCoords, 0).rgb, 2.2f);

	return color;
}
float3 getMetalnessRoughnessAO(MeshData data, float2 texCoords) {
	float3 color = data.metalnessRoughnessAoScales;
	if (data.flags & MESH_HAS_METALNESS_ROUGHNESS_AO_TEX)
		color *= sys_texMetalnessRoughnessAO.SampleLevel(ss, texCoords, 0).rgb;
	return color;
}

[shader("closesthit")]
void closestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	payload.recursionDepth++;

	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint instanceID = InstanceID();
	uint primitiveID = PrimitiveIndex();

	uint verticesPerPrimitive = 3;
	uint i1 = primitiveID * verticesPerPrimitive;
	uint i2 = primitiveID * verticesPerPrimitive + 1;
	uint i3 = primitiveID * verticesPerPrimitive + 2;
	// Use indices if available
	if (CB_MeshData.data[instanceID].flags & MESH_USE_INDICES) {
		i1 = indices[i1];
		i2 = indices[i2];
		i3 = indices[i3];
	}
	Vertex vertex1 = vertices[i1];
	Vertex vertex2 = vertices[i2];
	Vertex vertex3 = vertices[i3];

	float2 texCoords = Utils::barrypolation(barycentrics, vertex1.texCoords, vertex2.texCoords, vertex3.texCoords);
	float3 normalInLocalSpace = Utils::barrypolation(barycentrics, vertex1.normal, vertex2.normal, vertex3.normal);
	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), normalInLocalSpace));

	float3 tangentInLocalSpace = Utils::barrypolation(barycentrics, vertex1.tangent, vertex2.tangent, vertex3.tangent);
	float3 tangentInWorldSpace = normalize(mul(ObjectToWorld3x4(), tangentInLocalSpace));

	float3 bitangentInLocalSpace = Utils::barrypolation(barycentrics, vertex1.bitangent, vertex2.bitangent, vertex3.bitangent);
	float3 bitangentInWorldSpace = normalize(mul(ObjectToWorld3x4(), bitangentInLocalSpace));

	// Create TBN matrix to go from tangent space to world space
	float3x3 tbn = float3x3(
	  tangentInWorldSpace,
	  bitangentInWorldSpace,
	  normalInWorldSpace
	);
	if (CB_MeshData.data[instanceID].flags & MESH_HAS_NORMAL_TEX) {
        normalInWorldSpace = mul(normalize(sys_texNormal.SampleLevel(ss, texCoords, 0).rgb * 2.f - 1.f), tbn) / 2.f + .5f;
	}


	float3 albedoColor = getAlbedo(CB_MeshData.data[instanceID], texCoords);
	float3 metalnessRoughnessAO = getMetalnessRoughnessAO(CB_MeshData.data[instanceID], texCoords);
	float metalness = metalnessRoughnessAO.r;
	float roughness = metalnessRoughnessAO.g;
	float ao = metalnessRoughnessAO.b;

	shade(Utils::HitWorldPosition(), normalInWorldSpace, albedoColor, metalness, roughness, ao, payload, true);
}