/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"
#include "ui/shell_dialogs/select_file_dialog.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_file_select_utils.h"
#endif

namespace {

std::u16string GetTitle(content::RenderFrameHost* render_frame_host,
                        const std::u16string& original_title) {
#if BUILDFLAG(IS_ANDROID)
  return original_title;
#else
  return brave::GetFileSelectTitle(
      content::WebContents::FromRenderFrameHost(render_frame_host),
      render_frame_host->GetLastCommittedOrigin(),
      brave::FileSelectTitleType::kSave);
#endif
}

}  // namespace

// Override title of the file select dialog for downloads.
#define SelectFile(type, title, default_path, file_types, file_type_index,  \
                   default_extension, owning_window, caller)                \
  SelectFile(type, GetTitle(render_frame_host, title), default_path,        \
             file_types, file_type_index, default_extension, owning_window, \
             caller)

#include "src/chrome/browser/download/download_file_picker.cc"

#undef SelectFile
