/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_shields/common/features.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/test/render_view_test.h"
#include "third_party/blink/public/common/features.h"

using blink::features::kBraveRoundTimeStamps;

class BraveTimeStampRoundingRenderViewTest : public content::RenderViewTest {
 public:
  double ExecuteJSAndReturnDouble(const std::u16string& script) {
    double result;
    EXPECT_TRUE(ExecuteJavaScriptAndReturnNumberValue(script, &result));
    return result;
  }

  void CheckRounded(const std::u16string& script, bool expect_rounded) {
    double result = ExecuteJSAndReturnDouble(script);
    if (expect_rounded) {
      EXPECT_EQ(round(result) - result, 0);
    } else {
      EXPECT_NE(round(result) - result, 0);
    }
    std::cout << script << ": " << result << std::endl;
  }

  void Advance100Microseconds() {
    task_environment_.AdvanceClock(base::Milliseconds(0.1));
    task_environment_.RunUntilIdle();
  }

  void RunRoundingTests(bool expect_rounded) {
    LoadHTML("<html><body>hi</body></html>");
    Advance100Microseconds();
    CheckRounded(u"performance.now()", expect_rounded);
    Advance100Microseconds();
    CheckRounded(u"performance.mark('test').startTime", expect_rounded);
    Advance100Microseconds();
    if (expect_rounded) {
      CheckRounded(u"performance.timeOrigin", true);
    }
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
};

class BraveTimeStampRoundingRenderViewTest_Enable
    : public BraveTimeStampRoundingRenderViewTest {
 public:
  BraveTimeStampRoundingRenderViewTest_Enable() {
    feature_list_.InitAndEnableFeature(kBraveRoundTimeStamps);
  }
  ~BraveTimeStampRoundingRenderViewTest_Enable() override = default;
};

class BraveTimeStampRoundingRenderViewTest_Disable
    : public BraveTimeStampRoundingRenderViewTest {
 public:
  BraveTimeStampRoundingRenderViewTest_Disable() {
    feature_list_.InitAndDisableFeature(kBraveRoundTimeStamps);
  }
  ~BraveTimeStampRoundingRenderViewTest_Disable() override = default;
};

TEST_F(BraveTimeStampRoundingRenderViewTest_Enable, SynchronousApisRounded) {
  RunRoundingTests(true);
}

TEST_F(BraveTimeStampRoundingRenderViewTest_Disable,
       SynchronousApisRounded_Disable) {
  RunRoundingTests(false);
}

