/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_ANALYSER_NODE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_ANALYSER_NODE_H_

namespace blink {
class Document;
}

namespace brave {
double GetFudgeFactor(blink::Document* document);
}

#include "../../../../../../third_party/blink/renderer/modules/webaudio/analyser_node.h"

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_ANALYSER_NODE_H_
