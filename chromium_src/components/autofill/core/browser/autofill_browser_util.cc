/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define IsFormMixedContent IsFormMixedContent_ChromiumImpl
#include "src/components/autofill/core/browser/autofill_browser_util.cc"
#undef IsFormMixedContent

#include "net/base/url_util.h"

namespace autofill {

bool IsFormMixedContent(const AutofillClient& client, const FormData& form) {
  if (IsFormMixedContent_ChromiumImpl(client, form)) {
    return true;
  }

  return net::IsOnion(client.GetLastCommittedPrimaryMainFrameOrigin()) &&
         (form.action().is_valid() &&
          security_interstitials::IsInsecureFormAction(form.action()));
}

}  // namespace autofill
