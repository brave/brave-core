// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>

#include "brave/browser/ui/tabs/public/detached_tab_interface.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/android/tab_features.h"
#else
#include "chrome/browser/ui/tabs/public/tab_features.h"
#endif

namespace tabs {

DetachedTabInterface::DetachedTabInterface(
    std::unique_ptr<content::WebContents> contents)
    : contents_(std::move(contents)) {
  tab_features_ = std::make_unique<TabFeatures>();
  tab_features_->Init(
      *this, Profile::FromBrowserContext(contents_->GetBrowserContext()));
}

DetachedTabInterface::~DetachedTabInterface() = default;

base::WeakPtr<TabInterface> DetachedTabInterface::GetWeakPtr() {
  return tab_.GetWeakPtr();
}

content::WebContents* DetachedTabInterface::GetContents() const {
  return contents_.get();
}

void DetachedTabInterface::Close() {}

bool DetachedTabInterface::IsActivated() const {
  return false;
}

bool DetachedTabInterface::IsVisible() const {
  return false;
}

bool DetachedTabInterface::IsSelected() const {
  return false;
}

bool DetachedTabInterface::CanShowModalUI() const {
  return false;
}

std::unique_ptr<ScopedTabModalUI> DetachedTabInterface::ShowModalUI() {
  return nullptr;
}

bool DetachedTabInterface::IsInNormalWindow() const {
  return false;
}

BrowserWindowInterface* DetachedTabInterface::GetBrowserWindowInterface() {
  return nullptr;
}

const BrowserWindowInterface*
DetachedTabInterface::GetBrowserWindowInterface() const {
  return nullptr;
}

tabs::TabFeatures* DetachedTabInterface::GetTabFeatures() {
  return tab_features_.get();
}

const tabs::TabFeatures* DetachedTabInterface::GetTabFeatures() const {
  return tab_features_.get();
}

bool DetachedTabInterface::IsPinned() const {
  return false;
}

bool DetachedTabInterface::IsSplit() const {
  return false;
}

std::optional<tab_groups::TabGroupId> DetachedTabInterface::GetGroup() const {
  return std::nullopt;
}

std::optional<split_tabs::SplitTabId> DetachedTabInterface::GetSplit() const {
  return std::nullopt;
}

ui::UnownedUserDataHost& DetachedTabInterface::GetUnownedUserDataHost() {
  return unowned_user_data_host_;
}

const ui::UnownedUserDataHost& DetachedTabInterface::GetUnownedUserDataHost()
    const {
  return unowned_user_data_host_;
}

base::CallbackListSubscription
DetachedTabInterface::RegisterWillDiscardContents(
    WillDiscardContentsCallback callback) {
  return base::CallbackListSubscription();
}

base::CallbackListSubscription DetachedTabInterface::RegisterDidActivate(
    DidActivateCallback callback) {
  return base::CallbackListSubscription();
}

base::CallbackListSubscription DetachedTabInterface::RegisterDidBecomeVisible(
    DidBecomeVisibleCallback callback) {
  return base::CallbackListSubscription();
}

base::CallbackListSubscription DetachedTabInterface::RegisterWillBecomeHidden(
    WillBecomeHiddenCallback callback) {
  return base::CallbackListSubscription();
}

base::CallbackListSubscription DetachedTabInterface::RegisterWillDetach(
    WillDetach callback) {
  return base::CallbackListSubscription();
}

base::CallbackListSubscription DetachedTabInterface::RegisterDidInsert(
    DidInsertCallback callback) {
  return base::CallbackListSubscription();
}

base::CallbackListSubscription DetachedTabInterface::RegisterPinnedStateChanged(
    PinnedStateChangedCallback callback) {
  return base::CallbackListSubscription();
}

base::CallbackListSubscription DetachedTabInterface::RegisterGroupChanged(
    GroupChangedCallback callback) {
  return base::CallbackListSubscription();
}

base::CallbackListSubscription DetachedTabInterface::RegisterModalUIChanged(
    TabInterfaceCallback callback) {
  return base::CallbackListSubscription();
}

}  // namespace tabs
