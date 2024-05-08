/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/search_engines/search_engine_choice/search_engine_choice_utils.h"

#define IsChoiceScreenFlagEnabled IsChoiceScreenFlagEnabled_ChromiumImpl
#include "src/components/search_engines/search_engine_choice/search_engine_choice_utils.cc"
#undef IsChoiceScreenFlagEnabled

namespace search_engines {

bool IsChoiceScreenFlagEnabled(ChoicePromo promo) {
  return false;
}

}  // namespace search_engines
