// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_draw_session.h"

#include "base/functional/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/browser/ui/gestures/gesture_config_tracker.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/browser/prefs_registration.h"
#include "brave/components/gestures/core/browser/test/fake_mouse_rocker_wheel_default_commands.h"
#include "brave/components/gestures/core/common/features.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

// `GestureDrawSession` only constructs an overlay widget (which needs a real
// `WebContents`) when the trail/matched-gesture display prefs are on; both
// are disabled below so this callback is never actually invoked.
content::WebContents* NoWebContents() {
  return nullptr;
}

}  // namespace

class GestureDrawSessionTest : public testing::Test {
 public:
  GestureDrawSessionTest() {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kGesturesFeature, {{"MinStrokePixels", "30"}});
    RegisterProfilePrefs(pref_service_.registry(),
                         test::FakeMouseRockerDefaultCommands(),
                         test::FakeMouseWheelDefaultCommands());
    pref_service_.SetBoolean(prefs::kShowTrail, false);
    pref_service_.SetBoolean(prefs::kShowMatchedGesture, false);
    config_tracker_ = std::make_unique<GestureConfigTracker>(pref_service_);
  }

 protected:
  GestureDrawSession MakeDrawSession() {
    return {base::BindRepeating(&NoWebContents), pref_service_,
            *config_tracker_,
            base::BindRepeating(&GestureDrawSessionTest::OnMovement,
                                base::Unretained(this))};
  }

  void OnMovement() { ++movement_count_; }

  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  std::unique_ptr<GestureConfigTracker> config_tracker_;
  size_t movement_count_ = 0;
};

TEST_F(GestureDrawSessionTest, IsDrawingIsFalseBeforeAnyDrawStarts) {
  GestureDrawSession session = MakeDrawSession();

  EXPECT_FALSE(session.is_drawing());
}

TEST_F(GestureDrawSessionTest, StartMarksTheSessionAsDrawing) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), /*via_touch=*/false,
                /*via_alt_left_button=*/false);

  EXPECT_THAT(
      std::make_tuple(session.is_drawing(), session.is_drawing_via_touch(),
                      session.is_drawing_via_alt_left_button()),
      testing::FieldsAre(/*is_drawing=*/true,
                         /*is_drawing_via_touch=*/false,
                         /*is_drawing_via_alt_left_button=*/false));
}

TEST_F(GestureDrawSessionTest, StartViaTouchSetsIsDrawingViaTouch) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), /*via_touch=*/true,
                /*via_alt_left_button=*/false);

  EXPECT_THAT(std::make_tuple(session.is_drawing_via_touch(),
                              session.is_drawing_via_alt_left_button()),
              testing::FieldsAre(/*is_drawing_via_touch=*/true,
                                 /*is_drawing_via_alt_left_button=*/false));
}

TEST_F(GestureDrawSessionTest, StartViaAltLeftSetsIsDrawingViaAltLeft) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), /*via_touch=*/false,
                /*via_alt_left_button=*/true);

  EXPECT_THAT(std::make_tuple(session.is_drawing_via_touch(),
                              session.is_drawing_via_alt_left_button()),
              testing::FieldsAre(/*is_drawing_via_touch=*/false,
                                 /*is_drawing_via_alt_left_button=*/true));
}

TEST_F(GestureDrawSessionTest,
       MaybeFinishReturnsNulloptWhenNoDrawIsInProgress) {
  GestureDrawSession session = MakeDrawSession();

  EXPECT_FALSE(session.MaybeFinish().has_value());
}

TEST_F(GestureDrawSessionTest,
       MaybeFinishResolvesDownwardStrokeToItsDefaultMapping) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), /*via_touch=*/false,
                /*via_alt_left_button=*/false);
  session.Continue(gfx::PointF(0, 100));
  const std::optional<std::vector<GestureActionInfo>> actions =
      session.MaybeFinish();

  EXPECT_FALSE(session.is_drawing());
  ASSERT_TRUE(actions.has_value());
  EXPECT_THAT(*actions, testing::ElementsAre(testing::Field(
                            &GestureActionInfo::command, IDC_NEW_TAB)));
}

TEST_F(GestureDrawSessionTest,
       MaybeFinishReturnsEmptyActionsWhenNothingMatches) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), /*via_touch=*/false,
                /*via_alt_left_button=*/false);
  // Smaller than the pinned 30px sensitivity, so no directional stroke is
  // recognized; no custom gestures are configured either.
  session.Continue(gfx::PointF(0, 5));
  const std::optional<std::vector<GestureActionInfo>> actions =
      session.MaybeFinish();

  ASSERT_TRUE(actions.has_value());
  EXPECT_TRUE(actions->empty());
}

TEST_F(GestureDrawSessionTest,
       MaybeFinishResolvesCustomGestureWhenNoDirectionalMappingMatches) {
  // "↗" has no default directional mapping, so classifying to it falls
  // through to custom gesture matching instead.
  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "diagonal-gesture")
              .Set("points",
                   base::ListValue()
                       .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                       .Append(
                           base::DictValue().Set("x", 100.0).Set("y", -100.0)))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_BACK)))));
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), /*via_touch=*/false,
                /*via_alt_left_button=*/false);
  session.Continue(gfx::PointF(100, -100));
  const std::optional<std::vector<GestureActionInfo>> actions =
      session.MaybeFinish();

  ASSERT_TRUE(actions.has_value());
  EXPECT_THAT(*actions, testing::ElementsAre(testing::Field(
                            &GestureActionInfo::command, IDC_BACK)));
}

TEST_F(GestureDrawSessionTest, ContinueRunsTheMovementCallbackWhileDrawing) {
  GestureDrawSession session = MakeDrawSession();

  session.Continue(gfx::PointF(0, 100));
  EXPECT_EQ(0U, movement_count_);

  session.Start(gfx::PointF(0, 0), /*via_touch=*/false,
                /*via_alt_left_button=*/false);
  session.Continue(gfx::PointF(0, 100));
  EXPECT_EQ(1U, movement_count_);

  session.MaybeFinish();
  session.Continue(gfx::PointF(0, 200));
  EXPECT_EQ(1U, movement_count_);
}

TEST_F(GestureDrawSessionTest, CancelStopsDrawingAndDiscardsTheInProgressPath) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), /*via_touch=*/false,
                /*via_alt_left_button=*/false);
  session.Continue(gfx::PointF(0, 100));
  session.Cancel();

  EXPECT_FALSE(session.is_drawing());
  EXPECT_FALSE(session.MaybeFinish().has_value());
}

}  // namespace gestures
