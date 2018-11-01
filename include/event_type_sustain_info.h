/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "export.h"

namespace event_type {

ADS_EXPORT struct SustainInfo {
  std::string notification_id;
};

}  // namespace event_type
