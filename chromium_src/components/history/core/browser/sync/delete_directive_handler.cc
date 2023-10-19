/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/history/core/browser/sync/delete_directive_handler.h"

#define CreateUrlDeleteDirective CreateUrlDeleteDirective_ChromiumImpl
#include "src/components/history/core/browser/sync/delete_directive_handler.cc"
#undef CreateUrlDeleteDirective

namespace history {

bool DeleteDirectiveHandler::CreateUrlDeleteDirective(const GURL& url) {
  return false;
}

}  // namespace history
