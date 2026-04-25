// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web_view/public/cwv_password_extras.h"

#include "base/strings/sys_string_conversions.h"
#include "components/password_manager/core/browser/password_form.h"
#include "ios/web_view/internal/passwords/cwv_password_internal.h"

@implementation CWVPassword (Extras)

- (NSString*)identifier {
  const auto* form = self.internalPasswordForm;
  // Create a stable identifier from the PasswordFormUniqueKey components:
  // (signon_realm, url, username_element, username_value, password_element)
  NSString* signonRealm = base::SysUTF8ToNSString(form->signon_realm);
  NSString* url = base::SysUTF8ToNSString(form->url.spec());
  NSString* usernameElement = base::SysUTF16ToNSString(form->username_element);
  NSString* usernameValue = base::SysUTF16ToNSString(form->username_value);
  NSString* passwordElement = base::SysUTF16ToNSString(form->password_element);

  return [NSString stringWithFormat:@"%@|%@|%@|%@|%@", signonRealm, url,
                                    usernameElement, usernameValue,
                                    passwordElement];
}

@end
