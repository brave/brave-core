/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/browser/brave_app_controller_mac.h"

#import <Foundation/Foundation.h>
#import <objc/runtime.h>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/grit/generated_resources.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/prefs/pref_member.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/pref_names.h"
#endif

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

#if BUILDFLAG(ENABLE_TOR)
class TorPrefObserver : public BooleanPrefMember {
 public:
  TorPrefObserver(BraveAppController* controller, NSMenu* menu)
      : controller_(controller), menu_(menu) {
    Profile* profile = [controller_ lastProfileIfLoaded];
    CHECK(profile);

    // The incognito profile will be deleted before app termination. So we
    // should original profile in order to avoid crash.
    // https://github.com/brave/brave-core/pull/21892#issuecomment-1928944343
    profile = profile->GetOriginalProfile();

    incognito_pref_observer_.Init(
        policy::policy_prefs::kIncognitoModeAvailability, profile->GetPrefs(),
        base::BindRepeating(&TorPrefObserver::UpdateMenu,
                            base::Unretained(this)));
    CHECK(g_browser_process);
    CHECK(g_browser_process->local_state());
    tor_disabled_pref_observer_.Init(
        tor::prefs::kTorDisabled, g_browser_process->local_state(),
        base::BindRepeating(&TorPrefObserver::UpdateMenu,
                            base::Unretained(this)));
  }

 private:
  void UpdateMenu() { [controller_ menuNeedsUpdate:menu_]; }

  BraveAppController* controller_;  // Owner of this
  NSMenu* menu_;

  IntegerPrefMember incognito_pref_observer_;
  BooleanPrefMember tor_disabled_pref_observer_;
};
#endif  // BUILDFLAG(ENABLE_TOR)

}  // namespace

@interface AppController (Brave)
// Expose method in chrome/..app_controller_mac.mm
- (BOOL)canOpenNewBrowser;
@end

@interface BraveAppController () {
  NSMenuItem* _copyMenuItem;
  NSMenuItem* _copyCleanLinkMenuItem;

#if BUILDFLAG(ENABLE_TOR)
  NSMenuItem* _torMenuItem;
  std::unique_ptr<TorPrefObserver> tor_pref_observer_;
#endif  // BUILDFLAG(ENABLE_TOR)
}
@end

@implementation BraveAppController
- (void)mainMenuCreated {
  [super mainMenuCreated];

  NSMenu* editMenu = [[[NSApp mainMenu] itemWithTag:IDC_EDIT_MENU] submenu];
  _copyMenuItem = [editMenu itemWithTag:IDC_CONTENT_CONTEXT_COPY];
  DCHECK(_copyMenuItem);

  [[_copyMenuItem menu] setDelegate:self];
  _copyCleanLinkMenuItem = [editMenu itemWithTag:IDC_COPY_CLEAN_LINK];
  DCHECK(_copyCleanLinkMenuItem);
  [[_copyCleanLinkMenuItem menu] setDelegate:self];
}

- (void)dealloc {
  [[_copyMenuItem menu] setDelegate:nil];
  [[_copyCleanLinkMenuItem menu] setDelegate:nil];
}

- (void)applicationWillTerminate:(NSNotification*)notification {
  tor_pref_observer_.reset();
  _torMenuItem = nil;
  [super applicationWillTerminate:notification];
}

- (Browser*)getBrowser {
  return chrome::FindBrowserWithProfile([self lastProfileIfLoaded]);
}

- (BOOL)shouldShowCleanLinkItem {
  return brave::HasSelectedURL([self getBrowser]);
}

- (void)setKeyEquivalentToItem:(NSMenuItem*)item {
  auto* hotkeyItem =
      item == _copyMenuItem ? _copyMenuItem : _copyCleanLinkMenuItem;
  auto* noHotkeyItem =
      item == _copyMenuItem ? _copyCleanLinkMenuItem : _copyMenuItem;

  [hotkeyItem setKeyEquivalent:@"c"];
  [hotkeyItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];

  [noHotkeyItem setKeyEquivalent:@""];
  [noHotkeyItem setKeyEquivalentModifierMask:0];
}

