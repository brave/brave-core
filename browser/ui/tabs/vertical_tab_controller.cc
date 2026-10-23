// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/auto_reset.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/strings/to_string.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/sessions/brave_session_keys.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/public/switches.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

namespace {

std::optional<bool>& PendingVerticalTabsOverride() {
  // UI thread. Session restore sets this; the next normal window consumes it.
  static std::optional<bool> pending;
  return pending;
}

std::optional<bool> EnabledOverrideFromSession(
    const std::map<std::string, std::string>& extra_data) {
  const auto it = extra_data.find(kBraveVerticalTabsEnabledKey);
  if (it == extra_data.end() || it->second.empty()) {
    return std::nullopt;
  }
  if (it->second == "true") {
    return true;
  }
  if (it->second == "false") {
    return false;
  }
  return std::nullopt;
}

}  // namespace

DEFINE_USER_DATA(VerticalTabController);

// static
VerticalTabController* VerticalTabController::From(
    BrowserWindowInterface* browser) {
  if (!browser) {
    return nullptr;
  }
  return Get(browser->GetUnownedUserDataHost());
}

// static
const VerticalTabController* VerticalTabController::From(
    const BrowserWindowInterface* browser) {
  if (!browser) {
    return nullptr;
  }
  return Get(browser->GetUnownedUserDataHost());
}

VerticalTabController::VerticalTabController(
    ui::UnownedUserDataHost& host,
    BrowserWindowInterface* browser,
    BrowserWindowInterface::Type type,
    PrefService* prefs,
    FocusModeController* focus_mode_controller)
    : type_(type),
      browser_(browser),
      prefs_(prefs),
      focus_mode_controller_(focus_mode_controller),
      scoped_unowned_user_data_(host, *this) {
  if (type_ == BrowserWindowInterface::TYPE_NORMAL) {
    enabled_override_ =
        std::exchange(PendingVerticalTabsOverride(), std::nullopt);
  }
  pref_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_registrar_->Init(prefs_);
  pref_registrar_->Add(
      brave_tabs::kVerticalTabsEnabled,
      base::BindRepeating(&VerticalTabController::OnProfilePrefChanged,
                          base::Unretained(this)));
  // A copied value is an exception only while it disagrees with the shared
  // setting. A restored choice that already matches follows the setting.
  // Session restore writes the cleared value after the window finishes opening.
  if (EnabledOverrideMatchesProfilePref()) {
    enabled_override_.reset();
  }
  // The tab strip model is built before this controller and only sees the
  // profile pref.
  if (enabled_override_.has_value()) {
    SyncTreeTabs();
  }
  // SessionService tracks the window only after it finishes opening.
  if (browser_ && base::SequencedTaskRunner::HasCurrentDefault()) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&VerticalTabController::WriteEnabledOverrideToSession,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

VerticalTabController::~VerticalTabController() = default;

bool VerticalTabController::SupportsBraveVerticalTabs() const {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          tabs::switches::kDisableVerticalTabsSwitch)) {
    return false;
  }

  if (tabs::IsUpstreamVerticalTabsForceEnabled()) {
    // In case that Chromium's vertical tabs are enabled, we should not show
    // Brave's vertical tabs.
    return false;
  }

  if (base::FeatureList::IsEnabled(tabs::kTabStripUnification)) {
    // Our vertical tabs assume the horizontal tab strip region view is always a
    // `HorizontalTabStripRegionViewOld` (aliased as
    // `HorizontalTabStripRegionView`) and reach into its `tab_strip_` field
    // directly. Under `kTabStripUnification`, the horizontal region view can
    // instead be a `HorizontalTabStripRegionViewNew`, which has no such field,
    // so unsupported for now.
    return false;
  }

  return type_ == BrowserWindowInterface::TYPE_NORMAL;
}

bool VerticalTabController::ShouldShowBraveVerticalTabs() const {
  if (!SupportsBraveVerticalTabs()) {
    return false;
  }

  if (enabled_override_.has_value()) {
    return *enabled_override_;
  }

  return prefs_->GetBoolean(brave_tabs::kVerticalTabsEnabled);
}

void VerticalTabController::SetVerticalTabsEnabledForWindow(bool enabled) {
  if (!SupportsBraveVerticalTabs()) {
    return;
  }
  if (prefs_ &&
      enabled == prefs_->GetBoolean(brave_tabs::kVerticalTabsEnabled)) {
    FollowProfilePref();
    return;
  }
  if (enabled_override_ == enabled) {
    return;
  }
  enabled_override_ = enabled;
  SyncTreeTabs();
  enabled_changed_callbacks_.Notify();
  WriteEnabledOverrideToSession();
}

