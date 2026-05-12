/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ref_counted_memory.h"
#include "base/test/run_until.h"
#include "chrome/browser/browser_process.h"
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
#include "content/public/browser/render_frame_host.h"
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
    test_png_path_ = temp_dir_.GetPath().AppendASCII("test_avatar.png");
    gfx::Image image(gfx::test::CreateImage(64, 64));
    scoped_refptr<base::RefCountedMemory> png = image.As1xPNGBytes();
    ASSERT_TRUE(png && png->size() > 0u);
    ASSERT_TRUE(base::WriteFile(test_png_path_, *png));
    std::string png_file_bytes;
    ASSERT_TRUE(base::ReadFileToString(test_png_path_, &png_file_bytes));
    test_png_base64_ = base::Base64Encode(png_file_bytes);
  }

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath test_png_path_;
  std::string test_png_base64_;
};

// ARCH-062: critical user journey for custom profile avatar upload and removal
// on the manage-profile settings page. Linux CI often lacks a working
// xdg-desktop-portal FileChooser; drive the same `setProfileCustomAvatar` path
// the page uses after a file pick, without opening a native file dialog.
// Waits and removal use profile storage + `chrome.send` instead of shadow-DOM
// polling so the test does not depend on Polymer flush timing (macOS CI can be
// slow or noisy).
IN_PROC_BROWSER_TEST_F(BraveManageProfileCustomAvatarInteractiveTest,
                       UploadCustomAvatarThenRemove) {
  // Route paths use a leading slash as settings-root-absolute (see
  // router.ts Route.createChild); manage profile is /manageProfile even when
  // the getStarted section hosts the page in the UI.
  const GURL kManageProfileUrl("chrome://settings/manageProfile");

  RunTestSequence(
      InstrumentTab(kSettingsTab, 0, browser()),
      NavigateWebContents(kSettingsTab, kManageProfileUrl),
      WaitForWebContentsPainted(kSettingsTab), Do([this]() {
        content::WebContents* wc =
            browser()->tab_strip_model()->GetActiveWebContents();
        DismissSyncCannotRunInfobarIfPresent(wc);
        ASSERT_TRUE(content::WaitForLoadStop(wc));
        ASSERT_TRUE(base::test::RunUntil([wc]() {
          const content::EvalJsResult result = content::EvalJs(wc, R"(
            (() => {
              const el = document.querySelector('settings-manage-profile');
              return !!(el && el.shadowRoot);
            })()
          )");
          return result.is_ok() && result.ExtractBool();
        }));
      }),
      Do([this]() {
        content::WebContents* wc =
            browser()->tab_strip_model()->GetActiveWebContents();
        content::RenderFrameHost* rfh = wc->GetPrimaryMainFrame();
        ASSERT_TRUE(content::ExecJs(
            rfh, content::JsReplace(
                     R"(
                      (async () => {
                        const cr = await import('chrome://resources/js/cr.js');
                        await cr.sendWithPromise('setProfileCustomAvatar', $1);
                        return true;
                      })()
                    )",
                     test_png_base64_)));
      }),
      Do([this]() {
        ASSERT_TRUE(base::test::RunUntil([this]() {
          ProfileAttributesEntry* entry = GetActiveProfileEntry(browser());
          return entry && entry->HasBraveCustomAvatar() &&
                 entry->IsUsingBraveCustomAvatar();
        }));
      }),
      Do([this]() {
        content::WebContents* wc =
            browser()->tab_strip_model()->GetActiveWebContents();
        content::RenderFrameHost* rfh = wc->GetPrimaryMainFrame();
        ASSERT_TRUE(
            content::ExecJs(rfh, R"(chrome.send('removeProfileCustomAvatar', []);)"));
      }),
      Do([this]() {
        ASSERT_TRUE(base::test::RunUntil([this]() {
          ProfileAttributesEntry* entry = GetActiveProfileEntry(browser());
          return entry && !entry->HasBraveCustomAvatar() &&
                 !entry->IsUsingBraveCustomAvatar();
        }));
      }));
}
