/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/playlist_service_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/playlist/features.h"
#include "brave/components/playlist/media_detector_component_manager.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_download_request_manager.h"
#include "brave/components/playlist/playlist_service.h"
#include "brave/components/playlist/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace playlist {

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
  base::Value::Dict default_playlist;
  default_playlist.Set(kPlaylistIDKey, kDefaultPlaylistID);
  default_playlist.Set(kPlaylistNameKey, std::string());
  default_playlist.Set(kPlaylistItemsKey, base::Value::List());

  base::Value::Dict dict;
  dict.Set(kDefaultPlaylistID, base::Value(std::move(default_playlist)));

  registry->RegisterDictionaryPref(kPlaylistsPref,
                                   base::Value(std::move(dict)));
  registry->RegisterDictionaryPref(kPlaylistItemsPref);
  registry->RegisterBooleanPref(kPlaylistCacheByDefault, true);
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
  return new PlaylistService(context, media_detector_component_manager_.get());
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
