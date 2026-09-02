// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/profiles/profile_picker.h"
#include "chrome/browser/ui/profiles/profile_ui_test_utils.h"
#include "chrome/browser/ui/views/profiles/profile_picker_test_base.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

namespace {

// Keep this at NGShapeCache's 30-code-unit limit so it remains cacheable while
// still overflowing the profile name input.
constexpr char16_t kLongProfileName[] =
    u"WideProfileNameWideProfileName";

// The profile card is rendered after profiles-list-changed. Wait for each DOM
// element with MutationObserver, following Chromium's pattern in
// content/browser/service_worker/service_worker_internals_ui_browsertest.cc.
constexpr char kShowNameTooltipScript[] = R"(
  (async () => {
    const waitForElement = (root, selector) => {
      const element = root.querySelector(selector);
      if (element) {
        return Promise.resolve(element);
      }

      return new Promise(resolve => {
        const observer = new MutationObserver(() => {
          const element = root.querySelector(selector);
          if (element) {
            observer.disconnect();
            resolve(element);
          }
        });
        observer.observe(root, {childList: true, subtree: true});
      });
    };

    const getShadowRoot = element => {
      if (!element.shadowRoot) {
        throw new Error(`Missing shadow root for ${element.localName}`);
      }
      return element.shadowRoot;
    };

    const app = await waitForElement(document, 'profile-picker-app');
    const mainView =
        await waitForElement(getShadowRoot(app), '#mainView');
    const card =
        await waitForElement(getShadowRoot(mainView), 'profile-card');

    const cardRoot = getShadowRoot(card);
    const [nameInput, tooltip] = await Promise.all([
      waitForElement(cardRoot, '#nameInput'),
      waitForElement(cardRoot, '#tooltip'),
    ]);
    await document.fonts.ready;

    const [input, tooltipElement] = await Promise.all([
      waitForElement(getShadowRoot(nameInput), '#input'),
      waitForElement(getShadowRoot(tooltip), '#tooltip'),
    ]);
    await new Promise(resolve => requestAnimationFrame(resolve));

    const diagnostic = () => JSON.stringify({
      name: input.value,
      scrollWidth: input.scrollWidth,
      offsetWidth: input.offsetWidth,
      tooltipHidden: tooltipElement.hidden,
    });

    if (input.scrollWidth <= input.offsetWidth) {
      return `Profile name is not truncated: ${diagnostic()}`;
    }

    nameInput.dispatchEvent(new MouseEvent('mouseenter'));
    if (tooltipElement.hidden) {
      return `Tooltip stayed hidden after mouseenter: ${diagnostic()}`;
    }
    return true;
  })()
)";

class BraveProfilePickerViewBrowserTest : public ProfilePickerTestBase {};

// The shape cache key must distinguish exact font sizes because HarfBuzz uses
// that size even when nearby sizes share SimpleFontData and its NGShapeCache.
IN_PROC_BROWSER_TEST_F(BraveProfilePickerViewBrowserTest,
                       ShowsTooltipForTruncatedProfileName) {
  ProfileManager* const profile_manager = g_browser_process->profile_manager();
  ASSERT_NE(profile_manager, nullptr);
  ProfileAttributesEntry* const entry =
      profile_manager->GetProfileAttributesStorage()
          .GetProfileAttributesWithPath(browser()->GetProfile()->GetPath());
  ASSERT_NE(entry, nullptr);
  entry->SetLocalProfileName(kLongProfileName, /*is_default_name=*/false);

  ProfilePicker::Show(ProfilePicker::Params::FromEntryPoint(
      ProfilePicker::EntryPoint::kProfileMenuManageProfiles));
  profiles::testing::WaitForPickerUrl(GURL{"chrome://profile-picker"});

  content::WebContents* const contents = web_contents();
  ASSERT_NE(contents, nullptr);
  EXPECT_EQ(true, content::EvalJs(contents, kShowNameTooltipScript,
                                  content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                  ISOLATED_WORLD_ID_BRAVE_INTERNAL));
}

}  // namespace