- (void)menuNeedsUpdate:(NSMenu*)menu {
  if (menu == [_copyMenuItem menu] || menu == [_copyCleanLinkMenuItem menu]) {
    [self copyLinkMenuNeedsUpdate];
    return;
  }

#if BUILDFLAG(ENABLE_TOR)
  if (menu == [_torMenuItem menu]) {
    [self torMenuNeedsUpdate];
    return;
  }
#endif  // BUILDFLAG(ENALBLE_TOR)

  [super menuNeedsUpdate:menu];
}

- (void)copyLinkMenuNeedsUpdate {
  if ([self shouldShowCleanLinkItem]) {
    [_copyCleanLinkMenuItem setHidden:NO];
    if (base::FeatureList::IsEnabled(features::kBraveCopyCleanLinkByDefault)) {
      [self setKeyEquivalentToItem:_copyCleanLinkMenuItem];
    } else {
      [self setKeyEquivalentToItem:_copyMenuItem];
    }
  } else {
    [_copyCleanLinkMenuItem setHidden:YES];
    [self setKeyEquivalentToItem:_copyMenuItem];
  }
}

#if BUILDFLAG(ENABLE_TOR)
- (void)torMenuNeedsUpdate {
  _torMenuItem.enabled = [self validateUserInterfaceItem:_torMenuItem];
  _torMenuItem.hidden = !_torMenuItem.enabled;
}
#endif  // BUILDFLAG(ENABLE_TOR)

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
  NSInteger tag = [item tag];
  if (tag == IDC_COPY_CLEAN_LINK) {
    return [self shouldShowCleanLinkItem];
  }

#if BUILDFLAG(ENABLE_TOR)
  SEL action = [item action];
  if (action == @selector(commandFromDock:) &&
      tag == IDC_NEW_OFFTHERECORD_WINDOW_TOR) {
    auto* profile = [self lastProfileIfLoaded];
    return profile && !TorProfileServiceFactory::IsTorDisabled(profile) &&
           [self canOpenNewBrowser];
  }
#endif  // BUILDFLAG(ENABLE_TOR)

  return [super validateUserInterfaceItem:item];
}

- (void)executeCommand:(id)sender withProfile:(Profile*)profile {
  if (!profile) {
    // Couldn't load the Profile. RunInSafeProfileHelper will show the
    // ProfilePicker instead.
    return;
  }

  NSInteger tag = [sender tag];
  if (tag == IDC_COPY_CLEAN_LINK) {
    brave::CleanAndCopySelectedURL([self getBrowser]);
    return;
  }

#if BUILDFLAG(ENABLE_TOR)
  if (tag == IDC_NEW_OFFTHERECORD_WINDOW_TOR) {
    brave::NewOffTheRecordWindowTor(profile);
    return;
  }
#endif  // BUILDFLAG(ENABLE_TOR)

  [super executeCommand:sender withProfile:profile];
}

- (NSMenu*)applicationDockMenu:(NSApplication*)sender {
  auto* menu = [super applicationDockMenu:sender];

#if BUILDFLAG(ENABLE_TOR)
  // Add "New Private Window with Tor" only if the "New Private Window" item
  // exists. "New Private Window" could be missing when policy is set to
  // disable it.
  auto* open_new_private_window = [menu itemWithTag:IDC_NEW_INCOGNITO_WINDOW];
  if (open_new_private_window) {
    auto index = [menu indexOfItem:open_new_private_window];
    _torMenuItem =
        [[NSMenuItem alloc] initWithTitle:l10n_util::GetNSStringWithFixup(
                                              IDS_NEW_OFFTHERECORD_WINDOW_TOR)
                                   action:@selector(commandFromDock:)
                            keyEquivalent:@""];
    [menu insertItem:_torMenuItem atIndex:index + 1];
    _torMenuItem.target = self;
    _torMenuItem.tag = IDC_NEW_OFFTHERECORD_WINDOW_TOR;
    [self torMenuNeedsUpdate];
    tor_pref_observer_ =
        std::make_unique<TorPrefObserver>(self, [_torMenuItem menu]);
  }
#endif

  return menu;
}

@end  // @implementation BraveAppController
