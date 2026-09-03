/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"

namespace base {
class FilePath;
}  // namespace base

namespace brave {

// The upload-metadata strip is spliced into NotifyListenerAndEnd by
// rewrite/chrome/browser/file_select_helper.cc.yaml. Defined in
// brave/browser/file_select/brave_file_select_image_metadata_stripper.cc, and
// declared here so that chrome/browser does not depend on Brave targets.
bool MaybeStripImageMetadataForUpload(
    bool& already_processed,
    std::vector<base::FilePath>& temporary_files,
    std::vector<blink::mojom::FileChooserFileInfoPtr>& list,
    base::OnceCallback<void(std::vector<blink::mojom::FileChooserFileInfoPtr>)>
        notify);

}  // namespace brave

#include <chrome/browser/file_select_helper.cc>
