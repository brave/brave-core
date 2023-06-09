/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/browser/brave_app_controller_mac.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/browser_commands.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"

@implementation BraveAppController

@synthesize copyMenuItem = _copyMenuItem;
@synthesize copyCleanLinkMenuItem = _copyCleanLinkMenuItem;

- (void)mainMenuCreated {
  [super mainMenuCreated];

  NSMenu* editMenu = [[[NSApp mainMenu] itemWithTag:IDC_EDIT_MENU] submenu];
  _copyMenuItem = [editMenu itemWithTag:IDC_CONTENT_CONTEXT_COPY];
  DCHECK(self.copyMenuItem);
  [[self.copyMenuItem menu] setDelegate:self];
  _copyCleanLinkMenuItem = [editMenu itemWithTag:IDC_COPY_CLEAN_LINK];
  DCHECK(self.copyCleanLinkMenuItem);
  [[self.copyCleanLinkMenuItem menu] setDelegate:self];
}

- (void)dealloc {
  [[self.copyMenuItem menu] setDelegate:nil];
  [[self.copyCleanLinkMenuItem menu] setDelegate:nil];
}

- (Browser*)getBrowser {
  return chrome::FindBrowserWithProfile([self lastProfileIfLoaded]);
}

- (BOOL)shouldShowCleanLinkItem {
  return brave::HasSelectedURL([self getBrowser]);
}

- (void)setKeyEquivalentToItem:(NSMenuItem*)item {
  auto* hotkeyItem = item == self.copyMenuItem ? self.copyMenuItem
                                               : self.copyCleanLinkMenuItem;
  auto* noHotkeyItem = item == self.copyMenuItem ? self.copyCleanLinkMenuItem
                                                 : self.copyMenuItem;

  [hotkeyItem setKeyEquivalent:@"c"];
  [hotkeyItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];

  [noHotkeyItem setKeyEquivalent:@""];
  [noHotkeyItem setKeyEquivalentModifierMask:0];
}

- (void)menuNeedsUpdate:(NSMenu*)menu {
  if (menu != [self.copyMenuItem menu] &&
      menu != [self.copyCleanLinkMenuItem menu]) {
    [super menuNeedsUpdate:menu];
    return;
  }
  if ([self shouldShowCleanLinkItem]) {
    [self.copyCleanLinkMenuItem setHidden:NO];
    if (base::FeatureList::IsEnabled(features::kBraveCopyCleanLinkByDefault)) {
      [self setKeyEquivalentToItem:self.copyCleanLinkMenuItem];
    } else {
      [self setKeyEquivalentToItem:self.copyMenuItem];
    }
  } else {
    [self.copyCleanLinkMenuItem setHidden:YES];
    [self setKeyEquivalentToItem:self.copyMenuItem];
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

+ (BraveAppController*)sharedController {
  static BraveAppController* sharedController = [] {
    BraveAppController* sharedController = [[BraveAppController alloc] init];
    NSApp.delegate = sharedController;
    return sharedController;
  }();

  CHECK_NE(nil, sharedController);
  CHECK_EQ(NSApp.delegate, sharedController);
  return sharedController;
}

- (instancetype)init {
  return [super initForBrave];
}

@end  // @implementation BraveAppController
