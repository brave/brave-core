// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_STRIP_COLLECTION_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_STRIP_COLLECTION_H_

#define AddTabRecursive virtual AddTabRecursive

#include <components/tabs/public/tab_strip_collection.h>  // IWYU pragma: export

#undef AddTabRecursive

namespace tabs {

#if !BUILDFLAG(IS_ANDROID)
// BraveTabStripCollection is used to override methods for tree tabs.
// * In AddTabRecursive(), we need to to create TreeTabNode with a given new tab
//   and insert it into target collection. The target collection can be another
//   TreeTabNode in case new tab has opener relationship with the previous tab.
//   Otherwise, the target collection can be anything but pinned collection.
class BraveTabStripCollection : public TabStripCollection {
 public:
  using TabStripCollection::TabStripCollection;
  ~BraveTabStripCollection() override = default;

  void set_in_tree_tab_mode(bool in_tree_tab_mode) {
    in_tree_tab_mode_ = in_tree_tab_mode;
  }
  bool in_tree_tab_mode() const { return in_tree_tab_mode_; }

  // TabStripCollection:
  void AddTabRecursive(std::unique_ptr<TabInterface> tab,
                       size_t index,
                       std::optional<tab_groups::TabGroupId> new_group_id,
                       bool new_pinned_state) override;

 private:
  bool in_tree_tab_mode_ = false;
};
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace tabs

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_STRIP_COLLECTION_H_
