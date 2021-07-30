/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_PROFILE_WRITER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_PROFILE_WRITER_H_

// Extends ProfileWriter by file overriding instead of subclassing because there
// are many places that instantiate it.
#define AddAutofillFormDataEntries                           \
  AddCreditCard(const std::u16string& name_on_card,          \
                const std::u16string& expiration_month,      \
                const std::u16string& expiration_year,       \
                const std::u16string& decrypted_card_number, \
                const std::string& origin);                  \
  virtual void AddAutofillFormDataEntries

#include "../../../../../chrome/browser/importer/profile_writer.h"

#undef AddAutofillFormDataEntries

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_PROFILE_WRITER_H_
