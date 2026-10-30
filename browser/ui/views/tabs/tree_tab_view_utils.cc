/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/tree_tab_view_utils.h"

#include <algorithm>
#include <memory>
#include <variant>

#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"

namespace tree_tab_view_utils {

namespace {

void AppendChildrenForView(const tabs::TabCollection& collection,
                           tabs::ConstChildrenPtrs& out) {
  for (const auto& child : collection.GetChildren()) {
    if (const auto* child_collection =
            std::get_if<std::unique_ptr<tabs::TabCollection>>(&child)) {
      const tabs::TabCollection* ptr = child_collection->get();
      if (ptr->type() == tabs::TabCollection::Type::TREE_NODE) {
        AppendChildrenForView(*ptr, out);
      } else {
        out.emplace_back(ptr);
      }
    } else {
      out.emplace_back(static_cast<const tabs::TabInterface*>(
          std::get<tabs::ScopedTab>(child).get()));
    }
  }
}

}  // namespace

tabs::ConstChildrenPtrs GetChildrenForView(
    const tabs::TabCollection& collection) {
  tabs::ConstChildrenPtrs children;
  AppendChildrenForView(collection, children);
  return children;
}

bool HasTreeTabNodeChild(const tabs::TabCollection& collection) {
  return std::ranges::any_of(collection.GetChildren(), [](const auto& child) {
    const auto* child_collection =
        std::get_if<std::unique_ptr<tabs::TabCollection>>(&child);
    return child_collection &&
           (*child_collection)->type() == tabs::TabCollection::Type::TREE_NODE;
  });
}

}  // namespace tree_tab_view_utils
