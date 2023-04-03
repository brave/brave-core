/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_APP_BRAVE_CORE_SWITCHES_H_
#define BRAVE_IOS_APP_BRAVE_CORE_SWITCHES_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSString* BraveCoreSwitchKey NS_STRING_ENUM;
/// Overrides the component updater source. Defaults to the CI provided value
///
/// Expected value: url-source={url}
OBJC_EXPORT const BraveCoreSwitchKey BraveCoreSwitchKeyComponentUpdater;
/// Overrides Chromium VLOG verbosity. Defaults to only printing from folders
/// existing within a `brave` subfolder up to level 0.
///
/// Expected value: {folder-expression}={level}
OBJC_EXPORT const BraveCoreSwitchKey BraveCoreSwitchKeyVModule;
/// Overrides the sync service base URL. Defaults to the CI provided value
///
/// Expected value: A URL string
OBJC_EXPORT const BraveCoreSwitchKey BraveCoreSwitchKeySyncURL;
/// Sets a number of overrides for the ads & ledger services such as which
/// environment its using, debug mode, etc.
///
/// Expected value: A comma-separated list of flags, including:
///     - staging={bool}
////    - development={bool}
///     - debug={bool}
///     - reconcile-interval={int}
///     - retry-interval={int}
OBJC_EXPORT const BraveCoreSwitchKey BraveCoreSwitchKeyRewardsFlags;
/// Overrides the number of seconds to upload P3A metrics
///
/// Expected value: A number (in seconds)
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP3AUploadIntervalSeconds NS_SWIFT_NAME(p3aUploadIntervalSeconds);  // NOLINT
/// Avoid upload interval randomization
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP3ADoNotRandomizeUploadInterval NS_SWIFT_NAME(p3aDoNotRandomizeUploadInterval);  // NOLINT
/// Interval between restarting the uploading process for all gathered values
///
/// Expected value: A number (in seconds)
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP3ATypicalRotationIntervalSeconds NS_SWIFT_NAME(p3aTypicalRotationIntervalSeconds);  // NOLINT
/// Interval between restarting the uploading process for all gathered values
///
/// Expected value: A number (in seconds)
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP3AExpressRotationIntervalSeconds NS_SWIFT_NAME(p3aExpressRotationIntervalSeconds);  // NOLINT
/// Interval between restarting the uploading process for all gathered values
///
/// Expected value: A number (in seconds)
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP3ASlowRotationIntervalSeconds NS_SWIFT_NAME(p3aSlowRotationIntervalSeconds);  // NOLINT
/// Overrides the P3A JSON backend URL.
///
/// Expected value: A URL string
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP3AJsonUploadServerURL NS_SWIFT_NAME(p3aJsonUploadServerURL);
/// Overrides the P3A Creative JSON backend URL.
///
/// Expected value: A URL string
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP3ACreativeUploadServerURL NS_SWIFT_NAME(p3aCreativeUploadServerURL);
/// Overrides the P2A JSON backend URL.
///
/// Expected value: A URL string
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP2AJsonUploadServerURL NS_SWIFT_NAME(p2aJsonUploadServerURL);
/// Overrides the P3A Constellation backend URL.
///
/// Expected value: A URL string
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP3AConstellationUploadServerURL NS_SWIFT_NAME(p3aConstellationUploadServerURL);
/// Overrides the P3A "disable star attestation" setting.
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP3ADisableStarAttestation NS_SWIFT_NAME(p3aDisableStarAttestation);
/// Overrides the P3A STAR randomness host.
///
/// Expected value: A URL string
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP3AStarRandomnessHost NS_SWIFT_NAME(p3aStarRandomnessHost);
/// Do not try to resent values even if a cloud returned an HTTP error, just
/// continue the normal process.
OBJC_EXPORT const BraveCoreSwitchKey
    BraveCoreSwitchKeyP3AIgnoreServerErrors NS_SWIFT_NAME(p3aIgnoreServerErrors);  // NOLINT

/// Defines a switch that may be overriden on launch.
///
/// These are essentially the same as passing a command line argument in the
/// format `--key=value` (or `--key` if you do not require a value)
OBJC_EXPORT
@interface BraveCoreSwitch : NSObject
@property(readonly) BraveCoreSwitchKey key;
@property(readonly, nullable) NSString* value;
- (instancetype)initWithKey:(BraveCoreSwitchKey)key;
- (instancetype)initWithKey:(BraveCoreSwitchKey)key
                      value:(nullable NSString*)value NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_APP_BRAVE_CORE_SWITCHES_H_
