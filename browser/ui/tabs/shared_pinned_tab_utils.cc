/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/public/shared_pinned_tab_utils.h"

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/unload_controller.h"
#include "components/prefs/pref_service.h"

namespace brave_tabs {

namespace {

bool AreAllTabsSharedPinnedTabs(BrowserWindowInterface* browser) {
  if (!base::FeatureList::IsEnabled(tabs::kBraveSharedPinnedTabs)) {
    return false;
  }

  if (browser->GetType() != BrowserWindowInterface::Type::TYPE_NORMAL) {
    return false;
  }

  if (!browser->GetProfile()->GetPrefs()->GetBoolean(kSharedPinnedTab)) {
    return false;
  }

  TabStripModel* tab_strip_model = browser->GetTabStripModel();
  return tab_strip_model->count() > 0 &&
         tab_strip_model->count() ==
             tab_strip_model->IndexOfFirstNonPinnedTab();
}

void CloseWindowWithoutConfirmation(
    base::WeakPtr<BrowserWindowInterface> browser) {
  if (!browser) {
    return;
  }
  // The shared pinned tabs are also in another window, so there is nothing to
  // warn users about.
  UnloadController::From(browser.get())->set_confirmed_to_close(true);
  chrome::CloseWindow(browser.get());
}

}  // namespace

void MaybeCloseWindowWithOnlySharedPinnedTabs(BrowserWindowInterface* browser) {
  if (!AreAllTabsSharedPinnedTabs(browser)) {
    return;
  }

  bool more_than_one = false;
  ProfileBrowserCollection::GetForProfile(browser->GetProfile())
      ->ForEach([browser, &more_than_one](BrowserWindowInterface*) {
        if (!more_than_one) {
          more_than_one = true;
          return true;
        }

        base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
            FROM_HERE, base::BindOnce(&CloseWindowWithoutConfirmation,
                                      browser->GetWeakPtr()));
        return false;
      });
}

}  // namespace brave_tabs
