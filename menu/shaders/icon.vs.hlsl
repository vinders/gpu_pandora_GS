// World position offset
cbuffer WorldView : register(b1)
{
  float4 worldOffset;
}

// Input data
struct VS_INPUT
{
  float2 position : POSITION;
  float2 coords : TEXCOORD;
};
// Output data
struct PS_INPUT
{
  float4 position : SV_POSITION;
  float2 coords : TEXCOORD;
};

// ---

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;
  output.position = float4(input.position.x + worldOffset.x, input.position.y + worldOffset.y, 0.0, 1.0);
  output.coords = input.coords;
  return output;
}
