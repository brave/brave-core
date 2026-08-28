/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_FILE_SELECT_BRAVE_FILE_SELECT_IMAGE_METADATA_STRIPPER_H_
#define BRAVE_BROWSER_FILE_SELECT_BRAVE_FILE_SELECT_IMAGE_METADATA_STRIPPER_H_

#include <vector>

#include "base/functional/callback_forward.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"

namespace base {
class FilePath;
}  // namespace base

namespace brave {

// Strips tracking metadata (e.g. Facebook FBMD IPTC identifiers) from the JPEG
// files a user picked for upload. This is the file-picker counterpart to the
// download-completion strip in BraveDownloadManagerDelegate.
//
// The user's original files are never modified: each strippable image is copied
// to a temporary file which is then scrubbed, and the corresponding entry
// in |list| is rewritten to point at the sanitized copy (its display name is
// left unchanged, so the site still sees the original filename). Any temporary
// copies created are appended to `temporary_files`, so the existing
// FileSelectHelper teardown deletes them once the upload completes.
//
// Returns true when stripping has been started asynchronously and the caller
// took no ownership decision: the interposition now owns `list` and will run
// `notify` on the UI thread with the sanitized list once stripping finishes.
// The caller must return immediately without notifying its listener.
//
// Returns false when there is nothing to do (feature disabled, already run, or
// no strippable image selected); |list| is left untouched and the caller should
// proceed to notify it synchronously as usual.
//
// |already_processed| guards against re-entrancy: the caller is expected to be
// re-invoked with the sanitized list via |notify| callback, and this flag lets
// the strip run exactly once before falling through to the upstream path.
bool MaybeStripImageMetadataForUpload(
    bool& already_processed,
    std::vector<base::FilePath>& temporary_files,
    std::vector<blink::mojom::FileChooserFileInfoPtr>& list,
    base::OnceCallback<void(std::vector<blink::mojom::FileChooserFileInfoPtr>)>
        notify);

}  // namespace brave

#endif  // BRAVE_BROWSER_FILE_SELECT_BRAVE_FILE_SELECT_IMAGE_METADATA_STRIPPER_H_
