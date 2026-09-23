/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DRAG_DROP_BRAVE_DRAG_DROP_IMAGE_METADATA_STRIPPER_H_
#define BRAVE_BROWSER_DRAG_DROP_BRAVE_DRAG_DROP_IMAGE_METADATA_STRIPPER_H_

#include <vector>

#include "base/files/file_path.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "content/public/browser/web_contents_view_delegate.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

namespace content {
class WebContents;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace brave {

// Holds stripped image copies handed to a drop target. The `File` the page
// receives is backed by the copy on disk, so the copy has to outlive the drop.
// Owned by tab features so it follows the tab across discards. Copies are
// deleted when the contents are discarded.
class DropStripTempDirs : public tabs::ContentsObservingTabFeature {
 public:
  explicit DropStripTempDirs(tabs::TabInterface& tab);
  ~DropStripTempDirs() override;

  DropStripTempDirs(const DropStripTempDirs&) = delete;
  DropStripTempDirs& operator=(const DropStripTempDirs&) = delete;

  DECLARE_USER_DATA(DropStripTempDirs);

  static DropStripTempDirs* From(tabs::TabInterface* tab);

  void Add(base::FilePath temp_root_dir);

 private:
  // tabs::ContentsObservingTabFeature:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  std::vector<base::FilePath> temp_root_dirs_;
  ui::ScopedUnownedUserData<DropStripTempDirs> scoped_unowned_user_data_;
};

// Wraps |callback| which is the completion callback `HandleOnPerformingDrop`
// runs to hand the dropped files back to the page. We wrap it so as to strip
// out the metadata from the images before running the original
// |callback|. Returns |callback| unchanged when the feature is off.
//
// As with the upload flow, the dropped files themselves are never modified.
// Each strippable jpeg is copied into a unique subdirectory of one temporary
// parent directory (so the original basename is preserved) and
// `DropData::filenames` is pointed at the copy. The page holds those copies for
// as long as it holds the `File`, so the parent directory is owned by the tab's
// `DropStripTempDirs` and deleted once that tab is discarded or destroyed.
content::WebContentsViewDelegate::DropCompletionCallback
MaybeStripImageMetadataForDrop(
    content::WebContents* web_contents,
    content::WebContentsViewDelegate::DropCompletionCallback callback);

}  // namespace brave

#endif  // BRAVE_BROWSER_DRAG_DROP_BRAVE_DRAG_DROP_IMAGE_METADATA_STRIPPER_H_
