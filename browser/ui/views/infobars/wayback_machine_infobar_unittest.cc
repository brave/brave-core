// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/infobars/wayback_machine_infobar.h"

#include <string>

#include "base/i18n/message_formatter.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/browser/infobars/wayback_machine_infobar_delegate.h"
#include "brave/components/brave_wayback_machine/wayback_snapshot_info.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/types/event_type.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/button_test_api.h"
#include "url/gurl.h"

namespace {

constexpr char kSnapshotURL[] =
    "https://web.archive.org/web/20240226233000/https://example.com/";

}  // namespace

class WaybackMachineInfoBarTest : public ChromeRenderViewHostTestHarness {
 public:
  WaybackMachineInfoBarTest()
      : ChromeRenderViewHostTestHarness(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    infobars::ContentInfoBarManager::CreateForWebContents(web_contents());
  }

  infobars::ContentInfoBarManager* infobar_manager() {
    return infobars::ContentInfoBarManager::FromWebContents(web_contents());
  }

  WaybackMachineInfoBar* GetInfoBar() {
    if (infobar_manager()->infobars().empty()) {
      return nullptr;
    }
    return static_cast<WaybackMachineInfoBar*>(
        infobar_manager()->infobars()[0]);
  }

  WaybackMachineInfoBarDelegate* GetDelegate() {
    auto* infobar = GetInfoBar();
    return infobar ? static_cast<WaybackMachineInfoBarDelegate*>(
                         infobar->delegate())
                   : nullptr;
  }

  void ClickButton(views::Button* button) {
    views::test::ButtonTestApi(button).NotifyClick(
        ui::MouseEvent(ui::EventType::kMousePressed, gfx::Point(), gfx::Point(),
                       ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  }

  std::u16string ExpectedRedirectButtonText(int seconds) {
    return base::i18n::MessageFormatter::FormatWithNamedArgs(
        l10n_util::GetStringUTF16(
            IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_REDIRECT_BUTTON),
        "SECONDS", seconds);
  }

 private:
  gfx::AnimationTestApi::RenderModeResetter disable_rich_animations_ =
      gfx::AnimationTestApi::SetRichAnimationRenderMode(
          gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);
  ChromeLayoutProvider layout_provider_;
};

TEST_F(WaybackMachineInfoBarTest, CreateAddsSingleNonCloseableInfoBar) {
  const WaybackSnapshotInfo snapshot{.url = GURL(kSnapshotURL)};
  WaybackMachineInfoBarDelegate::Create(web_contents(), snapshot);
  ASSERT_EQ(infobar_manager()->infobars().size(), 1u);

  WaybackMachineInfoBarDelegate::Create(web_contents(), snapshot);
  EXPECT_EQ(infobar_manager()->infobars().size(), 1u);

  auto* infobar = GetInfoBar();
  ASSERT_TRUE(infobar);
  EXPECT_FALSE(infobar->delegate()->IsCloseable());
  EXPECT_FALSE(infobar->close_button_for_testing());
  EXPECT_EQ(GetDelegate()->snapshot_info(), snapshot);
}

TEST_F(WaybackMachineInfoBarTest, RemoveRemovesInfoBarAsynchronously) {
  WaybackMachineInfoBarDelegate::Create(
      web_contents(), WaybackSnapshotInfo{.url = GURL(kSnapshotURL)});
  ASSERT_EQ(infobar_manager()->infobars().size(), 1u);

  WaybackMachineInfoBarDelegate::Remove(web_contents());
  EXPECT_EQ(infobar_manager()->infobars().size(), 1u);

  EXPECT_TRUE(base::test::RunUntil(
      [&] { return infobar_manager()->infobars().empty(); }));
}

TEST_F(WaybackMachineInfoBarTest, DescriptionFormatsUTCDateAndAge) {
  base::Time snapshot_time;
  ASSERT_TRUE(
      base::Time::FromUTCString("2024-02-26 23:30:00 UTC", &snapshot_time));
  const base::Time target_now = snapshot_time + base::Days(400);
  if (target_now > base::Time::Now()) {
    task_environment()->AdvanceClock(target_now - base::Time::Now());
  }

  WaybackMachineInfoBarDelegate::Create(
      web_contents(),
      WaybackSnapshotInfo{.url = GURL(kSnapshotURL), .time = snapshot_time});
  auto* infobar = GetInfoBar();
  ASSERT_TRUE(infobar);
  ASSERT_TRUE(infobar->description_label_for_testing());

  const std::u16string description =
      infobar->description_label_for_testing()->GetText();
  EXPECT_NE(description.find(u"2024"), std::u16string::npos);
  EXPECT_NE(description.find(u"26"), std::u16string::npos);
  EXPECT_NE(description,
            l10n_util::GetStringUTF16(
                IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_DESCRIPTION_NO_DATE));
}

TEST_F(WaybackMachineInfoBarTest, DescriptionWithoutTimestampUsesFallback) {
  WaybackMachineInfoBarDelegate::Create(
      web_contents(), WaybackSnapshotInfo{.url = GURL(kSnapshotURL)});
  auto* infobar = GetInfoBar();
  ASSERT_TRUE(infobar);
  ASSERT_TRUE(infobar->description_label_for_testing());

  EXPECT_EQ(infobar->description_label_for_testing()->GetText(),
            l10n_util::GetStringUTF16(
                IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_DESCRIPTION_NO_DATE));
}

TEST_F(WaybackMachineInfoBarTest, CountdownUpdatesButtonAndRemovesInfoBar) {
  WaybackMachineInfoBarDelegate::Create(
      web_contents(), WaybackSnapshotInfo{.url = GURL(kSnapshotURL)});
  auto* infobar = GetInfoBar();
  ASSERT_TRUE(infobar);
  auto* redirect_button = infobar->redirect_button_for_testing();
  ASSERT_TRUE(redirect_button);

  for (int remaining = WaybackMachineInfoBarDelegate::kCountdownSeconds;
       remaining > 1; --remaining) {
    EXPECT_EQ(GetDelegate()->seconds_remaining(), remaining);
    EXPECT_EQ(redirect_button->GetText(),
              ExpectedRedirectButtonText(remaining));
    task_environment()->FastForwardBy(base::Seconds(1));
  }

  EXPECT_EQ(GetDelegate()->seconds_remaining(), 1);
  EXPECT_EQ(redirect_button->GetText(), ExpectedRedirectButtonText(1));

  task_environment()->FastForwardBy(base::Seconds(1));
  EXPECT_TRUE(infobar_manager()->infobars().empty());
}

TEST_F(WaybackMachineInfoBarTest, CancelButtonRemovesInfoBar) {
  WaybackMachineInfoBarDelegate::Create(
      web_contents(), WaybackSnapshotInfo{.url = GURL(kSnapshotURL)});
  auto* infobar = GetInfoBar();
  ASSERT_TRUE(infobar);
  ASSERT_TRUE(infobar->cancel_button_for_testing());

  ClickButton(infobar->cancel_button_for_testing());
  EXPECT_TRUE(infobar_manager()->infobars().empty());
}

TEST_F(WaybackMachineInfoBarTest, RedirectButtonRemovesInfoBar) {
  WaybackMachineInfoBarDelegate::Create(
      web_contents(), WaybackSnapshotInfo{.url = GURL(kSnapshotURL)});
  auto* infobar = GetInfoBar();
  ASSERT_TRUE(infobar);
  ASSERT_TRUE(infobar->redirect_button_for_testing());

  ClickButton(infobar->redirect_button_for_testing());
  EXPECT_TRUE(infobar_manager()->infobars().empty());
}
