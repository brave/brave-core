/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CONTENT_BROWSER_CONTENT_AUTOFILL_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CONTENT_BROWSER_CONTENT_AUTOFILL_CLIENT_H_

#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/content/common/mojom/autofill_agent.mojom.h"
#include "components/autofill/core/browser/foundations/autofill_client.h"

namespace autofill {

// This struct is only needed to add _Unused methods. These methods are needed
// because ChromeAutofillClient which derives from ContentAutofillClient marks
// several methods as final and we want to override them by re-defining them as
// _Unused and adding our own normally-named methods. But, since the _Unused
// methods would still be marked as final they need to be virtual in a base
// class - which is what we are providing here.
struct BraveContentAutofillClientUnused {
  virtual AutofillOptimizationGuide* GetAutofillOptimizationGuide_Unused()
      const;
  virtual bool IsAutofillEnabled_Unused() const;
  virtual bool IsAutocompleteEnabled_Unused() const;
};

}  // namespace autofill

#define AutofillClient BraveContentAutofillClientUnused, public AutofillClient
#include "src/components/autofill/content/browser/content_autofill_client.h"  // IWYU pragma: export
#undef AutofillClient

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CONTENT_BROWSER_CONTENT_AUTOFILL_CLIENT_H_
