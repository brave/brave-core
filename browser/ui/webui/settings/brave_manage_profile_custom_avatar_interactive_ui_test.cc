/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ref_counted_memory.h"
#include "base/test/run_until.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/file_system_chooser_test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "url/gurl.h"

namespace {

DEFINE_LOCAL_ELEMENT_IDENTIFIER_VALUE(kSettingsTab);

constexpr char kDeepQueryUploadAndClick[] = R"JS(
(() => {
  function deepQuerySelector(root, selector) {
    const direct = root.querySelector(selector);
    if (direct) {
      return direct;
    }
    const all = root.querySelectorAll('*');
    for (const el of all) {
      if (el.shadowRoot) {
        const found = deepQuerySelector(el.shadowRoot, selector);
        if (found) {
          return found;
        }
      }
    }
    return null;
  }
  const upload = deepQuerySelector(document, '#braveCustomAvatarUploadBtn');
  if (!upload) {
    return false;
  }
  upload.click();
  return true;
})()
)JS";

constexpr char kDeepQueryRemoveVisible[] = R"JS(
(() => {
  function deepQuerySelector(root, selector) {
    const direct = root.querySelector(selector);
    if (direct) {
      return direct;
    }
    const all = root.querySelectorAll('*');
    for (const el of all) {
      if (el.shadowRoot) {
        const found = deepQuerySelector(el.shadowRoot, selector);
        if (found) {
          return found;
        }
      }
    }
    return null;
  }
  const remove = deepQuerySelector(document, '#braveCustomAvatarRemoveBtn');
  return !!(remove && !remove.hidden);
})()
)JS";

constexpr char kDeepQueryClickRemove[] = R"JS(
(() => {
  function deepQuerySelector(root, selector) {
    const direct = root.querySelector(selector);
    if (direct) {
      return direct;
    }
    const all = root.querySelectorAll('*');
    for (const el of all) {
      if (el.shadowRoot) {
        const found = deepQuerySelector(el.shadowRoot, selector);
        if (found) {
          return found;
        }
      }
    }
    return null;
  }
  const remove = deepQuerySelector(document, '#braveCustomAvatarRemoveBtn');
  if (!remove || remove.hidden) {
    return false;
  }
  remove.click();
  return true;
})()
)JS";

constexpr char kDeepQueryRemoveHidden[] = R"JS(
(() => {
  function deepQuerySelector(root, selector) {
    const direct = root.querySelector(selector);
    if (direct) {
      return direct;
    }
    const all = root.querySelectorAll('*');
    for (const el of all) {
      if (el.shadowRoot) {
        const found = deepQuerySelector(el.shadowRoot, selector);
        if (found) {
          return found;
        }
      }
    }
    return null;
  }
  const remove = deepQuerySelector(document, '#braveCustomAvatarRemoveBtn');
  return !remove || remove.hidden;
})()
)JS";

ProfileAttributesEntry* GetActiveProfileEntry(Browser* browser) {
  Profile* profile = browser->profile();
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  if (!profile_manager) {
    return nullptr;
  }
  return profile_manager->GetProfileAttributesStorage()
      .GetProfileAttributesWithPath(profile->GetPath());
}

}  // namespace

class BraveManageProfileCustomAvatarInteractiveTest
    : public InteractiveBrowserTest {
 public:
  BraveManageProfileCustomAvatarInteractiveTest() = default;

  void SetUpOnMainThread() override {
    InteractiveBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    test_png_path_ = temp_dir_.GetPath().AppendASCII("test_avatar.png");
    gfx::Image image(gfx::test::CreateImage(64, 64));
    scoped_refptr<base::RefCountedMemory> png = image.As1xPNGBytes();
    ASSERT_TRUE(png && png->size() > 0u);
    ASSERT_EQ(static_cast<int>(png->size()),
              base::WriteFile(test_png_path_, *png));
  }

  void TearDownOnMainThread() override {
    ui::SelectFileDialog::SetFactory(nullptr);
    InteractiveBrowserTest::TearDownOnMainThread();
  }

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath test_png_path_;
};

// ARCH-062: critical user journey for custom profile avatar upload and removal
// on the manage-profile settings page.
IN_PROC_BROWSER_TEST_F(BraveManageProfileCustomAvatarInteractiveTest,
                       UploadCustomAvatarThenRemove) {
  ui::SelectFileDialog::SetFactory(
      std::make_unique<content::FakeSelectFileDialogFactory>(
          std::vector<base::FilePath>{test_png_path_}));

  const GURL kManageProfileUrl("chrome://settings/getStarted/manageProfile");

  RunTestSequence(
      InstrumentTab(kSettingsTab, 0, browser()),
      NavigateWebContents(kSettingsTab, kManageProfileUrl),
      WaitForWebContentsPainted(kSettingsTab), Do([this]() {
        content::WebContents* wc =
            browser()->tab_strip_model()->GetActiveWebContents();
        ASSERT_TRUE(base::test::RunUntil([wc]() {
          return content::EvalJs(wc, kDeepQueryUploadAndClick).ExtractBool();
        }));
      }),
      Do([this]() {
        content::WebContents* wc =
            browser()->tab_strip_model()->GetActiveWebContents();
        ASSERT_TRUE(base::test::RunUntil([wc]() {
          return content::EvalJs(wc, kDeepQueryRemoveVisible).ExtractBool();
        }));
      }),
      Do([this]() {
        ProfileAttributesEntry* entry = GetActiveProfileEntry(browser());
        ASSERT_TRUE(entry);
        EXPECT_TRUE(entry->HasBraveCustomAvatar());
        EXPECT_TRUE(entry->IsUsingBraveCustomAvatar());
      }),
      Do([this]() {
        content::WebContents* wc =
            browser()->tab_strip_model()->GetActiveWebContents();
        ASSERT_TRUE(base::test::RunUntil([wc]() {
          return content::EvalJs(wc, kDeepQueryClickRemove).ExtractBool();
        }));
      }),
      Do([this]() {
        content::WebContents* wc =
            browser()->tab_strip_model()->GetActiveWebContents();
        ASSERT_TRUE(base::test::RunUntil([wc]() {
          return content::EvalJs(wc, kDeepQueryRemoveHidden).ExtractBool();
        }));
      }),
      Do([this]() {
        ASSERT_TRUE(base::test::RunUntil([this]() {
          ProfileAttributesEntry* entry = GetActiveProfileEntry(browser());
          return entry && !entry->HasBraveCustomAvatar() &&
                 !entry->IsUsingBraveCustomAvatar();
        }));
      }));
}
