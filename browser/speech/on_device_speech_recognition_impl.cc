/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Implements the hooks that the chromium_src override of
// `chrome/browser/speech/on_device_speech_recognition_impl.cc` forward
// declares.

#include "chrome/browser/speech/on_device_speech_recognition_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/speech/on_device_speech_model_permission_request.h"
#include "brave/components/local_ai/core/on_device_speech_locales.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom-shared.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "brave/components/local_ai/core/utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/permissions/permission_request_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

namespace speech {

// Implemented in on_device_speech_recognition_util.cc.
media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    content::BrowserContext* context,
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality);

namespace {

// The model to ask the user to download before `Install` runs: the
// multilingual model when one of `languages` needs it, because it also serves
// English, and the English model otherwise. std::nullopt when none of
// `languages` is `kDownloadable`, when one is `kUnavailable`, because upstream
// `install()` then fails for all of them, or when the enabled model serves them
// all. Each of `languages` must be the tag of a locale in
// `GetOnDeviceSpeechLocales()`.
std::optional<local_ai::OnDeviceSpeechModel> GetBraveOnDeviceSpeechModelToAsk(
    content::BrowserContext* context,
    base::span<const std::string> languages,
    media::mojom::SpeechRecognitionQuality quality) {
  bool needs_download = false;
  local_ai::OnDeviceSpeechModel model_to_ask =
      local_ai::OnDeviceSpeechModel::kEnglish;
  for (const std::string& language : languages) {
    const local_ai::OnDeviceSpeechLocale* locale =
        local_ai::ResolveOnDeviceSpeechLocale(language);
    CHECK(locale);
    const media::mojom::AvailabilityStatus availability =
        GetBraveOnDeviceSpeechAvailability(context, language, quality);
    if (availability == media::mojom::AvailabilityStatus::kUnavailable) {
      return std::nullopt;
    }
    if (availability == media::mojom::AvailabilityStatus::kDownloadable) {
      needs_download = true;
    }
    if (locale->model == local_ai::OnDeviceSpeechModel::kMultilingual) {
      model_to_ask = local_ai::OnDeviceSpeechModel::kMultilingual;
    }
  }
  const std::optional<local_ai::OnDeviceSpeechModel> enabled =
      local_ai::GetEnabledOnDeviceSpeechModel(g_browser_process->local_state());
  // No prompt when nothing needs a download, or when the enabled model serves
  // all `languages`. The multilingual model serves all locales. The English
  // model serves `languages` only when all of them are en-US. Then
  // `model_to_ask` is the English model.
  if (!needs_download || enabled == model_to_ask ||
      enabled == local_ai::OnDeviceSpeechModel::kMultilingual) {
    return std::nullopt;
  }
  return model_to_ask;
}

// Answers the prompt that `PromptForBraveOnDeviceSpeechModel` shows for
// `model`. When the user allows it, enables `model`, which is what makes the
// component registrar register the model for download, and runs `install` with
// `callback`. Otherwise answers false.
//
// One prompt can serve more than one `install()` call of the page. Allow runs
// this function once for each call, in the order of the calls. Thus the calls
// can enable different models:
// - The English model never replaces the multilingual model, which also
//   serves English.
// - The multilingual model can replace the English model after an English
//   call requested the English model. The English registrar then answers
//   the request for the English model with false.
//   `BraveSodaInstaller::OnSpeechModelInstallFinished` sees that the
//   multilingual model is enabled and requests it for en-US. Thus the English
//   call does not fail. It waits for the multilingual model.
void OnPromptAnswered(
    local_ai::OnDeviceSpeechModel model,
    base::OnceCallback<void(base::OnceCallback<void(bool)>)> install,
    base::OnceCallback<void(bool)> callback,
    bool allowed) {
  if (!allowed) {
    std::move(callback).Run(false);
    return;
  }
  PrefService& local_state = *g_browser_process->local_state();
  if (local_ai::GetEnabledOnDeviceSpeechModel(&local_state) !=
      local_ai::OnDeviceSpeechModel::kMultilingual) {
    local_ai::EnableOnDeviceSpeechModel(local_state, model);
  }
  std::move(install).Run(std::move(callback));
}

// Asks to download `model`. Runs `install` with `callback` once the user allows
// it, and otherwise answers false. "Don't ask again" is answered without a
// prompt.
void PromptForBraveOnDeviceSpeechModel(
    content::RenderFrameHost& rfh,
    local_ai::OnDeviceSpeechModel model,
    base::OnceCallback<void(base::OnceCallback<void(bool)>)> install,
    base::OnceCallback<void(bool)> callback) {
  Profile* profile = Profile::FromBrowserContext(rfh.GetBrowserContext());
  auto* manager = permissions::PermissionRequestManager::FromWebContents(
      content::WebContents::FromRenderFrameHost(&rfh));
  if (!manager || !profile->GetPrefs()->GetBoolean(
                      local_ai::prefs::kAskEnableOnDeviceSpeechModel)) {
    std::move(callback).Run(false);
    return;
  }

  manager->AddRequest(
      &rfh, std::make_unique<OnDeviceSpeechModelPermissionRequest>(
                *profile->GetPrefs(), rfh.GetLastCommittedOrigin().GetURL(),
                base::BindOnce(&OnPromptAnswered, model, std::move(install),
                               std::move(callback))));
}

// Sends `kDictation` down upstream's SODA branch, as `kCommand`, in both
// `Available` and `Install`. That branch is where `BraveSodaInstaller` installs
// Brave's model, and both functions must take it, so that `available()` reads
// the language names that `install()` records for the origin. Otherwise
// upstream refuses `kDictation` unless `kOnDeviceWebSpeechSmallExpertModel` is
// enabled, and `install()` asks the optimization guide for a model Brave does
// not ship.
//
// Nothing Brave decides depends on it. Brave's policy answers `kCommand` and
// `kDictation` alike, and `BraveSodaInstaller` installs one model for both.
media::mojom::SpeechRecognitionQuality ToBraveOnDeviceSpeechQuality(
    media::mojom::SpeechRecognitionQuality quality) {
  return quality == media::mojom::SpeechRecognitionQuality::kDictation
             ? media::mojom::SpeechRecognitionQuality::kCommand
             : quality;
}

// Maps each tag that the page gives to the tag of the Brave locale that
// serves it, such as "es-MX" to "es-US". Returns nothing when no Brave locale
// serves one of the tags. The hooks give upstream only the mapped tags. Thus
// upstream records and settles each locale by its own tag, and SODA's lookups
// find an exact match.
std::optional<std::vector<std::string>> MapPageTagsToBraveLocaleTags(
    base::span<const std::string> page_tags) {
  std::vector<std::string> locale_tags;
  for (const std::string& page_tag : page_tags) {
    const local_ai::OnDeviceSpeechLocale* locale =
        local_ai::ResolveOnDeviceSpeechLocale(page_tag);
    if (!locale) {
      return std::nullopt;
    }
    locale_tags.emplace_back(locale->tag);
  }
  return locale_tags;
}

}  // namespace

