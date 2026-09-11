/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_FILE_SELECT_BRAVE_FILE_SELECT_IMAGE_METADATA_STRIPPER_H_
#define BRAVE_BROWSER_FILE_SELECT_BRAVE_FILE_SELECT_IMAGE_METADATA_STRIPPER_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom-forward.h"

namespace brave {

// Prefix passed to CreateUniqueTempDir for stripped upload copies.
inline constexpr base::FilePath::CharType kUploadStripTempDirPrefix[] =
    FILE_PATH_LITERAL("brave_upload_strip");

// A method which is called when the browser is about to hand over the selected
// files via `NotifyListenerAndEnd`. This method is responsible for stripping
// out the flagged metadata from the uploaded jpeg image.
//
// For cases where it's clear no metadata needs to be stripped it returns false
// to continue with the synchronous execution of `NotifyListenerAndEnd` path;
// and true otherwise where it runs the metadata removal and call the
// `NotifyListenerAndEnd` asynchronously via |notify| callback to join back with
// the regular execution.
//
// The method does not strip metadata from the original file. It copies each
// strippable JPEG into a unique subdirectory of one temporary parent directory
// (so the original basename is preserved) and that copy is what gets uploaded.
// Only the parent directory is added to |temporary_files|;
// `MaybeDeleteImageMetadataStripperTemporaryDir` recursively deletes it
// after the upload completes.
//
// |already_processed| helps to avoid looping between `NotifyListenerAndEnd`
// and `MaybeStripImageMetadataForUpload` by letting `NotifyListenerAndEnd` know
// that the stripping work has been scheduled.
//
// Note on the the |list| ownership. For cases where this method returns true
// its ownership would be taken via move, but re-transferred back via the
// |notify| callback. For false, regular execution would follow as the |list|
// ownership would still be with the caller.
bool MaybeStripImageMetadataForUpload(
    bool& already_processed,
    std::vector<base::FilePath>& temporary_files,
    std::vector<blink::mojom::FileChooserFileInfoPtr>& list,
    base::OnceCallback<void(std::vector<blink::mojom::FileChooserFileInfoPtr>)>
        notify);

// Recursively deletes the upload-strip temp directory (basename contains
// `kUploadStripTempDirPrefix`) which holds the stripped copies. No-ops if
// |paths| has no matching directory.
void MaybeDeleteImageMetadataStripperTemporaryDir(
    std::vector<base::FilePath>& paths);

// Test-only: The caller owns |callback| and must keep
// it alive until it fires; pass nullptr to clear it.
void SetStripCompletedCallbackForTesting(  // IN-TEST
    base::OnceCallback<void(std::vector<base::FilePath>)>* callback);

}  // namespace brave

#endif  // BRAVE_BROWSER_FILE_SELECT_BRAVE_FILE_SELECT_IMAGE_METADATA_STRIPPER_H_
