/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_CHILD_BRAVE_RUNTIME_FEATURES_H_
#define BRAVE_CONTENT_CHILD_BRAVE_RUNTIME_FEATURES_H_

namespace base {
class CommandLine;
}  // namespace base

namespace content {

void BraveSetRuntimeFeaturesDefaultsAndUpdateFromArgs(
    const base::CommandLine& command_line);

}  // namespace content

#endif  // BRAVE_CONTENT_CHILD_BRAVE_RUNTIME_FEATURES_H_
