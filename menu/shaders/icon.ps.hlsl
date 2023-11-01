// Color multiplier (hover/active/disabled effects)
cbuffer ColorView : register(b0)
{
  float4 colorMultiplier;
}

// Input data
struct PS_INPUT
{
  float4 position : SV_POSITION;
  float2 coords : TEXCOORD;
};

SamplerState TextureSampler : register(s0);
Texture2D Sprite : register(t0);

// ---

float4 main(PS_INPUT input) : SV_TARGET
{
  float4 output = Sprite.Sample(TextureSampler, input.coords) * colorMultiplier;
  if (output.r > 1.0)
    output.r = 1.0;
  if (output.g > 1.0)
    output.g = 1.0;
  if (output.b > 1.0)
    output.b = 1.0;
  if (output.a > 1.0)
    output.a = 1.0;
  return output;
}
