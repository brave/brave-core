// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_UI_H_
#define BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_UI_H_

namespace autofill {

// The type of IBAN bubble to show.
enum class ConfirmAutocompleteBubbleType {
  // There is no bubble to show anymore. This also indicates that the icon
  // should not be visible.
  kInactive = 0,

  // Save prompt when the user is saving locally.
  kLocalSave = 1,
};

}  // namespace autofill

#endif  // BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_UI_H_
