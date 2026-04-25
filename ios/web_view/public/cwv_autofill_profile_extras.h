// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_PROFILE_EXTRAS_H_
#define BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_PROFILE_EXTRAS_H_

#import <Foundation/Foundation.h>

#include "cwv_autofill_profile.h"  // NOLINT

NS_ASSUME_NONNULL_BEGIN

/// Exposes additional properties that may exist on autofill::AutofillProfile
/// but not on CWVAutofillProfile
@interface CWVAutofillProfile (Extras)

@property(readonly) NSString* identifier;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_AUTOFILL_PROFILE_EXTRAS_H_
