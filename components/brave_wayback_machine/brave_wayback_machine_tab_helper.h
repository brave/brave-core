/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/gfx/native_widget_types.h"

class BraveWaybackMachineDelegate;
class PrefService;

class BraveWaybackMachineTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BraveWaybackMachineTabHelper> {
 public:
  explicit BraveWaybackMachineTabHelper(content::WebContents* contents);
  ~BraveWaybackMachineTabHelper() override;

  BraveWaybackMachineTabHelper(const BraveWaybackMachineTabHelper&) = delete;
  BraveWaybackMachineTabHelper& operator=(
      const BraveWaybackMachineTabHelper&) = delete;

  void set_delegate(std::unique_ptr<BraveWaybackMachineDelegate> delegate);
  void set_active_dialog(gfx::NativeWindow dialog) { active_dialog_ = dialog; }
  gfx::NativeWindow active_dialog() const { return active_dialog_; }

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveWaybackMachineTest, DialogLaunchTest);

  // content::WebContentsObserver overrides:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void ShowWaybackMachineDialog();
  bool IsWaybackMachineEnabled() const;

  bool ShouldShowWaybackMachineDialog(int response_code) const;

  // If |active_dialog_| exists, close it before launching another one.
  gfx::NativeWindow active_dialog_ = nullptr;
  raw_ref<PrefService> pref_service_;
  std::unique_ptr<BraveWaybackMachineDelegate> delegate_;

  base::WeakPtrFactory<BraveWaybackMachineTabHelper> weak_factory_;

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_
