/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/tree_tab_view_utils.h"

#include <functional>
#include <memory>
#include <variant>

#include "components/tabs/public/tab_collection.h"

namespace tree_tab_view_utils {

namespace {

void AppendUnwrappedChildren(
    const tabs::TabCollection& collection,
    std::vector<std::reference_wrapper<const tabs::Child>>& out) {
  for (const auto& child : collection.GetChildren()) {
    const auto* child_collection =
        std::get_if<std::unique_ptr<tabs::TabCollection>>(&child);
    if (child_collection &&
        (*child_collection)->type() == tabs::TabCollection::Type::TREE_NODE) {
      AppendUnwrappedChildren(**child_collection, out);
    } else {
      out.push_back(std::cref(child));
    }
  }
}

}  // namespace

std::vector<std::reference_wrapper<const tabs::Child>> GetUnwrappedChildren(
    const tabs::TabCollection& collection) {
  std::vector<std::reference_wrapper<const tabs::Child>> children;
  AppendUnwrappedChildren(collection, children);
  return children;
}

}  // namespace tree_tab_view_utils
