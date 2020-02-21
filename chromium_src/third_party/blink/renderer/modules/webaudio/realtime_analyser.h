/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_REALTIME_ANALYSER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_REALTIME_ANALYSER_H_

// RealtimeAnalyser doesn't have access to the owning Document
// so it needs its own copy of the fudge factor.
#define BRAVE_REALTIMEANALYSER_H double fudge_factor_;

#include "../../../../../../third_party/blink/renderer/modules/webaudio/realtime_analyser.h"

#undef BRAVE_REALTIMEANALYSER_H

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_REALTIME_ANALYSER_H_
