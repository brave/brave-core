/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#if defined(CHROMIUM_BUILD) && !defined(STANDALONE_BUILD)
#include "base/logging.h"
#else
#include <cassert>
#include <iostream>

#define DCHECK assert
#define LOG(LEVEL) std::cerr << std::endl << #LEVEL << ": "
#endif
