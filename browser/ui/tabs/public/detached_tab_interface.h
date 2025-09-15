// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_DETACHED_TAB_INTERFACE_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_DETACHED_TAB_INTERFACE_H_

#include "base/memory/weak_ptr.h"
#include "components/tabs/public/split_tab_id.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/unowned_user_data/unowned_user_data_host.h"

namespace tabs {

class TabFeatures;

class DetachedTabInterface : public TabInterface {
 public:
  explicit DetachedTabInterface(std::unique_ptr<content::WebContents> contents);
  ~DetachedTabInterface() override;

  // TabInterface:
  base::WeakPtr<TabInterface> GetWeakPtr() override;

  content::WebContents* GetContents() const override;

  void Close() override;

  ui::UnownedUserDataHost& GetUnownedUserDataHost() override;
  const ui::UnownedUserDataHost& GetUnownedUserDataHost() const override;

  base::CallbackListSubscription RegisterWillDiscardContents(
      TabInterface::WillDiscardContentsCallback callback) override;

  bool IsActivated() const override;
  base::CallbackListSubscription RegisterDidActivate(
      TabInterface::DidActivateCallback callback) override;
  bool IsVisible() const override;
  bool IsSelected() const override;
  base::CallbackListSubscription RegisterDidBecomeVisible(
      DidBecomeVisibleCallback callback) override;
  base::CallbackListSubscription RegisterWillBecomeHidden(
      WillBecomeHiddenCallback callback) override;
  base::CallbackListSubscription RegisterWillDetach(
      WillDetach callback) override;
  base::CallbackListSubscription RegisterDidInsert(
      DidInsertCallback callback) override;
  base::CallbackListSubscription RegisterPinnedStateChanged(
      PinnedStateChangedCallback callback) override;
  base::CallbackListSubscription RegisterGroupChanged(
      GroupChangedCallback callback) override;

  bool CanShowModalUI() const override;
  std::unique_ptr<ScopedTabModalUI> ShowModalUI() override;
  base::CallbackListSubscription RegisterModalUIChanged(
      TabInterfaceCallback callback) override;
  bool IsInNormalWindow() const override;

  BrowserWindowInterface* GetBrowserWindowInterface() override;
  const BrowserWindowInterface* GetBrowserWindowInterface() const override;

  tabs::TabFeatures* GetTabFeatures() override;
  const tabs::TabFeatures* GetTabFeatures() const override;
  bool IsPinned() const override;
  bool IsSplit() const override;
  std::optional<tab_groups::TabGroupId> GetGroup() const override;
  std::optional<split_tabs::SplitTabId> GetSplit() const override;

 private:
  std::unique_ptr<content::WebContents> contents_;
  ui::UnownedUserDataHost unowned_user_data_host_;
  std::unique_ptr<TabFeatures> tab_features_;
  base::WeakPtrFactory<DetachedTabInterface> tab_{this};
};

}  // namespace tabs

#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_DETACHED_TAB_INTERFACE_H_
