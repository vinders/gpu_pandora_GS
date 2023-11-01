// Input data
struct PS_INPUT
{
  float4 projection : SV_POSITION;
  float4 color : COLOR;
};

// ---

float4 main(PS_INPUT input) : SV_TARGET
{
  return input.color;
}
