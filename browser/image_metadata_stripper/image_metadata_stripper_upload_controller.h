/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_UPLOAD_CONTROLLER_H_
#define BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_UPLOAD_CONTROLLER_H_

#include <optional>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}  // namespace content

namespace brave {

// Per-tab owner of the temporary root directory that holds metadata-free
// copies of files handed to a page (upload / drop). One root per tab; each
// copy lives in a numbered subdirectory so the original basename is kept.
//
// The root is created on a blocking thread only after
// `IsSupportedImagePath` and `ContainsMetadataToStrip` say a strip will be
// attempted. The directory is deleted when this object is destroyed (tab
// close).
class ImageMetadataStripperUploadController
    : public content::WebContentsObserver {
 public:
  // The controller for the tab that owns |web_contents|, or null when
  // |web_contents| is not a tab.
  // This is lazily called from the upload clients.
  static ImageMetadataStripperUploadController* From(
      content::WebContents* web_contents);

  // True when any |files| has an extension we can strip metadata from.
  // Empty paths are skipped.
  static bool ContainsSupportedImage(const std::vector<base::FilePath>& files);

  explicit ImageMetadataStripperUploadController(
      content::WebContents* web_contents);
  ~ImageMetadataStripperUploadController() override;

  ImageMetadataStripperUploadController(
      const ImageMetadataStripperUploadController&) = delete;
  ImageMetadataStripperUploadController& operator=(
      const ImageMetadataStripperUploadController&) = delete;

  using StrippedCopiesCallback =
      base::OnceCallback<void(std::vector<std::optional<base::FilePath>>)>;

  // For each entry in |files|, returns the stripped copy path or nullopt if
  // stripping did not occur. |client| refers to the stripping client requesting
  // the operation.
  void MaybeStrip(image_metadata_stripper::StrippingClient client,
                  std::vector<base::FilePath> files,
                  StrippedCopiesCallback callback);

  // Empty until the first strip that created the root.
  base::FilePath GetTempRootDirForTesting() const;  // IN-TEST

 private:
  struct RootState : public base::RefCountedThreadSafe<RootState> {
    RootState();

    base::ScopedTempDir root;
    size_t next_index = 0;

   private:
    friend class base::RefCountedThreadSafe<RootState>;
    ~RootState();
  };

  struct StripResult {
    std::vector<std::optional<base::FilePath>> copies;
    base::FilePath temp_root_dir;
  };

  static StripResult StripOnBlockingThread(
      scoped_refptr<RootState> state,
      image_metadata_stripper::StrippingClient client,
      std::vector<base::FilePath> files);

  void OnStripComplete(StrippedCopiesCallback callback, StripResult result);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  scoped_refptr<RootState> state_;
  // Cached on the UI thread after the first successful strip. Ownership of
  // the directory lives in `state_`, not here.
  base::FilePath temp_root_dir_for_testing_;
  base::WeakPtrFactory<ImageMetadataStripperUploadController> weak_factory_{
      this};
};

}  // namespace brave

#endif  // BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_IMAGE_METADATA_STRIPPER_UPLOAD_CONTROLLER_H_
