/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/link_loader.h"

#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/html/html_link_element.h"

#define initiator_info                                                     \
  initiator_info.dom_node_id = DOMNodeIds::IdForNode(client_->GetOwner()); \
  options.initiator_info

#include "src/third_party/blink/renderer/core/loader/link_loader.cc"

#undef initiator_info
