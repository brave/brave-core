/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "brave/browser/file_select/brave_file_select_image_metadata_stripper.h"

// The upload-metadata strip is spliced into
// FileSelectHelper::NotifyListenerAndEnd by
// rewrite/chrome/browser/file_select_helper.cc.yaml. The declaration above is
// all this target needs; the implementation is linked from
// //brave/browser/file_select, so chrome/browser does not depend on Brave
// targets (mirroring the download-delegate factory approach).
#include <chrome/browser/file_select_helper.cc>
