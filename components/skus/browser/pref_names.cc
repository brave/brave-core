/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/pref_names.h"

namespace skus {
namespace prefs {

// Dictionary storage for the SKU SDK. For example, account.brave.com
// stores SKU key/value pairs in local storage.
const char kSkusState[] = "skus.state";

}  // namespace prefs
}  // namespace skus
