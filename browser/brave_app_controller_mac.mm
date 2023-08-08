/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#import "brave/browser/brave_app_controller_mac.h"

#import <Foundation/Foundation.h>
#import <objc/runtime.h>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/browser_commands.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BraveAppController () {
  NSMenuItem* _copyMenuItem;
  NSMenuItem* _copyCleanLinkMenuItem;
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
  if (menu != [_copyMenuItem menu] && menu != [_copyCleanLinkMenuItem menu]) {
    [super menuNeedsUpdate:menu];
    return;
  }
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

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
  NSInteger tag = [item tag];
  if (tag == IDC_COPY_CLEAN_LINK) {
    return [self shouldShowCleanLinkItem];
  }
  return [super validateUserInterfaceItem:item];
}

- (void)executeCommand:(id)sender withProfile:(Profile*)profile {
  NSInteger tag = [sender tag];
  if (tag == IDC_COPY_CLEAN_LINK) {
    brave::CleanAndCopySelectedURL([self getBrowser]);
    return;
  }

  [super executeCommand:sender withProfile:profile];
}

@end  // @implementation BraveAppController
