// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_COMMANDER_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_COMMANDER_COMMON_CONSTANTS_H_

#include <iterator>

#include "base/component_export.h"
#include "base/strings/string_piece.h"

namespace commander {

COMPONENT_EXPORT(COMMANDER_COMMON)
extern const base::StringPiece16 kCommandPrefix;

}  // namespace commander

#endif  // BRAVE_COMPONENTS_COMMANDER_COMMON_CONSTANTS_H_
