/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_HELPER_PLATFORM_H_
#define BRAVELEDGER_BAT_HELPER_PLATFORM_H_

#include <string>
#if defined(CHROMIUM_BUILD) && !defined(STANDALONE_BUILD)
#else
// Chromium debug macros redefined
//TODO: implement!
#include <cassert>

#define DCHECK assert
#endif

#endif  //BRAVELEDGER_BAT_HELPER_PLATFORM_H_
