/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/download/bubble/download_toolbar_ui_controller.h"

#include "base/callback_list.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/download/bubble/download_bubble_update_service.h"
#include "chrome/browser/download/bubble/download_bubble_update_service_factory.h"
#include "chrome/browser/download/download_item_model.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/download/download_display.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/download/public/common/download_item.h"
#include "components/download/public/common/mock_download_item.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

// Minimal fake that lets tests control which download items are visible, so
// HasInsecureDownloads() returns a predictable value without needing real
// network activity.
class FakeDownloadBubbleUpdateService : public DownloadBubbleUpdateService {
 public:
  explicit FakeDownloadBubbleUpdateService(Profile* profile)
      : DownloadBubbleUpdateService(profile) {}

  bool IsInitialized() const override { return true; }

  bool GetAllModelsToDisplay(
      std::vector<DownloadUIModel::DownloadUIModelPtr>& models,
      const webapps::AppId* /*web_app_id*/,
      bool /*force_backfill*/) override {
    models.clear();
    for (download::DownloadItem* item : items_) {
      models.push_back(DownloadItemModel::Wrap(item));
    }
    return true;
  }

  void AddItem(download::DownloadItem* item) { items_.push_back(item); }

 private:
  std::vector<raw_ptr<download::DownloadItem, VectorExperimental>> items_;
};

std::unique_ptr<KeyedService> BuildFakeService(
    content::BrowserContext* context) {
  return std::make_unique<FakeDownloadBubbleUpdateService>(
      Profile::FromBrowserContext(context));
}

}  // namespace

class DownloadToolbarInsecureIconTest : public InProcessBrowserTest {
 public:
  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    // Register the fake service factory before the browser (and its profile)
    // is created, so DownloadBubbleUIController picks it up from the start and
    // no dangling raw_ptr is left to the replaced service.
    subscription_ =
        BrowserContextDependencyManager::GetInstance()
            ->RegisterCreateServicesCallbackForTesting(
                base::BindRepeating([](content::BrowserContext* context) {
                  DownloadBubbleUpdateServiceFactory::GetInstance()
                      ->SetTestingFactory(
                          context, base::BindRepeating(&BuildFakeService));
                }));
  }

  FakeDownloadBubbleUpdateService* fake_service() {
    return static_cast<FakeDownloadBubbleUpdateService*>(
        DownloadBubbleUpdateServiceFactory::GetForProfile(
            browser()->profile()));
  }

  ToolbarButton* download_button() {
    return BrowserView::GetBrowserViewForBrowser(browser())
        ->toolbar_button_provider()
        ->GetDownloadButton();
  }

  DownloadToolbarUIController* controller() {
    return DownloadToolbarUIController::From(browser());
  }

 private:
  base::CallbackListSubscription subscription_;
};

// Verifies that UpdateIcon_BraveImpl sets / clears the toolbar button colour
// override according to the insecure-download status of the current models.
// Covers: no insecure download, WARN status, and BLOCK status.
IN_PROC_BROWSER_TEST_F(DownloadToolbarInsecureIconTest, InsecureDownloadIcon) {
  auto* ctrl = controller();
  ASSERT_NE(nullptr, ctrl) << "controller should be available";
  ctrl->Show();

  auto* btn = download_button();
  ASSERT_NE(nullptr, btn) << "download button should be present after Show()";

  NiceMock<download::MockDownloadItem> item;

  // No insecure downloads — colour override must be absent.
  ctrl->UpdateDownloadIcon(
      {.new_active = DownloadDisplay::IconActive::kActive});
  EXPECT_FALSE(btn->HasIconEnabledColorsOverride()) << "state: no items";
  EXPECT_TRUE(!btn->HasVectorIcons() ||
              &btn->GetVectorIcon() != &vector_icons::kNotSecureWarningOldIcon)
      << "state: no items; HasVectorIcons=" << btn->HasVectorIcons();

  // WARN status: warning colour override and warning vector icon must be set.
  ON_CALL(item, GetInsecureDownloadStatus())
      .WillByDefault(
          Return(download::DownloadItem::InsecureDownloadStatus::WARN));
  fake_service()->AddItem(&item);
  ctrl->UpdateDownloadIcon(
      {.new_state = DownloadDisplay::IconState::kProgress});
  EXPECT_TRUE(btn->HasIconEnabledColorsOverride()) << "state: WARN";
  EXPECT_EQ(&btn->GetVectorIcon(), &vector_icons::kNotSecureWarningOldIcon)
      << "state: WARN";

  // BLOCK status is treated the same way.
  ON_CALL(item, GetInsecureDownloadStatus())
      .WillByDefault(
          Return(download::DownloadItem::InsecureDownloadStatus::BLOCK));
  ctrl->UpdateDownloadIcon(
      {.new_state = DownloadDisplay::IconState::kComplete});
  EXPECT_TRUE(btn->HasIconEnabledColorsOverride()) << "state: BLOCK";
  EXPECT_EQ(&btn->GetVectorIcon(), &vector_icons::kNotSecureWarningOldIcon)
      << "state: BLOCK";
}
