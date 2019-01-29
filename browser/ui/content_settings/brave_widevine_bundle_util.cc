/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_widevine_bundle_util.h"

#include "base/path_service.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_paths.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cdm_registry.h"
#include "content/public/common/cdm_info.h"
#include "media/base/decrypt_config.h"
#include "media/base/video_codecs.h"
#include "media/media_buildflags.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"
#include "widevine_cdm_version.h"

void RegisterWidevineCdmToCdmRegistry() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  // Only registers widevine cdm when user explicitly requested.
  if (!prefs->GetBoolean(kWidevineOptedIn))
    return;

  base::FilePath cdm_path;
  bool success = base::PathService::Get(chrome::FILE_WIDEVINE_CDM, &cdm_path);
  DCHECK(success);
  content::CdmCapability capability;
  const base::Version cdm_version(WIDEVINE_CDM_VERSION_STRING);

  // Add the supported codecs as if they came from the component manifest.
  // This list must match the CDM that is being bundled with Chrome.
  capability.video_codecs.push_back(media::VideoCodec::kCodecVP8);
  capability.video_codecs.push_back(media::VideoCodec::kCodecVP9);
  // TODO(xhwang): Update this and tests after Widevine CDM supports VP9
  // profile 2.
  capability.supports_vp9_profile2 = false;
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
  capability.video_codecs.push_back(media::VideoCodec::kCodecH264);
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)

  // Add the supported encryption schemes as if they came from the
  // component manifest. This list must match the CDM that is being
  // bundled with Chrome.
  capability.encryption_schemes.insert(media::EncryptionMode::kCenc);
  capability.encryption_schemes.insert(media::EncryptionMode::kCbcs);

  // Temporary session is always supported.
  capability.session_types.insert(media::CdmSessionType::kTemporary);

  content::CdmRegistry::GetInstance()->RegisterCdm(
      content::CdmInfo(kWidevineCdmDisplayName, kWidevineCdmGuid, cdm_version,
                       cdm_path, kWidevineCdmFileSystemId,
                       capability, kWidevineKeySystem, false));
}
