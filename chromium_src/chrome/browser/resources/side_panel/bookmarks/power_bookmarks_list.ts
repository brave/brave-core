// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { RegisterPolymerPrototypeModification } from '//resources/brave/polymer_overriding.js'
import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'
import { PowerBookmarksService } from './power_bookmarks_service.js';
import type { BookmarksTreeNode } from './bookmarks.mojom-webui.js';

import { PowerBookmarksListElement } from './power_bookmarks_list-chromium.js'

injectStyle(PowerBookmarksListElement, css`
  :host {
    --iron-icon-width: 16px;
  }

  .new-folder-row {
    color: var(--leo-color-text-interactive);
  }

  .new-folder-icon-container {
    --iron-icon-fill-color: var(--leo-color-icon-interactive);

    border: 1px solid var(--leo-color-divider-interactive);
    border-radius: var(--leo-radius-s);
  }
`)

const originalSortBookmarks = PowerBookmarksService.prototype.sortBookmarks
PowerBookmarksService.prototype.sortBookmarks = function (
  bookmarks: BookmarksTreeNode[],
  activeSortIndex: number): boolean {
  if (activeSortIndex === /* custom order */5) {
    return false
  }
  return originalSortBookmarks.apply(this, [bookmarks, activeSortIndex])
}

RegisterPolymerPrototypeModification({
  'power-bookmarks-list': prototype => {
    const originalOnBookmarkMoved = prototype.onBookmarkMoved
    prototype.onBookmarkMoved = function (
      bookmark: BookmarksTreeNode,
      oldParent: BookmarksTreeNode,
      newParent: BookmarksTreeNode) {
      originalOnBookmarkMoved.apply(this, [bookmark, oldParent, newParent])
      const shouldShow = prototype.bookmarkShouldShow_.apply(this, [bookmark]);
      // Update if currently visible item is moved in the same directory,
      // Upstream doesn't update in this situation because they don't support
      // custom order. Moving in same direcotry doesn't affect with upstream's
      // sort orders.
      if (oldParent === newParent && shouldShow &&
          this.activeSortIndex === /* customOrder */5) {
        prototype.updateDisplayList_.apply(this)
      }
    }
  }
})

export * from './power_bookmarks_list-chromium.js'
customElements.define(PowerBookmarksListElement.is, PowerBookmarksListElement);
