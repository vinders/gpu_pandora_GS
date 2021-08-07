/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2021  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
*******************************************************************************/
#include <gtest/gtest.h>
#include <display/viewport.h>

using namespace display;
using pandora::hardware::DisplayMode;

class ViewportTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};


TEST_F(ViewportTest, windowMode) {
  Viewport empty;
  EXPECT_EQ((uint32_t)0, empty.scaledSourceWidth());
  EXPECT_EQ((uint32_t)0, empty.scaledSourceHeight());
  EXPECT_EQ((uint32_t)640, empty.minWindowWidth());
  EXPECT_EQ((uint32_t)480, empty.minWindowHeight());

  Viewport wnd1(600, false);
  EXPECT_EQ((uint32_t)800, wnd1.scaledSourceWidth());
  EXPECT_EQ((uint32_t)600, wnd1.scaledSourceHeight());
  EXPECT_EQ((uint32_t)640, wnd1.minWindowWidth());
  EXPECT_EQ((uint32_t)480, wnd1.minWindowHeight());

  Viewport wnd2(720, false);
  EXPECT_EQ((uint32_t)960, wnd2.scaledSourceWidth());
  EXPECT_EQ((uint32_t)720, wnd2.scaledSourceHeight());
  EXPECT_EQ((uint32_t)640, wnd2.minWindowWidth());
  EXPECT_EQ((uint32_t)480, wnd2.minWindowHeight());

  Viewport wnd3(600, true);
  EXPECT_EQ((uint32_t)1067, wnd3.scaledSourceWidth());
  EXPECT_EQ((uint32_t)600, wnd3.scaledSourceHeight());
  EXPECT_EQ((uint32_t)853, wnd3.minWindowWidth());
  EXPECT_EQ((uint32_t)480, wnd3.minWindowHeight());

  Viewport wnd4(720, true);
  EXPECT_EQ((uint32_t)1280, wnd4.scaledSourceWidth());
  EXPECT_EQ((uint32_t)720, wnd4.scaledSourceHeight());
  EXPECT_EQ((uint32_t)853, wnd4.minWindowWidth());
  EXPECT_EQ((uint32_t)480, wnd4.minWindowHeight());
}

