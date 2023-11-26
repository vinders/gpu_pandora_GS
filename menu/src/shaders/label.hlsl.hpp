D3D11_INPUT_ELEMENT_DESC labelLayout[]{
  { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

// ---

char labelVertexShader[] = R"(
cbuffer WorldView : register(b1)
{
  float4 worldOffset;
}

struct VS_INPUT
{
  float2 position : POSITION;
  float2 coords : TEXCOORD;
};
struct PS_INPUT
{
  float4 position : SV_POSITION;
  float2 coords : TEXCOORD;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;
  output.position = float4(input.position.x + worldOffset.x, input.position.y + worldOffset.y, 0.0, 1.0);
  output.coords = input.coords;
  return output;
}
)";

// ---

char labelPixelShader[] = R"(
cbuffer ColorView : register(b0)
{
  float4 textColor;
}

struct PS_INPUT
{
  float4 position : SV_POSITION;
  float2 coords : TEXCOORD;
};
SamplerState TextureSampler : register(s0);
Texture2D Glyph : register(t0);

float4 main(PS_INPUT input) : SV_TARGET
{
  return float4(textColor.r, textColor.g, textColor.b, Glyph.Sample(TextureSampler, input.coords).a * textColor.a);
}
)";
