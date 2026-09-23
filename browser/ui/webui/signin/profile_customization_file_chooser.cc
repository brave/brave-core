// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <utility>

#include "base/feature_list.h"
#include "base/memory/scoped_refptr.h"
#include "brave/ui/webui/custom_profile_image/buildflags/buildflags.h"
#include "chrome/browser/file_select_helper.h"
#include "chrome/browser/ui/webui/signin/signin_url_utils.h"
#include "chrome/common/webui_url_constants.h"
#include "components/signin/public/base/signin_buildflags.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom-forward.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_CUSTOM_PROFILE_IMAGE)
#include "brave/browser/ui/webui/custom_profile_image/features.h"
#endif  // BUILDFLAG(ENABLE_CUSTOM_PROFILE_IMAGE)

namespace brave {

bool MaybeRunProfileCustomizationFileChooser(
    content::WebContents* web_contents,
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const blink::mojom::FileChooserParams& params) {
#if BUILDFLAG(ENABLE_CUSTOM_PROFILE_IMAGE) && BUILDFLAG(ENABLE_DICE_SUPPORT)
  if (!base::FeatureList::IsEnabled(
          custom_profile_image::features::kBraveCustomProfileImage) ||
      !web_contents || !render_frame_host ||
      render_frame_host != web_contents->GetPrimaryMainFrame() ||
      render_frame_host->GetLastCommittedOrigin() !=
          url::Origin::Create(GURL(chrome::kChromeUIProfileCustomizationURL)) ||
      GetProfileCustomizationStyle(render_frame_host->GetLastCommittedURL()) !=
          ProfileCustomizationStyle::kLocalProfileCreation) {
    return false;
  }

  FileSelectHelper::RunFileChooser(render_frame_host, std::move(listener),
                                   params);
  return true;
#else
  return false;
#endif  // BUILDFLAG(ENABLE_CUSTOM_PROFILE_IMAGE) &&
        // BUILDFLAG(ENABLE_DICE_SUPPORT)
}

}  // namespace brave
