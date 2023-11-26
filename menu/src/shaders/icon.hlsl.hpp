D3D11_INPUT_ELEMENT_DESC iconLayout[]{
  { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

// ---

char iconVertexShader[] = R"(
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

char iconPixelShader[] = R"(
cbuffer ColorView : register(b0)
{
  float4 colorFilter;
}

struct PS_INPUT
{
  float4 position : SV_POSITION;
  float2 coords : TEXCOORD;
};
SamplerState TextureSampler : register(s0);
Texture2D Sprite : register(t0);

float hueToRgb(float p, float q, float hue) {
  if (hue < 0.0)
    hue += 6.0;
  if (hue > 6.0)
    hue -= 6.0;
  if (hue < 1.0) return p + (q - p) * hue;
  if (hue < 3.0) return q;
  if (hue < 4.0) return p + (q - p) * (4.0 - hue);
  return p;
}

float4 applyColorFilter(float lum, float sourceAlpha) {
  float max, min, hue, sat, alpha;
  if (colorFilter.w > 1.0) {
    lum *= colorFilter.w;
    if (lum > 1.0)
      lum = 1.0;
    alpha = sourceAlpha;
  }
  else alpha = sourceAlpha*colorFilter.w;
  
  if (colorFilter.x > colorFilter.y && colorFilter.x > colorFilter.z) {
    max = colorFilter.x;
    min = (colorFilter.y <= colorFilter.z) ? colorFilter.y : colorFilter.z;
    hue = (colorFilter.y - colorFilter.z)/(max - min);
  }
  else if (colorFilter.y > colorFilter.z) {
    max = colorFilter.y;
    min = (colorFilter.x <= colorFilter.z) ? colorFilter.x : colorFilter.z;
    hue = 2.0 + (colorFilter.z - colorFilter.x)/(max - min);
  }
  else {
    max = colorFilter.z;
    min = (colorFilter.x <= colorFilter.y) ? colorFilter.x : colorFilter.y;
    if (max == min)
      return float4(lum, lum, lum, alpha);
    hue = 4.0 + (colorFilter.x - colorFilter.y)/(max - min);
  }
  sat = (max - min) / (1.0 - abs(max + min - 1.0));
  
  // pixel lum with filter hue/sat
  float q = (lum < 0.5) ? (lum * (1.0 + sat)) : (lum + sat - lum*sat);
  float p = 2.0*lum - q;
  return float4(hueToRgb(p, q, hue + 2.0), hueToRgb(p, q, hue), hueToRgb(p, q, hue - 2.0), alpha);
}

float4 main(PS_INPUT input) : SV_TARGET
{
  float4 pixel = Sprite.Sample(TextureSampler, input.coords);
  if (colorFilter.x == colorFilter.y && colorFilter.x == colorFilter.z) {
    pixel *= colorFilter;
    if (pixel.r > 1.0)
      pixel.r = 1.0;
    if (pixel.g > 1.0)
      pixel.g = 1.0;
    if (pixel.b > 1.0)
      pixel.b = 1.0;
    if (pixel.a > 1.0)
      pixel.a = 1.0;
    return pixel;
  } else {
    if (pixel.x > pixel.y && pixel.x > pixel.z)
      return applyColorFilter((pixel.y <= pixel.z) ? (pixel.x + pixel.y)/2.0 : (pixel.x + pixel.z)/2.0, pixel.w);
    else if (pixel.y > pixel.z)
      return applyColorFilter((pixel.x <= pixel.z) ? (pixel.y + pixel.x)/2.0 : (pixel.y + pixel.z)/2.0, pixel.w);
    else
      return applyColorFilter((pixel.x <= pixel.y) ? (pixel.z + pixel.x)/2.0 : (pixel.z + pixel.y)/2.0, pixel.w);
  }
}
)";
