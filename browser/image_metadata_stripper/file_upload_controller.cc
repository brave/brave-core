/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/browser/image_metadata_stripper/file_upload_controller.h"

#include "base/memory/ptr_util.h"
#include "components/tabs/public/tab_interface.h"

namespace image_metadata_stripper {

DEFINE_USER_DATA(FileUploadController);

FileUploadController::FileUploadController(tabs::TabInterface& tab)
    : tabs::ContentsObservingTabFeature(tab),
      scoped_unowned_user_data_(tab.GetUnownedUserDataHost(), *this) {}

FileUploadController::~FileUploadController() = default;

// static
std::unique_ptr<FileUploadController> FileUploadController::MaybeCreate(
    tabs::TabInterface& tab) {
  // No controller must have been created at this point.
  auto* controller = Get(tab.GetUnownedUserDataHost());
  CHECK(!controller);

  return base::WrapUnique(new FileUploadController(tab));
}

// static
FileUploadController* FileUploadController::FromWebContents(
    content::WebContents* web_contents) {
  return web_contents
             ? From(tabs::TabInterface::MaybeGetFromContents(web_contents))
             : nullptr;
}

void FileUploadController::Strip(std::vector<base::FilePath> srcs,
                                 StrippingClient client,
                                 StripCallback callback) {}

// static
FileUploadController* FileUploadController::From(tabs::TabInterface* tab) {
  return tab ? Get(tab->GetUnownedUserDataHost()) : nullptr;
}

void FileUploadController::OnDiscardContents(
    tabs::TabInterface* tab,
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  tabs::ContentsObservingTabFeature::OnDiscardContents(tab, old_contents,
                                                       new_contents);
}

}  // namespace image_metadata_stripper