// Runs first in `Available`. Answers `kUnavailable` when no locale serves one
// of `languages`. Runs `Available` again with Brave's locale tags when they
// differ from `languages`, such as "de-DE" for "de-AT". Returns true in both
// cases, and only then takes `callback`. Otherwise maps `quality` for
// upstream's SODA branch, and lets upstream answer.
bool MaybeHandleBraveOnDeviceSpeechAvailable(
    base::WeakPtr<OnDeviceSpeechRecognitionImpl> impl,
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality& quality,
    OnDeviceSpeechRecognitionImpl::AvailableCallback& callback) {
  std::optional<std::vector<std::string>> locale_tags =
      MapPageTagsToBraveLocaleTags(languages);
  if (!locale_tags) {
    std::move(callback).Run(media::mojom::AvailabilityStatus::kUnavailable);
    return true;
  }
  if (*locale_tags != languages) {
    impl->Available(*locale_tags, quality, std::move(callback));
    return true;
  }
  quality = ToBraveOnDeviceSpeechQuality(quality);
  return false;
}

// Runs first in `Install`. Cancels `Install` in a Private, Tor or Guest
// profile, or when no locale serves one of `languages`. Runs `Install` again
// with Brave's locale tags when they differ from `languages`. Otherwise prompts
// before `Install` starts a download of Brave's model, and runs `Install` again
// once the user allows it. Returns true when it cancels, runs `Install` again
// or prompts, and only then takes `callback`. Otherwise maps `quality` for
// upstream's SODA branch, and lets upstream answer.
// `frame_can_use_on_device_speech` is what the private
// `CanRenderFrameHostUseOnDeviceSpeechRecognition` says, so there is no prompt
// for a frame that upstream refuses.
//
// The prompt reads `quality` before the mapping. That is fine, because it asks
// Brave's policy, not upstream's branches, and `Install` maps the quality again
// when it runs again.
bool MaybeHandleBraveOnDeviceSpeechInstall(
    bool frame_can_use_on_device_speech,
    base::WeakPtr<OnDeviceSpeechRecognitionImpl> impl,
    const std::vector<std::string>& languages,
    media::mojom::SpeechRecognitionQuality& quality,
    OnDeviceSpeechRecognitionImpl::InstallCallback& callback) {
  content::RenderFrameHost& rfh = impl->render_frame_host();
  content::BrowserContext* context = rfh.GetBrowserContext();
  // Cancels `Install` in Private, Tor and Guest profiles.
  if (!Profile::FromBrowserContext(context)->IsRegularProfile()) {
    std::move(callback).Run(false);
    return true;
  }
  std::optional<std::vector<std::string>> locale_tags =
      MapPageTagsToBraveLocaleTags(languages);
  if (!locale_tags) {
    std::move(callback).Run(false);
    return true;
  }
  if (*locale_tags != languages) {
    impl->Install(*locale_tags, quality, std::move(callback));
    return true;
  }
  const std::optional<local_ai::OnDeviceSpeechModel> model =
      frame_can_use_on_device_speech
          ? GetBraveOnDeviceSpeechModelToAsk(context, languages, quality)
          : std::nullopt;
  if (model) {
    PromptForBraveOnDeviceSpeechModel(
        rfh, *model,
        base::BindOnce(&OnDeviceSpeechRecognitionImpl::Install, std::move(impl),
                       languages, quality),
        std::move(callback));
    return true;
  }
  quality = ToBraveOnDeviceSpeechQuality(quality);
  return false;
}

}  // namespace speech
