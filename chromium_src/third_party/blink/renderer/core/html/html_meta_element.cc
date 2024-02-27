/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/html/html_meta_element.h"

#include "third_party/blink/renderer/core/frame/local_frame.h"

#define ScriptEnabled() ScriptEnabled(document.Url())

#include "src/third_party/blink/renderer/core/html/html_meta_element.cc"
#undef ScriptEnabled
