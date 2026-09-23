// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web_view/public/cwv_autofill_profile_extras.h"

#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/browser/data_model/addresses/autofill_profile.h"
#include "ios/web_view/internal/autofill/cwv_autofill_profile_internal.h"

@implementation CWVAutofillProfile (Extras)

- (NSString*)identifier {
  return base::SysUTF8ToNSString(self.internalProfile->guid());
}

@end
