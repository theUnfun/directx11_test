
struct VertexInput
{
	float4 pos : POSITION;
	float3 color : COLOR0;
	float3 normal : NORMAL;
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
	float3 normal : NORMAL;
};

//--------------------------------------------------------------------------------------
// Переменные константных буферов
//--------------------------------------------------------------------------------------
cbuffer SceneConstantBuffer : register(b0)  // b0 - индекс буфера
{
	matrix View;
	matrix Projection;
	float4 LightDir[2];
	float4 LightColor[2];
}

cbuffer ObjectConstantBuffer : register(b1)  // b0 - индекс буфера
{
	matrix World;
	float4 OutputColor;
}

//--------------------------------------------------------------------------------------
// Вершинный шейдер
//--------------------------------------------------------------------------------------
void VS(VertexInput vertex_input, out VertexOutput vertex_output)
{
	vertex_output.pos = mul(vertex_input.pos, World);        // сначала в пространство мира
	vertex_output.pos = mul(vertex_output.pos, View);        // затем в пространство вида
	vertex_output.pos = mul(vertex_output.pos, Projection);  // в проекционное пространство
	vertex_output.normal = mul(vertex_input.normal, World);

	// vertex_output.color = vertex_input.color;
	vertex_output.color = 1.0f;
}


//--------------------------------------------------------------------------------------
// Пиксельный шейдер
//--------------------------------------------------------------------------------------
float4 PS(VertexOutput pixel_input) : SV_Target
{
	// float4 finalColor = float4(pixel_input.color,1.0f);
	float4 final_color = 0;

	// складываем освещенность пикселя от всех источников света
	for (int i = 0; i < 2; i++)
	{
		final_color +=
		    saturate(dot((float3)LightDir[i], pixel_input.normal)) * LightColor[i] * float4(pixel_input.color, 1.0f);
	}
	final_color = saturate(final_color);
	final_color.a = 1;
	return final_color;
}
