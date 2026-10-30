// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_draw_session.h"

#include "base/functional/bind.h"
#include "base/numerics/safe_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/browser/ui/gestures/session/gesture_config_tracker.h"
#include "brave/components/gestures/core/browser/input/mouse/test/fake_mouse_rocker_wheel_default_commands.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/browser/prefs_registration.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/input/draw_input_source_types.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

class GestureDrawSessionTest : public testing::Test {
 public:
  GestureDrawSessionTest() {
    // Pinned explicitly, since several tests below assert on distances
    // computed by hand against these exact values.
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kGesturesFeature,
        {{"DirectionalGestureMinStrokePixels", "30"},
         {"CustomGestureResamplePointCount", "32"},
         {"CustomGestureMaxMatchDistance", "0.14"}});
    RegisterProfilePrefs(pref_service_.registry(),
                         test::FakeMouseRockerDefaultCommands(),
                         test::FakeMouseWheelDefaultCommands());
    pref_service_.SetBoolean(prefs::kShowTrail, false);
    pref_service_.SetBoolean(prefs::kShowMatchedGesture, false);
    config_tracker_ = std::make_unique<GestureConfigTracker>(pref_service_);
  }

 protected:
  GestureDrawSession MakeDrawSession() {
    // The trail/matched-gesture display prefs are disabled above, so the
    // overlay widget this would back is never actually constructed.
    return {
        base::BindRepeating([]() -> content::WebContents* { return nullptr; }),
        pref_service_,
        config_tracker_->directional_gesture_mapping(),
        config_tracker_->custom_gesture_recognizer(),
        config_tracker_->smart_gesture_mapping(),
        base::BindRepeating(&GestureDrawSessionTest::MovementCallback,
                            base::Unretained(this))};
  }

  void MovementCallback() { ++movement_count_; }

  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  std::unique_ptr<GestureConfigTracker> config_tracker_;
  size_t movement_count_ = 0;
};

TEST_F(GestureDrawSessionTest, ShouldNotBeDrawingBeforeStart) {
  GestureDrawSession session = MakeDrawSession();

  EXPECT_FALSE(session.is_drawing());
}

TEST_F(GestureDrawSessionTest, ShouldMarkDrawingAsActiveWhenStarted) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);

  EXPECT_TRUE(session.is_drawing());
  EXPECT_FALSE(session.is_drawing_via_touch());
  EXPECT_FALSE(session.is_drawing_via_alt_left_button());
}

TEST_F(GestureDrawSessionTest, ShouldRecordTouchAsDrawingInput) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kTouchDrag);

  EXPECT_TRUE(session.is_drawing_via_touch());
  EXPECT_FALSE(session.is_drawing_via_alt_left_button());
}

TEST_F(GestureDrawSessionTest, ShouldRecordAltLeftClickAsDrawingInput) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kAltLeftButtonDrag);

  EXPECT_FALSE(session.is_drawing_via_touch());
  EXPECT_TRUE(session.is_drawing_via_alt_left_button());
}

TEST_F(GestureDrawSessionTest, ShouldReturnNoResultWhenNothingIsBeingDrawn) {
  GestureDrawSession session = MakeDrawSession();

  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  EXPECT_FALSE(test_future.Take());
}

TEST_F(GestureDrawSessionTest, ShouldUseDefaultActionForDownwardStroke) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  session.Continue(gfx::PointF(0, 100));
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  std::optional<GestureActions> actions = test_future.Take();

  EXPECT_FALSE(session.is_drawing());
  EXPECT_THAT(
      actions,
      testing::Optional(testing::ElementsAre(MakeGestureAction(IDC_NEW_TAB))));
}

TEST_F(GestureDrawSessionTest,
       ShouldUseDefaultActionForStraightStrokeWithMultiplePoints) {
  // Exercises a many-point gesture_path, not the degenerate two-point case
  // above, since resampling to a fixed count previously bunched points.
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  for (int x = 10; x <= 100; x += 10) {
    session.Continue(gfx::PointF(base::saturated_cast<float>(-x), 0));
  }
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  std::optional<GestureActions> actions = test_future.Take();

  EXPECT_THAT(actions, testing::Optional(
                           testing::ElementsAre(MakeGestureAction(IDC_BACK))));
}

TEST_F(GestureDrawSessionTest, ShouldReturnNoActionsWhenNothingMatches) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  // Smaller than the pinned 30px sensitivity, so no directional stroke is
  // recognized; no custom gestures are configured either.
  session.Continue(gfx::PointF(0, 5));
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  std::optional<GestureActions> actions = test_future.Take();

  EXPECT_THAT(actions, testing::Optional(testing::IsEmpty()));
}

TEST_F(GestureDrawSessionTest,
       ShouldRecognizeCustomGestureWhenNoDirectionalMappingMatches) {
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

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  session.Continue(gfx::PointF(100, -100));
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  std::optional<GestureActions> actions = test_future.Take();

  EXPECT_THAT(actions, testing::Optional(
                           testing::ElementsAre(MakeGestureAction(IDC_BACK))));
}

