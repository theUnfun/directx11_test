
struct InputVertex
{
	float4 pos : POSITION;
	float3 color : COLOR0;
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
};

//--------------------------------------------------------------------------------------
// ���������� ����������� �������
//--------------------------------------------------------------------------------------
cbuffer SceneConstantBuffer : register(b0)  // b0 - ������ ������
{
	matrix View;
	matrix Projection;
}

cbuffer ObjectConstantBuffer : register(b1)  // b0 - ������ ������
{
	matrix World;

}

//--------------------------------------------------------------------------------------
// ��������� ������
//--------------------------------------------------------------------------------------
void VS(InputVertex input_vertex, out VertexOutput vertex_output)
{
	vertex_output.pos = mul(input_vertex.pos, World);  // ������� � ������������ ����
	vertex_output.pos = mul(vertex_output.pos, View);  // ����� � ������������ ����
	vertex_output.pos = mul(vertex_output.pos, Projection);  // � ������������ ������������

	vertex_output.color = input_vertex.color;
}


//--------------------------------------------------------------------------------------
// ���������� ������
//--------------------------------------------------------------------------------------
float4 PS(VertexOutput vertex_output) : SV_Target
{
	return float4(vertex_output.color, 1);
}
