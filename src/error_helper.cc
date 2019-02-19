/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "error_helper.h"

namespace helper {

const std::string GetDescription(const ads::Result result) {
  std::string description = "";

  switch (result) {
    case ads::Result::SUCCESS:
      description = "Successful";
      break;

    case ads::Result::FAILED:
      description = "Failed";
      break;
  }

  return description;
}

}  // namespace helper