TEST_F(ViewportTest, fullscreenMode) {
  Viewport full4_3_a(DisplayMode{ 800,600,32,60 }, 0, 0, false);
  EXPECT_EQ((uint32_t)800, full4_3_a.scaledSourceWidth());
  EXPECT_EQ((uint32_t)600, full4_3_a.scaledSourceHeight());
  EXPECT_EQ((uint32_t)640, full4_3_a.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full4_3_a.minWindowHeight());
  Viewport full4_3_b(DisplayMode{ 800,600,32,60 }, 4, 4, false);
  EXPECT_EQ((uint32_t)800, full4_3_b.scaledSourceWidth());
  EXPECT_EQ((uint32_t)600, full4_3_b.scaledSourceHeight());
  EXPECT_EQ((uint32_t)640, full4_3_b.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full4_3_b.minWindowHeight());
  Viewport full4_3_c(DisplayMode{ 800,600,32,60 }, 0, 0, true);
  EXPECT_EQ((uint32_t)800, full4_3_c.scaledSourceWidth());
  EXPECT_EQ((uint32_t)450, full4_3_c.scaledSourceHeight());
  EXPECT_EQ((uint32_t)853, full4_3_c.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full4_3_c.minWindowHeight());
  Viewport full4_3_d(DisplayMode{ 800,600,32,60 }, 8, 0, true);
  EXPECT_EQ((uint32_t)800, full4_3_d.scaledSourceWidth());
  EXPECT_EQ((uint32_t)600, full4_3_d.scaledSourceHeight());
  EXPECT_EQ((uint32_t)853, full4_3_d.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full4_3_d.minWindowHeight());
  Viewport full4_3_e(DisplayMode{ 800,600,32,60 }, 0, 8, true);
  EXPECT_EQ((uint32_t)1067, full4_3_e.scaledSourceWidth());
  EXPECT_EQ((uint32_t)600, full4_3_e.scaledSourceHeight());
  EXPECT_EQ((uint32_t)853, full4_3_e.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full4_3_e.minWindowHeight());
  Viewport full4_3_f(DisplayMode{ 800,600,32,60 }, 8, 8, true);
  EXPECT_EQ((uint32_t)800+(1067-800)/2, full4_3_f.scaledSourceWidth());
  EXPECT_EQ((uint32_t)600, full4_3_f.scaledSourceHeight());
  EXPECT_EQ((uint32_t)853, full4_3_f.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full4_3_f.minWindowHeight());
  Viewport full4_3_g(DisplayMode{ 800,600,32,60 }, 4, 0, true);
  EXPECT_EQ((uint32_t)800, full4_3_g.scaledSourceWidth());
  EXPECT_EQ((uint32_t)450+(600-450)/2, full4_3_g.scaledSourceHeight());
  EXPECT_EQ((uint32_t)853, full4_3_g.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full4_3_g.minWindowHeight());
  Viewport full4_3_h(DisplayMode{ 800,600,32,60 }, 0, 4, true);
  EXPECT_EQ((uint32_t)800+(1067-800)/2, full4_3_h.scaledSourceWidth());
  EXPECT_EQ((uint32_t)450+(600-450)/2, full4_3_h.scaledSourceHeight());
  EXPECT_EQ((uint32_t)853, full4_3_h.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full4_3_h.minWindowHeight());

  Viewport full16_9_a(DisplayMode{ 1280,720,32,60 }, 0, 0, true);
  EXPECT_EQ((uint32_t)1280, full16_9_a.scaledSourceWidth());
  EXPECT_EQ((uint32_t)720, full16_9_a.scaledSourceHeight());
  EXPECT_EQ((uint32_t)853, full16_9_a.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full16_9_a.minWindowHeight());
  Viewport full16_9_b(DisplayMode{ 1280,720,32,60 }, 4, 4, true);
  EXPECT_EQ((uint32_t)1280, full16_9_b.scaledSourceWidth());
  EXPECT_EQ((uint32_t)720, full16_9_b.scaledSourceHeight());
  EXPECT_EQ((uint32_t)853, full16_9_b.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full16_9_b.minWindowHeight());
  Viewport full16_9_c(DisplayMode{ 1280,720,32,60 }, 0, 0, false);
  EXPECT_EQ((uint32_t)960, full16_9_c.scaledSourceWidth());
  EXPECT_EQ((uint32_t)720, full16_9_c.scaledSourceHeight());
  EXPECT_EQ((uint32_t)640, full16_9_c.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full16_9_c.minWindowHeight());
  Viewport full16_9_d(DisplayMode{ 1280,720,32,60 }, 8, 0, false);
  EXPECT_EQ((uint32_t)1280, full16_9_d.scaledSourceWidth());
  EXPECT_EQ((uint32_t)720, full16_9_d.scaledSourceHeight());
  EXPECT_EQ((uint32_t)640, full16_9_d.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full16_9_d.minWindowHeight());
  Viewport full16_9_e(DisplayMode{ 1280,720,32,60 }, 0, 8, false);
  EXPECT_EQ((uint32_t)1280, full16_9_e.scaledSourceWidth());
  EXPECT_EQ((uint32_t)960, full16_9_e.scaledSourceHeight());
  EXPECT_EQ((uint32_t)640, full16_9_e.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full16_9_e.minWindowHeight());
  Viewport full16_9_f(DisplayMode{ 1280,720,32,60 }, 8, 8, false);
  EXPECT_EQ((uint32_t)1280, full16_9_f.scaledSourceWidth());
  EXPECT_EQ((uint32_t)720+(960-720)/2, full16_9_f.scaledSourceHeight());
  EXPECT_EQ((uint32_t)640, full16_9_f.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full16_9_f.minWindowHeight());
  Viewport full16_9_g(DisplayMode{ 1280,720,32,60 }, 4, 0, false);
  EXPECT_EQ((uint32_t)960+(1280-960)/2, full16_9_g.scaledSourceWidth());
  EXPECT_EQ((uint32_t)720, full16_9_g.scaledSourceHeight());
  EXPECT_EQ((uint32_t)640, full16_9_g.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full16_9_g.minWindowHeight());
  Viewport full16_9_h(DisplayMode{ 1280,720,32,60 }, 0, 4, false);
  EXPECT_EQ((uint32_t)960+(1280-960)/2, full16_9_h.scaledSourceWidth());
  EXPECT_EQ((uint32_t)720+(960-720)/2, full16_9_h.scaledSourceHeight());
  EXPECT_EQ((uint32_t)640, full16_9_h.minWindowWidth());
  EXPECT_EQ((uint32_t)480, full16_9_h.minWindowHeight());
}