void VerticalTabController::FollowProfilePref() {
  if (!enabled_override_.has_value()) {
    return;
  }
  enabled_override_.reset();
  SyncTreeTabs();
  enabled_changed_callbacks_.Notify();
  WriteEnabledOverrideToSession();
}

base::CallbackListSubscription VerticalTabController::AddEnabledChangedCallback(
    base::RepeatingCallback<void()> callback) {
  return enabled_changed_callbacks_.Add(std::move(callback));
}

// static
base::AutoReset<std::optional<bool>>
VerticalTabController::InheritEnabledOverrideFromSession(
    BrowserWindowInterface::Type type,
    const std::map<std::string, std::string>& extra_data) {
  if (type != BrowserWindowInterface::TYPE_NORMAL) {
    return base::AutoReset<std::optional<bool>>(&PendingVerticalTabsOverride(),
                                                PendingVerticalTabsOverride());
  }
  return InheritEnabledOverrideForTesting(
      EnabledOverrideFromSession(extra_data));
}

// static
base::AutoReset<std::optional<bool>>
VerticalTabController::InheritEnabledOverrideForTesting(
    std::optional<bool> enabled) {
  return base::AutoReset<std::optional<bool>>(&PendingVerticalTabsOverride(),
                                              std::move(enabled));
}

bool VerticalTabController::ShouldShowWindowTitleForVerticalTabs() const {
  if (!ShouldShowBraveVerticalTabs()) {
    return false;
  }

  if (focus_mode_controller_ && focus_mode_controller_->IsEnabled()) {
    return false;
  }

  return prefs_->GetBoolean(brave_tabs::kVerticalTabsShowTitleOnWindow);
}

bool VerticalTabController::IsFloatingVerticalTabsEnabled() const {
  if (!ShouldShowBraveVerticalTabs()) {
    return false;
  }

  if (ShouldHideVerticalTabsCompletelyWhenCollapsed()) {
    // In this case, we should support floating mode regardless of the setting
    // of kVerticalTabsFloatingEnabled.
    return true;
  }

  if (!ShouldShowVerticalTabToggleButton()) {
    // When the toggle button is hidden, there is no other way to expand
    // collapsed vertical tabs, so floating mode must stay on regardless of
    // the setting of kVerticalTabsFloatingEnabled.
    return true;
  }

  return prefs_->GetBoolean(brave_tabs::kVerticalTabsFloatingEnabled);
}

bool VerticalTabController::IsVerticalTabOnRight() const {
  return prefs_->GetBoolean(brave_tabs::kVerticalTabsOnRight);
}

bool VerticalTabController::ShouldHideVerticalTabsCompletelyWhenCollapsed()
    const {
  return base::FeatureList::IsEnabled(tabs::kBraveVerticalTabHideCompletely) &&
         prefs_->GetBoolean(
             brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed);
}

bool VerticalTabController::ShouldShowVerticalTabToggleButton() const {
  if (!ShouldShowBraveVerticalTabs()) {
    return false;
  }

  return prefs_->GetBoolean(brave_tabs::kVerticalTabsShowToggleButton);
}

bool VerticalTabController::EnabledOverrideMatchesProfilePref() const {
  return enabled_override_.has_value() &&
         *enabled_override_ ==
             prefs_->GetBoolean(brave_tabs::kVerticalTabsEnabled);
}

void VerticalTabController::OnProfilePrefChanged() {
  // A stored override is a disagreement with the shared setting. When this
  // change makes them the same, the disagreement ends and this window follows
  // again. A window that is already following updates below.
  if (enabled_override_.has_value()) {
    if (EnabledOverrideMatchesProfilePref()) {
      FollowProfilePref();
    }
    return;
  }
  enabled_changed_callbacks_.Notify();
}

void VerticalTabController::WriteEnabledOverrideToSession() {
  if (!browser_) {
    return;
  }
  Profile* profile = browser_->GetProfile();
  if (!profile) {
    return;
  }
  SessionService* service =
      SessionServiceFactory::GetForProfileIfExisting(profile);
  if (!service) {
    return;
  }
  std::string value;
  if (enabled_override_.has_value()) {
    value = base::ToString(*enabled_override_);
  }
  service->AddWindowExtraData(browser_->GetSessionID(),
                              kBraveVerticalTabsEnabledKey, value);
}

void VerticalTabController::SyncTreeTabs() {
  if (!browser_) {
    return;
  }
  auto* model = browser_->GetTabStripModel();
  if (!model) {
    return;
  }
  static_cast<BraveTabStripModel*>(model)->SyncTreeTabsWithWindowVerticalTabs();
}
