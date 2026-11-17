// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/common/unpinned_tab_container_view_vertical_layout.h"

#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace views {
class View;
}  // namespace views

// Tree tab hooks. Defined in brave/browser/ui/views/tabs/tree_tab_view_utils.cc
// so that this upstream target doesn't depend on Brave targets.
namespace tree_tab_view_utils {
int GetTreeTabIndent(
    const views::View* view,
    int available_width,
    absl::flat_hash_map<int, int>& offset_per_level_by_tree_height);
bool IsInCollapsedTreeTabNode(const views::View* view);
}  // namespace tree_tab_view_utils

#include <chrome/browser/ui/views/tabs/common/unpinned_tab_container_view_vertical_layout.cc>
