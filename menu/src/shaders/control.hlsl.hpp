D3D11_INPUT_ELEMENT_DESC controlLayout[]{
  { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

// ---

char controlVertexShader[] = R"(
cbuffer ColorView : register(b0)
{
  float4 colorMultiplier;
}
cbuffer WorldView : register(b1)
{
  float4 worldOffset;
}

struct VS_INPUT
{
  float4 position : POSITION;
  float4 color : COLOR;
};
struct PS_INPUT
{
  float4 position : SV_POSITION;
  float4 color : COLOR;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;
  output.position = float4(input.position.x + worldOffset.x, input.position.y + worldOffset.y, 0.0, 1.0);
  output.color = input.color * colorMultiplier;
  if (output.color.r > 1.0)
    output.color.r = 1.0;
  if (output.color.g > 1.0)
    output.color.g = 1.0;
  if (output.color.b > 1.0)
    output.color.b = 1.0;
  if (output.color.a > 1.0)
    output.color.a = 1.0;
  return output;
}
)";

// ---

char controlPixelShader[] = R"(
struct PS_INPUT
{
  float4 projection : SV_POSITION;
  float4 color : COLOR;
};

float4 main(PS_INPUT input) : SV_TARGET
{
  return input.color;
}
)";
