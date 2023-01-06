/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/playlist_service_factory.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/ranges/algorithm.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/playlist/features.h"
#include "brave/components/playlist/media_detector_component_manager.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_download_request_manager.h"
#include "brave/components/playlist/playlist_service.h"
#include "brave/components/playlist/playlist_service_helper.h"
#include "brave/components/playlist/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/sessions/content/session_tab_helper.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#else
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#endif

namespace playlist {

namespace {

class PlaylistServiceDelegateImpl : public PlaylistService::Delegate {
 public:
#if BUILDFLAG(IS_ANDROID)
  PlaylistServiceDelegateImpl() = default;
#else
  explicit PlaylistServiceDelegateImpl(Profile* profile) : profile_(profile) {}
#endif  // BUILDFLAG(IS_ANDROID)
  PlaylistServiceDelegateImpl(const PlaylistServiceDelegateImpl&) = delete;
  PlaylistServiceDelegateImpl& operator=(const PlaylistServiceDelegateImpl&) =
      delete;
  ~PlaylistServiceDelegateImpl() override = default;

  content::WebContents* GetActiveWebContents() override {
#if BUILDFLAG(IS_ANDROID)
    auto tab_models = TabModelList::models();
    auto iter = base::ranges::find_if(
        tab_models, [](const auto& model) { return model->IsActive(); });
    if (iter == tab_models.end())
      return nullptr;

    iter->GetActiveWebContents();
#else
    auto* browser = chrome::FindLastActiveWithProfile(profile_);
    if (!browser)
      return nullptr;

    auto* tab_model = browser->tab_strip_model();
    DCHECK(tab_model);
    return tab_model->GetActiveWebContents();
#endif  // defined(IS_ANDROID)
  }

 private:
#if !BUILDFLAG(IS_ANDROID)
  raw_ptr<Profile> profile_;
#endif  // !BUILDFLAG(IS_ANDROID)
};

}  // namespace

// static
PlaylistServiceFactory* PlaylistServiceFactory::GetInstance() {
  return base::Singleton<PlaylistServiceFactory>::get();
}

// static
PlaylistService* PlaylistServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  DCHECK(context);
  if (IsPlaylistEnabled(context)) {
    GetInstance()->PrepareMediaDetectorComponentManager();

    return static_cast<PlaylistService*>(
        GetInstance()->GetServiceForBrowserContext(context, true));
  }

  return nullptr;
}

// static
bool PlaylistServiceFactory::IsPlaylistEnabled(
    content::BrowserContext* context) {
  return base::FeatureList::IsEnabled(playlist::features::kPlaylist) &&
         brave::IsRegularProfile(context);
}

void PlaylistServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  auto default_list = mojom::Playlist::New();
  default_list->id = kDefaultPlaylistID;

  base::Value::Dict playlists_value;
  playlists_value.Set(
      kDefaultPlaylistID,
      playlist::TypeConverter::ConvertPlaylistToValue(default_list));

  registry->RegisterDictionaryPref(kPlaylistsPref,
                                   base::Value(std::move(playlists_value)));
  registry->RegisterDictionaryPref(kPlaylistItemsPref);
  registry->RegisterBooleanPref(kPlaylistCacheByDefault, true);
  registry->RegisterStringPref(kPlaylistDefaultSaveTargetListID,
                               kDefaultPlaylistID);
}

PlaylistServiceFactory::PlaylistServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "PlaylistService",
          BrowserContextDependencyManager::GetInstance()) {
  PlaylistDownloadRequestManager::SetPlaylistJavaScriptWorldId(
      ISOLATED_WORLD_ID_CHROME_INTERNAL);
}

PlaylistServiceFactory::~PlaylistServiceFactory() = default;

KeyedService* PlaylistServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  DCHECK(media_detector_component_manager_);
  return new PlaylistService(context, media_detector_component_manager_.get(),
#if BUILDFLAG(IS_ANDROID)
                             std::make_unique<PlaylistServiceDelegateImpl>());
#else
                             std::make_unique<PlaylistServiceDelegateImpl>(
                                 Profile::FromBrowserContext(context)));
#endif  // !BUILDFLAG(IS_ANDROID)
}

void PlaylistServiceFactory::PrepareMediaDetectorComponentManager() {
  if (!media_detector_component_manager_) {
    DCHECK(g_browser_process);

    media_detector_component_manager_ =
        std::make_unique<MediaDetectorComponentManager>(
            g_browser_process->component_updater());
  }
}

}  // namespace playlist
