// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_delegate_impl.h"

#include <cstddef>

#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/psst/brave_psst_permission_context.h"
#include "brave/browser/psst/psst_ui_presenter.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

namespace {

constexpr char kNavigationUrlKey[] = "https://a.test/";

}  // namespace

class MockUiPresenter : public psst::PsstUiPresenter {
 public:
  MockUiPresenter() = default;
  ~MockUiPresenter() override = default;

  MOCK_METHOD(void,
              ShowInfoBar,
              (BravePsstInfoBarDelegate::AcceptCallback on_accept_callback),
              (override));
  MOCK_METHOD(void, ShowIcon, (), (override));
};

class BravePsstUIDelegateImplUnitTest : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(psst::features::kEnablePsst);
    ChromeRenderViewHostTestHarness::SetUp();
    NavigateAndCommit(GURL(kNavigationUrlKey),
                      ui::PageTransition::PAGE_TRANSITION_FIRST);
    auto mock_ui_presenter = std::make_unique<MockUiPresenter>();
    mock_ui_presenter_ = mock_ui_presenter.get();
    psst_ui_delegate_ = std::make_unique<psst::PsstUiDelegateImpl>(
        profile(), web_contents(), std::move(mock_ui_presenter));

    psst_permission_context_ =
        static_cast<psst::PsstUiDelegateImpl*>(psst_ui_delegate_.get())
            ->psst_permission_context_.get();
  }

  void TearDown() override {
    psst_permission_context_ = nullptr;
    mock_ui_presenter_ = nullptr;
    psst_ui_delegate_ = nullptr;
    ChromeRenderViewHostTestHarness::TearDown();
  }
  psst::PsstUiDelegate* psst_ui_delegate() { return psst_ui_delegate_.get(); }

  MockUiPresenter* mock_ui_presenter() { return mock_ui_presenter_; }

  BravePsstPermissionContext* psst_permission_context() {
    return psst_permission_context_.get();
  }

  PrefService* prefs() { return profile()->GetPrefs(); }

 private:
  base::test::ScopedFeatureList feature_list_;
  raw_ptr<MockUiPresenter> mock_ui_presenter_;                   // not owned
  raw_ptr<BravePsstPermissionContext> psst_permission_context_;  // not owned
  std::unique_ptr<psst::PsstUiDelegate> psst_ui_delegate_;
};

TEST_F(BravePsstUIDelegateImplUnitTest,
       HasGrantedPermissionInfobarAndIconShown) {
  prefs()->SetBoolean(psst::prefs::kShowPsstInfoBar, true);
  EXPECT_CALL(*mock_ui_presenter(), ShowInfoBar(testing::_))
      .WillOnce(testing::Invoke(
          [](BravePsstInfoBarDelegate::AcceptCallback callback) {
            std::move(callback).Run(true);  // Simulate user accepts the infobar
          }));
  EXPECT_CALL(*mock_ui_presenter(), ShowIcon).Times(1);

  psst_permission_context()->CreateOrUpdate(
      url::Origin::Create(GURL(kNavigationUrlKey)),
      PsstPermissionInfo{psst::ConsentStatus::kAllow, 1, "test_user",
                         base::Value::List()});

  psst_ui_delegate()->ShowPsstInfobar(
      base::BindOnce([](bool is_accepted) { EXPECT_TRUE(is_accepted); }));
  EXPECT_FALSE(prefs()->GetBoolean(psst::prefs::kShowPsstInfoBar));

  psst_ui_delegate()->Show({"test_user", kNavigationUrlKey, base::Value::List(),
                            1, base::NullCallback()});
}

TEST_F(BravePsstUIDelegateImplUnitTest,
       BlockedPermissionNoInfobarAndIconShown) {
  prefs()->SetBoolean(psst::prefs::kShowPsstInfoBar, true);
  EXPECT_CALL(*mock_ui_presenter(), ShowInfoBar(testing::_))
      .WillOnce(testing::Invoke(
          [](BravePsstInfoBarDelegate::AcceptCallback callback) {
            std::move(callback).Run(true);  // Simulate user accepts the infobar
          }));
  EXPECT_CALL(*mock_ui_presenter(), ShowIcon).Times(0);

  psst_permission_context()->CreateOrUpdate(
      url::Origin::Create(GURL(kNavigationUrlKey)),
      PsstPermissionInfo{psst::ConsentStatus::kBlock, 1, "test_user",
                         base::Value::List()});

  psst_ui_delegate()->ShowPsstInfobar(base::NullCallback());
  EXPECT_FALSE(prefs()->GetBoolean(psst::prefs::kShowPsstInfoBar));

  psst_ui_delegate()->Show({"test_user", kNavigationUrlKey, base::Value::List(),
                            1, base::NullCallback()});
}

TEST_F(BravePsstUIDelegateImplUnitTest, HasNoGrantedPermissionNothingShown) {
  prefs()->SetBoolean(psst::prefs::kShowPsstInfoBar, true);
  EXPECT_CALL(*mock_ui_presenter(), ShowInfoBar).Times(0);
  EXPECT_CALL(*mock_ui_presenter(), ShowIcon).Times(1);

  psst_ui_delegate()->Show({"test_user", kNavigationUrlKey, base::Value::List(),
                            1, base::NullCallback()});
}

TEST_F(BravePsstUIDelegateImplUnitTest, HasGrantedPermissionOnlyIconShown) {
  prefs()->SetBoolean(psst::prefs::kShowPsstInfoBar, false);
  EXPECT_CALL(*mock_ui_presenter(), ShowInfoBar(testing::_)).Times(0);
  EXPECT_CALL(*mock_ui_presenter(), ShowIcon).Times(1);

  psst_permission_context()->CreateOrUpdate(
      url::Origin::Create(GURL(kNavigationUrlKey)),
      PsstPermissionInfo{psst::ConsentStatus::kAllow, 1, "test_user",
                         base::Value::List()});

  psst_ui_delegate()->Show({"test_user", kNavigationUrlKey, base::Value::List(),
                            1, base::NullCallback()});
}

}  // namespace psst
