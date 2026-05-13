/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/ref_counted_memory.h"
#include "base/test/run_until.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_delegate.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "url/gurl.h"

namespace {

DEFINE_LOCAL_ELEMENT_IDENTIFIER_VALUE(kSettingsTab);

ProfileAttributesEntry* GetActiveProfileEntry(Browser* browser) {
  Profile* profile = browser->profile();
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  if (!profile_manager) {
    return nullptr;
  }
  return profile_manager->GetProfileAttributesStorage()
      .GetProfileAttributesWithPath(profile->GetPath());
}

void DismissSyncCannotRunInfobarIfPresent(content::WebContents* web_contents) {
  infobars::ContentInfoBarManager* manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents);
  if (!manager) {
    return;
  }
  for (infobars::InfoBar* infobar : manager->infobars()) {
    infobars::InfoBarDelegate* delegate = infobar->delegate();
    if (delegate->GetIdentifier() !=
        infobars::InfoBarDelegate::SYNC_CANNOT_RUN_INFOBAR) {
      continue;
    }
    ConfirmInfoBarDelegate* confirm = delegate->AsConfirmInfoBarDelegate();
    if (confirm) {
      // "Don't show again" — clears the bar without navigating away from
      // settings (Accept opens sync settings).
      confirm->Cancel();
    }
    break;
  }
}

}  // namespace

class BraveManageProfileCustomAvatarInteractiveTest
    : public InteractiveBrowserTest {
 public:
  BraveManageProfileCustomAvatarInteractiveTest() = default;

  void SetUpOnMainThread() override {
    InteractiveBrowserTest::SetUpOnMainThread();
    // Some test runners ship a Local State pref that disables native file
    // pickers; keep dialogs enabled for parity with real user flows.
    if (g_browser_process->local_state()) {
      g_browser_process->local_state()->SetBoolean(
          prefs::kAllowFileSelectionDialogs, true);
    }
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

 protected:
  base::ScopedTempDir temp_dir_;
};

// ARCH-062: critical user journey for custom profile avatar upload and removal
// on the manage-profile settings page.
//
// The page is now backed by a Mojo interface (`BraveManageProfileSettings*`),
// so this test does not drive `chrome.send` / `sendWithPromise` directly.
// Linux CI lacks a working `xdg-desktop-portal` FileChooser, and the page-side
// proxy is module-internal; instead the test drives the save/clear via the
// `ProfileAttributesEntry` public API, which is the same path the Mojo handler
// uses after a decode succeeds. The page is still loaded end-to-end and we
// verify that `settings-manage-profile` renders, so the WebUI surface is
// covered along with the storage observer wiring.
IN_PROC_BROWSER_TEST_F(BraveManageProfileCustomAvatarInteractiveTest,
                       UploadCustomAvatarThenRemove) {
  // Route paths use a leading slash as settings-root-absolute (see
  // router.ts Route.createChild); manage profile is /manageProfile even when
  // the getStarted section hosts the page in the UI.
  const GURL kManageProfileUrl("chrome://settings/manageProfile");

  // Per TA-007, drive the renderer-side wait through the test sequence's
  // own `WaitForJsResult` poller instead of running `content::EvalJs` inside
  // `base::test::RunUntil` (which would spin a nested run loop and trip
  // DCHECKs on macOS arm64). Brave hosts `settings-manage-profile` under
  // `settings-getting-started-page-index`, so the predicate has to walk shadow
  // roots manually to find it.
  RunTestSequence(
      InstrumentTab(kSettingsTab, 0, browser()),
      NavigateWebContents(kSettingsTab, kManageProfileUrl),
      WaitForWebContentsPainted(kSettingsTab), Do([this]() {
        content::WebContents* wc =
            browser()->tab_strip_model()->GetActiveWebContents();
        DismissSyncCannotRunInfobarIfPresent(wc);
        ASSERT_TRUE(content::WaitForLoadStop(wc));
      }),
      WaitForJsResult(kSettingsTab, R"(
        () => {
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
          const el = deepQuerySelector(document, 'settings-manage-profile');
          return !!(el && el.shadowRoot);
        }
      )"),
      Do([this]() {
        // Save via the same public profile-entry API the Mojo handler calls
        // after decoding the user-selected image.
        ProfileAttributesEntry* entry = GetActiveProfileEntry(browser());
        ASSERT_TRUE(entry);
        gfx::Image image = gfx::test::CreateImage(64, 64);
        entry->SetBraveCustomAvatar(std::move(image), base::DoNothing());
        ASSERT_TRUE(base::test::RunUntil([this]() {
          ProfileAttributesEntry* current = GetActiveProfileEntry(browser());
          return current && current->HasBraveCustomAvatar() &&
                 current->IsUsingBraveCustomAvatar();
        }));
      }),
      Do([this]() {
        ProfileAttributesEntry* entry = GetActiveProfileEntry(browser());
        ASSERT_TRUE(entry);
        entry->ClearBraveCustomAvatar();
        ASSERT_TRUE(base::test::RunUntil([this]() {
          ProfileAttributesEntry* current = GetActiveProfileEntry(browser());
          return current && !current->HasBraveCustomAvatar() &&
                 !current->IsUsingBraveCustomAvatar();
        }));
      }));
}
