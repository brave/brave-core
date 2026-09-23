/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/blink_probe_types.h"

#include "base/json/json_writer.h"

namespace blink {

std::string PageGraphValueToString(base::ValueView args) {
  return base::WriteJson(args).value_or("\"__pg_write_json_failed__\"");
}

}  // namespace blink
