// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_

#include "base/memory/raw_ref.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"

// Add TabCustomTitleChanged() callback to TabStripModelObserver.
// BraveBrowser will make sure that the custom title is stored in the session
// service so that it can be restored even after browser restarts.
#define TabPinnedStateChanged                                              \
  TabCustomTitleChanged(content::WebContents* contents,                    \
                        const std::optional<std::string>& custom_title) {} \
  virtual void TabPinnedStateChanged

// Add TreeTabChange type to represent changes in the tree tab structure.
struct TreeTabChange {
  enum Type {
    kNodeCreated,
    kNodeWillBeDestroyed,
  };

  struct Delta {
    virtual ~Delta() = default;
  };

  struct CreatedChange : public Delta {
    explicit CreatedChange(const tabs::TreeTabNode& node);
    ~CreatedChange() override;

    raw_ref<const tabs::TreeTabNode> node;
  };

  struct WillBeDestroyedChange : public Delta {
    explicit WillBeDestroyedChange(const tabs::TreeTabNode& node);
    ~WillBeDestroyedChange() override;

    raw_ref<const tabs::TreeTabNode> node;
  };

  TreeTabChange(Type type,
                tree_tab::TreeTabNodeId id,
                std::unique_ptr<Delta> delta);
  TreeTabChange(tree_tab::TreeTabNodeId id,
                const CreatedChange& created_change);
  TreeTabChange(tree_tab::TreeTabNodeId id,
                const WillBeDestroyedChange& destroyed_change);
  TreeTabChange(const TreeTabChange& other) = delete;
  TreeTabChange& operator=(const TreeTabChange& other) = delete;
  ~TreeTabChange();

  const CreatedChange& GetCreatedChange() const;
  const WillBeDestroyedChange& GetWillBeDestroyedChange() const;

  Type type;
  tree_tab::TreeTabNodeId id;
  std::unique_ptr<Delta> delta;
};

// Add OnTreeTabChange() callback to TabStripModelObserver.
#define OnTabGroupChanged(...)    \
  OnTabGroupChanged(__VA_ARGS__); \
  virtual void OnTreeTabChanged(const TreeTabChange& change)

#include <chrome/browser/ui/tabs/tab_strip_model_observer.h>  // IWYU pragma: export

#undef OnTabGroupChanged
#undef TabPinnedStateChanged

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_
