// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

#include <chrome/browser/ui/tabs/tab_strip_model_observer.cc>

TreeTabChange::CreatedChange::CreatedChange(const tabs::TreeTabNode& node)
    : node(node) {}

TreeTabChange::CreatedChange::~CreatedChange() = default;

TreeTabChange::DestroyedChange::DestroyedChange() = default;

TreeTabChange::DestroyedChange::~DestroyedChange() = default;

TreeTabChange::TreeTabChange(Type type,
                             tree_tab::TreeTabNodeId id,
                             std::unique_ptr<Delta> delta)
    : type(type), id(id), delta(std::move(delta)) {}

TreeTabChange::TreeTabChange(tree_tab::TreeTabNodeId id,
                             const CreatedChange& created_change)
    : TreeTabChange(Type::kNodeCreated,
                    id,
                    std::make_unique<CreatedChange>(created_change)) {}

TreeTabChange::TreeTabChange(tree_tab::TreeTabNodeId id,
                             const DestroyedChange& destroyed_change)
    : TreeTabChange(Type::kNodeDestroyed,
                    id,
                    std::make_unique<DestroyedChange>(destroyed_change)) {}

TreeTabChange::~TreeTabChange() = default;

const TreeTabChange::CreatedChange& TreeTabChange::GetCreatedChange() const {
  CHECK_EQ(type, Type::kNodeCreated);
  return *static_cast<CreatedChange*>(delta.get());
}

const TreeTabChange::DestroyedChange& TreeTabChange::GetDestroyedChange()
    const {
  CHECK_EQ(type, Type::kNodeDestroyed);
  return *static_cast<DestroyedChange*>(delta.get());
}

void TabStripModelObserver::OnTreeTabChanged(const TreeTabChange& change) {}
