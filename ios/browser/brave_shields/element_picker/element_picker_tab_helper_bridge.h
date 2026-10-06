// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_TAB_HELPER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_TAB_HELPER_BRIDGE_H_

#ifdef __cplusplus
#include "brave/ios/browser/brave_shields/element_picker/element_picker_localized_texts.h"
#else
#include "element_picker_localized_texts.h"
#endif

NS_ASSUME_NONNULL_BEGIN

@protocol ElementPickerTabHelperBridge

@required

/**
 * Persists a cosmetic filter hiding `selector` on the site at `url`.
 *
 * @param selector The CSS selector the user picked.
 * @param url The URL of the frame the element was picked in.
 */
- (void)addCosmeticFilter:(NSString*)selector
                   forURL:(NSURL*)url NS_SWIFT_NAME(addCosmeticFilter(_:for:));

/**
 * Presents the UI for editing the user's custom filters.
 */
- (void)manageCustomFilters;

/**
 * Gets the theme the element picker should render itself with.
 *
 * @param completion A block invoked with whether dark mode is active and the
 * RGB background colour to use.
 */
- (void)elementPickerThemeInfoWithCompletion:
    (void (^)(BOOL isDarkModeEnabled, int32_t backgroundColor))completion
    NS_SWIFT_NAME(elementPickerThemeInfo(completion:));

/**
 * Gets the localized button labels for the element picker UI.
 *
 * @param completion A block invoked with the labels, or nil if they could not
 * be resolved.
 */
- (void)elementPickerLocalizedTextsWithCompletion:
    (void (^)(ElementPickerLocalizedTexts* _Nullable))completion
    NS_SWIFT_NAME(elementPickerLocalizedTexts(completion:));

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_TAB_HELPER_BRIDGE_H_
