/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/psst_infobar.h"

#include <optional>
#include <string>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/psst/psst_infobar_delegate.h"
#include "brave/components/psst/core/common/constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_manager.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/font_list.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/test/views_test_utils.h"
#include "url/gurl.h"

namespace psst {

namespace {

constexpr int kTestInfoBarWidth = 600;

// Minimal InfoBarManager so PsstInfoBarDelegate::Create() can produce a real
// PsstInfoBar without needing a content::WebContents.
class FakeInfoBarManager : public infobars::InfoBarManager {
 public:
  FakeInfoBarManager() = default;
  ~FakeInfoBarManager() override { RemoveAllInfoBars(/*animate=*/false); }

  // infobars::InfoBarManager:
  int GetActiveEntryID() override { return 0; }
  void OpenURL(const GURL& url,
               WindowOpenDisposition disposition,
               const std::string& text_fragment) override {
    opened_url_ = url;
  }

  const std::optional<GURL>& opened_url() const { return opened_url_; }

 private:
  std::optional<GURL> opened_url_;
};

}  // namespace

class PsstInfoBarTest : public ChromeViewsTestBase {
 public:
  PsstInfoBar* CreateInfoBar(FakeInfoBarManager* manager) {
    PsstInfoBarDelegate::Create(manager, base::DoNothing());
    CHECK_EQ(1u, manager->infobars().size());
    return static_cast<PsstInfoBar*>(manager->infobars()[0].get());
  }

  // Lay the infobar out at the standard test width so children acquire real
  // bounds we can inspect.
  void LayOut(PsstInfoBar* infobar) {
    infobar->SetBounds(0, 0, kTestInfoBarWidth, infobar->target_height());
    views::test::RunScheduledLayout(infobar);
  }
};

TEST_F(PsstInfoBarTest, ShowsBoldTitleWithBrandedIconBadge) {
  FakeInfoBarManager manager;
  auto* infobar = CreateInfoBar(&manager);

  ASSERT_TRUE(infobar->icon_for_testing());
  // The icon is grown into a branded badge background (see PsstInfoBar).
  EXPECT_TRUE(infobar->icon_for_testing()->GetBackground());

  ASSERT_TRUE(infobar->label_for_testing());
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_BRAVE_PSST_INFOBAR_MESSAGE),
            infobar->label_for_testing()->GetText());
  EXPECT_EQ(gfx::Font::Weight::SEMIBOLD,
            infobar->label_for_testing()->font_list().GetFontWeight());
}

TEST_F(PsstInfoBarTest, HidesBaseLinkInFavorOfInlineSubtitleLink) {
  FakeInfoBarManager manager;
  auto* infobar = CreateInfoBar(&manager);

  // The base BraveConfirmInfoBar link is unused; "Learn more" is inline in
  // the subtitle instead.
  ASSERT_TRUE(infobar->link_for_testing());
  EXPECT_FALSE(infobar->link_for_testing()->GetVisible());

  ASSERT_TRUE(infobar->subtitle_for_testing());
  const std::u16string expected_text =
      l10n_util::GetStringUTF16(IDS_BRAVE_PSST_INFOBAR_SUBTITLE) + u" " +
      l10n_util::GetStringUTF16(IDS_BRAVE_PSST_INFOBAR_LEARN_MORE);
  EXPECT_EQ(expected_text, infobar->subtitle_for_testing()->GetText());
}

TEST_F(PsstInfoBarTest, ClickingLearnMoreOpensPsstReportDialogUrl) {
  FakeInfoBarManager manager;
  auto* infobar = CreateInfoBar(&manager);
  // Link child views are only materialized once the StyledLabel has laid
  // out; ClickFirstLinkForTesting() needs one to exist.
  LayOut(infobar);

  EXPECT_FALSE(manager.opened_url().has_value());

  ASSERT_TRUE(infobar->subtitle_for_testing());
  infobar->subtitle_for_testing()->ClickFirstLinkForTesting();

  ASSERT_TRUE(manager.opened_url().has_value());
  EXPECT_EQ(GURL(kPsstReportDialogLearnMoreUrl), *manager.opened_url());
}

TEST_F(PsstInfoBarTest, GrowsTargetHeightToFitSubtitleRow) {
  FakeInfoBarManager manager;
  auto* infobar = CreateInfoBar(&manager);

  EXPECT_GT(
      infobar->target_height(),
      ChromeLayoutProvider::Get()->GetDistanceMetric(DISTANCE_INFOBAR_HEIGHT));
}

TEST_F(PsstInfoBarTest, LayoutStacksSubtitleBelowTitle) {
  FakeInfoBarManager manager;
  auto* infobar = CreateInfoBar(&manager);
  LayOut(infobar);

  auto* label = infobar->label_for_testing();
  auto* subtitle = infobar->subtitle_for_testing();
  ASSERT_TRUE(label);
  ASSERT_TRUE(subtitle);
  EXPECT_LE(label->bounds().bottom(), subtitle->y());
  // Both share the title's x position (left-aligned title+subtitle block).
  EXPECT_EQ(label->x(), subtitle->x());
}

}  // namespace psst
