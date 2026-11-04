// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_CONTENT_SETTINGS_DEFAULT_HOST_CONTENT_SETTINGS_H_
#define BRAVE_IOS_BROWSER_API_CONTENT_SETTINGS_DEFAULT_HOST_CONTENT_SETTINGS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// The mode in which pages should be loaded.
typedef NS_ENUM(NSUInteger, DefaultPageMode) {
  DefaultPageModeMobile,
  DefaultPageModeDesktop,
};

/// How a link opened from another app should be loaded.
typedef NS_ENUM(NSInteger, ExternalLinkMode) {
  /// Opens in the current browsing mode.
  ExternalLinkModeCurrent = 0,
  ExternalLinkModeAsk,
  ExternalLinkModeRegular,
  ExternalLinkModePrivateBrowsing,
  ExternalLinkModeModal,
};

/// What to do with a link opened from another app.
typedef NS_ENUM(NSInteger, ExternalLinkAction) {
  ExternalLinkActionOpenCurrent = 0,
  ExternalLinkActionShowOnboarding,
  ExternalLinkActionShowPrompt,
  ExternalLinkActionOpenRegular,
  ExternalLinkActionOpenPrivate,
  ExternalLinkActionOpenModal,
};

/// A list of default website preferences.
OBJC_EXPORT
@interface DefaultHostContentSettings : NSObject

/// The default page mode in which pages should be loaded.
@property(nonatomic) DefaultPageMode defaultPageMode;

/// Whether or not popups are allowed by default
@property(nonatomic) BOOL popupsAllowed;

/// How links opened from other apps are loaded by default.
@property(nonatomic) ExternalLinkMode defaultExternalLinkMode;

/// The external link mode for the given URL, falling back to
/// `defaultExternalLinkMode` when no per-site mode is remembered.
- (ExternalLinkMode)externalLinkModeForURL:(NSURL*)url;

/// Remembers the external link mode for the given URL's base domain (eTLD+1)
/// and its subdomains.
- (void)setExternalLinkMode:(ExternalLinkMode)mode forURL:(NSURL*)url;

/// Removes all remembered per-site external link modes, keeping the default.
- (void)clearAllExternalLinkModes;

/// Whether the onboarding for links opened from other apps has been shown.
@property(nonatomic) BOOL externalLinkOnboardingShown;

/// Decides what to do with a link opened from another app. A remembered
/// per-site mode takes precedence over the default. Returns
/// `ExternalLinkActionShowOnboarding` at most once.
- (ExternalLinkAction)resolveExternalLinkForURL:(NSURL*)url;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_CONTENT_SETTINGS_DEFAULT_HOST_CONTENT_SETTINGS_H_