TEST_F(GestureDrawSessionTest,
       ShouldRejectDirectionalMatchWhenShapeDoesNotResembleIt) {
  // Commits a single "→" stroke, but the overall drawn shape is nothing
  // like a straight swipe, so it must resolve as unmatched, not `IDC_FORWARD`.
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  for (const gfx::PointF& location :
       {gfx::PointF(30, 0), gfx::PointF(60, 0), gfx::PointF(70, 2),
        gfx::PointF(78, 8), gfx::PointF(82, 18), gfx::PointF(80, 28),
        gfx::PointF(72, 35), gfx::PointF(62, 36), gfx::PointF(54, 30),
        gfx::PointF(52, 20), gfx::PointF(56, 10), gfx::PointF(70, 8),
        gfx::PointF(85, 8), gfx::PointF(100, 8)}) {
    session.Continue(location);
  }
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  std::optional<GestureActions> actions = test_future.Take();

  EXPECT_THAT(actions, testing::Optional(testing::IsEmpty()));
}

TEST_F(GestureDrawSessionTest,
       ShouldPreferStrongerCustomMatchOverWeakDirectionalMatch) {
  // Same weak directional match as the rejection test above, but a custom
  // gesture saved with that exact shape must win instead.
  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "tall-excursion-gesture")
              .Set("points",
                   base::ListValue()
                       .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                       .Append(base::DictValue().Set("x", 30.0).Set("y", 0.0))
                       .Append(base::DictValue().Set("x", 60.0).Set("y", 0.0))
                       .Append(base::DictValue().Set("x", 70.0).Set("y", 2.0))
                       .Append(base::DictValue().Set("x", 78.0).Set("y", 8.0))
                       .Append(base::DictValue().Set("x", 82.0).Set("y", 18.0))
                       .Append(base::DictValue().Set("x", 80.0).Set("y", 28.0))
                       .Append(base::DictValue().Set("x", 72.0).Set("y", 35.0))
                       .Append(base::DictValue().Set("x", 62.0).Set("y", 36.0))
                       .Append(base::DictValue().Set("x", 54.0).Set("y", 30.0))
                       .Append(base::DictValue().Set("x", 52.0).Set("y", 20.0))
                       .Append(base::DictValue().Set("x", 56.0).Set("y", 10.0))
                       .Append(base::DictValue().Set("x", 70.0).Set("y", 8.0))
                       .Append(base::DictValue().Set("x", 85.0).Set("y", 8.0))
                       .Append(base::DictValue().Set("x", 100.0).Set("y", 8.0)))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_BACK)))));
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  for (const gfx::PointF& location :
       {gfx::PointF(30, 0), gfx::PointF(60, 0), gfx::PointF(70, 2),
        gfx::PointF(78, 8), gfx::PointF(82, 18), gfx::PointF(80, 28),
        gfx::PointF(72, 35), gfx::PointF(62, 36), gfx::PointF(54, 30),
        gfx::PointF(52, 20), gfx::PointF(56, 10), gfx::PointF(70, 8),
        gfx::PointF(85, 8), gfx::PointF(100, 8)}) {
    session.Continue(location);
  }
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  std::optional<GestureActions> actions = test_future.Take();

  EXPECT_THAT(actions, testing::Optional(
                           testing::ElementsAre(MakeGestureAction(IDC_BACK))));
}

TEST_F(GestureDrawSessionTest,
       ShouldPreferDirectionalMatchOverLessSimilarCustomMatch) {
  // Closer to the idealized "←" template than to a similarly-shaped custom
  // gesture, so directional must win, not whichever resolves first.
  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "bumped-left-gesture")
              .Set(
                  "points",
                  base::ListValue()
                      .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                      .Append(base::DictValue().Set("x", -50.0).Set("y", 15.0))
                      .Append(base::DictValue().Set("x", -100.0).Set("y", 0.0)))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_FORWARD)))));
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  for (int x = 10; x <= 100; x += 10) {
    session.Continue(gfx::PointF(base::saturated_cast<float>(-x), 0));
  }
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  std::optional<GestureActions> actions = test_future.Take();

  EXPECT_THAT(actions, testing::Optional(
                           testing::ElementsAre(MakeGestureAction(IDC_BACK))));
}

TEST_F(GestureDrawSessionTest,
       ShouldPreferDirectionalMatchWhenDistancesAreEqual) {
  // A custom gesture matching the "←" directional template exactly, so both
  // distances are near-zero and differ only by floating-point noise.
  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "straight-left-gesture")
              .Set(
                  "points",
                  base::ListValue()
                      .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                      .Append(base::DictValue().Set("x", -100.0).Set("y", 0.0)))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_FORWARD)))));
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  for (int x = 10; x <= 100; x += 10) {
    session.Continue(gfx::PointF(base::saturated_cast<float>(-x), 0));
  }
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  std::optional<GestureActions> actions = test_future.Take();

  EXPECT_THAT(actions, testing::Optional(
                           testing::ElementsAre(MakeGestureAction(IDC_BACK))));
}

