// Color multiplier (hover/active/disabled effects)
cbuffer ColorView : register(b0)
{
  float4 colorMultiplier;
}
// World position offset
cbuffer WorldView : register(b1)
{
  float4 worldOffset;
}

// Input data
struct VS_INPUT
{
  float4 position : POSITION;
  float4 color : COLOR;
};
// Output data
struct PS_INPUT
{
  float4 position : SV_POSITION;
  float4 color : COLOR;
};

// ---

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;
  output.position = float4(input.position.x + worldOffset.x, input.position.y + worldOffset.y, 0.0, 1.0);
  output.color = float4(input.color.r * colorMultiplier.r,
                        input.color.g * colorMultiplier.g,
                        input.color.b * colorMultiplier.b,
                        input.color.a * colorMultiplier.a);
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
