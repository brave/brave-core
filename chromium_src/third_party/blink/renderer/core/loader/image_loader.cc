/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/image_loader.h"

#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/frame/ad_tracker.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"

#define initiator_info                                              \
  initiator_info.dom_node_id = DOMNodeIds::IdForNode(GetElement()); \
  resource_loader_options.initiator_info

#include "src/third_party/blink/renderer/core/loader/image_loader.cc"

#undef initiator_info