TEST_F(GestureDrawSessionTest,
       ShouldPreferCustomMatchOverLessSimilarDirectionalMatch) {
  // Drawn shape exactly matches a saved custom gesture (distance zero),
  // closer than the "←" directional candidate, which only approximates it.
  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "bumped-left-gesture")
              .Set(
                  "points",
                  base::ListValue()
                      .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                      .Append(base::DictValue().Set("x", -50.0).Set("y", 15.0))
                      .Append(base::DictValue().Set("x", -100.0).Set("y", 0.0)))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_FORWARD)))));
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  session.Continue(gfx::PointF(-50, 15));
  session.Continue(gfx::PointF(-100, 0));
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  std::optional<GestureActions> actions = test_future.Take();

  EXPECT_THAT(
      actions,
      testing::Optional(testing::ElementsAre(MakeGestureAction(IDC_FORWARD))));
}

TEST_F(GestureDrawSessionTest,
       ShouldInvalidateCustomMatchAfterUnrelatedTrailingMovement) {
  // The user keeps moving past an early exact match into territory that
  // resolves to nothing, so a stale earlier-in-the-draw match never fires.
  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "bumped-left-gesture")
              .Set(
                  "points",
                  base::ListValue()
                      .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                      .Append(base::DictValue().Set("x", -50.0).Set("y", 15.0))
                      .Append(base::DictValue().Set("x", -100.0).Set("y", 0.0)))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_FORWARD)))));
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  session.Continue(gfx::PointF(-50, 15));
  session.Continue(gfx::PointF(-100, 0));
  session.Continue(gfx::PointF(-100, 200));
  session.Continue(gfx::PointF(100, 200));
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  std::optional<GestureActions> actions = test_future.Take();

  EXPECT_THAT(actions, testing::Optional(testing::IsEmpty()));
}

TEST_F(GestureDrawSessionTest,
       ShouldSwitchToLongerCustomMatchAsDrawingContinues) {
  // The first bump matches "bumped-left-gesture" partway through, but a
  // second bump completes "double-bumped-left-gesture" by the end.
  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue()
          .Append(
              base::DictValue()
                  .Set("id", "bumped-left-gesture")
                  .Set(
                      "points",
                      base::ListValue()
                          .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                          .Append(
                              base::DictValue().Set("x", -50.0).Set("y", 15.0))
                          .Append(
                              base::DictValue().Set("x", -100.0).Set("y", 0.0)))
                  .Set("actions",
                       base::ListValue().Append(
                           base::DictValue().Set("command", IDC_FORWARD))))
          .Append(
              base::DictValue()
                  .Set("id", "double-bumped-left-gesture")
                  .Set(
                      "points",
                      base::ListValue()
                          .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                          .Append(
                              base::DictValue().Set("x", -50.0).Set("y", 15.0))
                          .Append(
                              base::DictValue().Set("x", -100.0).Set("y", 0.0))
                          .Append(
                              base::DictValue().Set("x", -150.0).Set("y", 15.0))
                          .Append(
                              base::DictValue().Set("x", -200.0).Set("y", 0.0)))
                  .Set("actions",
                       base::ListValue().Append(
                           base::DictValue().Set("command", IDC_BACK)))));
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  session.Continue(gfx::PointF(-50, 15));
  session.Continue(gfx::PointF(-100, 0));
  session.Continue(gfx::PointF(-150, 15));
  session.Continue(gfx::PointF(-200, 0));
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  std::optional<GestureActions> actions = test_future.Take();

  EXPECT_THAT(actions, testing::Optional(
                           testing::ElementsAre(MakeGestureAction(IDC_BACK))));
}

TEST_F(GestureDrawSessionTest, ShouldNotifyMovementWhileDrawing) {
  GestureDrawSession session = MakeDrawSession();

  session.Continue(gfx::PointF(0, 100));
  EXPECT_EQ(0U, movement_count_);

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  session.Continue(gfx::PointF(0, 100));
  EXPECT_EQ(1U, movement_count_);

  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
  session.Continue(gfx::PointF(0, 200));
  EXPECT_EQ(1U, movement_count_);
}

TEST_F(GestureDrawSessionTest, ShouldDiscardUnfinishedPathWhenCancelled) {
  GestureDrawSession session = MakeDrawSession();

  session.Start(gfx::PointF(0, 0), DrawInputSourceType::kRightButtonDrag);
  session.Continue(gfx::PointF(0, 100));
  session.Cancel();

  EXPECT_FALSE(session.is_drawing());
  base::test::TestFuture<std::optional<GestureActions>> test_future;
  session.Finish(test_future.GetCallback());
  EXPECT_FALSE(test_future.Take());
}

}  // namespace gestures
