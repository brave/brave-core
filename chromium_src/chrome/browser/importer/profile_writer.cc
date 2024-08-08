/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/uuid.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "components/autofill/core/browser/personal_data_manager.h"

#include "src/chrome/browser/importer/profile_writer.cc"

void ProfileWriter::AddCreditCard(const std::u16string& name_on_card,
                                  const std::u16string& expiration_month,
                                  const std::u16string& expiration_year,
                                  const std::u16string& decrypted_card_number,
                                  const std::string& origin) {
  autofill::PersonalDataManager* personal_data =
      autofill::PersonalDataManagerFactory::GetForBrowserContext(profile_);

  autofill::CreditCard credit_card = autofill::CreditCard(
      base::Uuid::GenerateRandomV4().AsLowercaseString(), origin);

  if (!name_on_card.empty()) {
    credit_card.SetRawInfo(autofill::CREDIT_CARD_NAME_FULL,
                           name_on_card);
  }

  if (!decrypted_card_number.empty()) {
    credit_card.SetRawInfo(autofill::CREDIT_CARD_NUMBER,
                           decrypted_card_number);
  }

  if (!expiration_month.empty()) {
    credit_card.SetRawInfo(autofill::CREDIT_CARD_EXP_MONTH,
                           expiration_month);
  }

  if (!expiration_year.empty()) {
    credit_card.SetRawInfo(autofill::CREDIT_CARD_EXP_4_DIGIT_YEAR,
                           expiration_year);
  }

  personal_data->payments_data_manager().AddCreditCard(credit_card);
}

#if BUILDFLAG(IS_ANDROID)
namespace first_run {
bool IsChromeFirstRun() {
  return false;
}
}  // namespace first_run
#endif  // BUILDFLAG(IS_ANDROID)
