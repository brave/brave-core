/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_VALUE_DEBUG_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_VALUE_DEBUG_H_

#include <string>

namespace base {
  class Value;
}

namespace brave {

namespace debug {

std::string ToPrintableString(const base::Value &val, const int &ident = 0);

} //namespace debug

} //namespace brave

#endif // BRAVE_COMPONENTS_BRAVE_SYNC_VALUE_DEBUG_H_
