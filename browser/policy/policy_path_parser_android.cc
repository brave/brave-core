/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/policy/policy_path_parser.h"

namespace policy {

namespace path_parser {

base::FilePath::StringType ExpandPathVariables(
    const base::FilePath::StringType& untranslated_string) {
  return base::FilePath::StringType(untranslated_string);
}

}  // namespace path_parser

}  // namespace policy
