/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/variations/seed_reader_writer.h"

#include <components/variations/seed_reader_writer.cc>

namespace variations {

void SeedReaderWriter::SetSessionCountry(std::string_view country_code) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (ShouldUseSeedFile()) {
    stored_seed_info_.set_session_country_code(country_code);
  }
  local_state_->SetString(fields_prefs_->session_country_code, country_code);
}

}  // namespace variations
