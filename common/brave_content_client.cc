/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_content_client.h"

#include <optional>
#include <string>

#include "base/check.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/version.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/constants/url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/flags_ui_resources.h"
#include "content/public/common/url_constants.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/base/resource/resource_bundle.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/constants.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_WIDEVINE) && BUILDFLAG(IS_LINUX)
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/media/component_widevine_cdm_hint_file_linux.h"
#endif

namespace {

#if BUILDFLAG(ENABLE_WIDEVINE) && BUILDFLAG(IS_LINUX)
// Hint file is used to give where widevine cdm library is installed.
// On linux, zygote loads widevine before sandbox is initialized.
// So, it should know where widevine is installed.
// We will create hint file always if there is no hint file.
// and that initial hint file will indicates widevine is installed to
// user-dir/Widevine directory and it'll always work well.
// If user don't install widevine yet, nothing is loaded.
// If user installs old widevine, it's installed in user-dir/Widevine.
// Then, newer version could be installed via component. In that case, hint
// file will be updated to point to newer version. That new version will be
// loaded after re-launching.
void CreateDefaultWidevineCdmHintFile() {
  base::FilePath hint_file_path;
  CHECK(base::PathService::Get(chrome::FILE_COMPONENT_WIDEVINE_CDM_HINT,
                               &hint_file_path));
  if (base::PathExists(hint_file_path)) {
    return;
  }

  base::FilePath widevine_root_dir_path;
  CHECK(base::PathService::Get(chrome::DIR_COMPONENT_UPDATED_WIDEVINE_CDM,
                               &widevine_root_dir_path));

  if (!base::CreateDirectory(widevine_root_dir_path)) {
    DVLOG(2) << __func__ << " : Failed to create widevine dir";
    return;
  }

  DCHECK(UpdateWidevineCdmHintFile(widevine_root_dir_path,
                                   /*bundled_version=*/std::nullopt));
}
#endif

}  // namespace

BraveContentClient::BraveContentClient() = default;

BraveContentClient::~BraveContentClient() = default;

scoped_refptr<base::RefCountedMemory> BraveContentClient::GetDataResourceBytes(
    int resource_id) {
  if (resource_id == IDR_FLAGS_UI_APP_JS) {
    const ui::ResourceBundle& resource_bundle =
        ui::ResourceBundle::GetSharedInstance();
    const std::string flags_js =
        resource_bundle.LoadDataResourceString(resource_id) +
        resource_bundle.LoadDataResourceString(
            IDR_FLAGS_UI_BRAVE_FLAGS_OVERRIDES_JS);
    auto bytes = base::MakeRefCounted<base::RefCountedString>();
    bytes->as_string().assign(flags_js.data(), flags_js.length());
    return bytes;
  }
  return ChromeContentClient::GetDataResourceBytes(resource_id);
}

void BraveContentClient::AddAdditionalSchemes(Schemes* schemes) {
  ChromeContentClient::AddAdditionalSchemes(schemes);
  schemes->standard_schemes.push_back(content::kBraveUIScheme);
  schemes->secure_schemes.push_back(content::kBraveUIScheme);
  schemes->cors_enabled_schemes.push_back(content::kBraveUIScheme);
  schemes->savable_schemes.push_back(content::kBraveUIScheme);

#if BUILDFLAG(ENABLE_AI_CHAT)
  // Leo workspace previews. Runs in every process before any URL is parsed;
  // otherwise these would parse as non-standard and land in opaque origins.
  //
  // |standard| makes each uuid an origin, |secure| makes those origins
  // trustworthy (without it there is no storage, and previews count as mixed
  // content when embedded).
  //
  // Deliberately not |cors_enabled|, so previews cannot issue fetch()/XHR at
  // all rather than being refused by connect-src; and not |service_worker|,
  // since such a worker would be registered by model-authored script.
  //
  // Being standard also means GURL canonicalises the path the way it does for
  // http(s): backslashes become slashes and dot segments are collapsed before
  // any of our code sees the URL.
  schemes->standard_schemes.push_back(ai_chat::kLeoWorkspaceContentScheme);
  schemes->secure_schemes.push_back(ai_chat::kLeoWorkspaceContentScheme);
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
}

void BraveContentClient::AddContentDecryptionModules(
    std::vector<content::CdmInfo>* cdms,
    std::vector<media::CdmHostFilePath>* cdm_host_file_paths) {
#if BUILDFLAG(ENABLE_WIDEVINE) && BUILDFLAG(IS_LINUX)
  CreateDefaultWidevineCdmHintFile();
#endif

  ChromeContentClient::AddContentDecryptionModules(cdms, cdm_host_file_paths);
}
