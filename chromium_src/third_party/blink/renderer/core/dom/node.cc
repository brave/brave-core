/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/dom/node.cc"

namespace blink {

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
void Node::NodeConstructed() {
  // Document is required for probe sink.
  if (tree_scope_)
    probe::RegisterPageGraphNodeFullyCreated(this);
}
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

}  // namespace blink
