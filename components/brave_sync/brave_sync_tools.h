/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_TOOLS_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_TOOLS_H_

#include <string>

namespace brave_sync {

namespace tools {

std::string GenerateObjectId();
std::string replaceUnsupportedCharacters(const std::string &in);

} // namespace tools

} // namespace brave_sync

#endif // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_TOOLS_H_
