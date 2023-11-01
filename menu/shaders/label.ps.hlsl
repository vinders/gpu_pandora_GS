// Text color
cbuffer ColorView : register(b0)
{
  float4 textColor;
}

// Input data
struct PS_INPUT
{
  float4 position : SV_POSITION;
  float2 coords : TEXCOORD;
};

SamplerState TextureSampler : register(s0);
Texture2D Glyph : register(t0);

// ---

float4 main(PS_INPUT input) : SV_TARGET
{
  return float4(textColor.r, textColor.g, textColor.b, Glyph.Sample(TextureSampler, input.coords).a);
}
