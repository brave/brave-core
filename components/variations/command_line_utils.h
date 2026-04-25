// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_VARIATIONS_COMMAND_LINE_UTILS_H_
#define BRAVE_COMPONENTS_VARIATIONS_COMMAND_LINE_UTILS_H_

namespace base {
class CommandLine;
}

namespace variations {

// Appends Brave-specific command line options to fetch variations seed from the
// correct server.
void AppendBraveCommandLineOptions(base::CommandLine& command_line);

}  // namespace variations

#endif  // BRAVE_COMPONENTS_VARIATIONS_COMMAND_LINE_UTILS_H_
