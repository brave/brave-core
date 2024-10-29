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
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/task/thread_pool.h"
#include "brave/browser/brave_stats/first_run_util.h"
#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/playlist/browser/type_converter.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/playlist/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/image_fetcher/image_decoder_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/sessions/content/session_tab_helper.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#else
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/browser/playlist/playlist_data_source.h"
#endif

namespace playlist {

namespace {

data_decoder::DataDecoder* GetDataDecoder() {
  static base::NoDestructor<data_decoder::DataDecoder> data_decoder;
  return data_decoder.get();
}

class PlaylistServiceDelegateImpl : public PlaylistService::Delegate {
 public:
  explicit PlaylistServiceDelegateImpl(Profile* profile) : profile_(profile) {}
  PlaylistServiceDelegateImpl(const PlaylistServiceDelegateImpl&) = delete;
  PlaylistServiceDelegateImpl& operator=(const PlaylistServiceDelegateImpl&) =
      delete;
  ~PlaylistServiceDelegateImpl() override = default;

  content::WebContents* GetActiveWebContents() override {
#if BUILDFLAG(IS_ANDROID)
    auto tab_models = TabModelList::models();
    auto iter = base::ranges::find_if(
        tab_models, [](const auto& model) { return model->IsActiveModel(); });
    if (iter == tab_models.end()) {
      return nullptr;
    }

    auto* active_contents = (*iter)->GetActiveWebContents();
    DCHECK(active_contents);
    DCHECK_EQ(active_contents->GetBrowserContext(), profile_.get());
    return active_contents;
#else
    auto* browser = chrome::FindLastActiveWithProfile(profile_);
    if (!browser) {
      return nullptr;
    }

    auto* tab_model = browser->tab_strip_model();
    DCHECK(tab_model);
    return tab_model->GetActiveWebContents();
#endif  // BUILDFLAG(IS_ANDROID)
  }

  void SanitizeImage(
      std::unique_ptr<std::string> image,
      base::OnceCallback<void(scoped_refptr<base::RefCountedBytes>)> callback)
      override {
    DecodeImageInIsolatedProcess(
        std::move(image),
        base::BindOnce(&PlaylistServiceDelegateImpl::EncodeAsPNG,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void EnabledStateChanged(bool enabled) override {
#if !BUILDFLAG(IS_ANDROID)
    // Before removing the Playlist item from the service, close all active
    // Playlist panels.
    for (Browser* browser : *BrowserList::GetInstance()) {
      if (!browser->is_type_normal() || browser->profile() != profile_) {
        continue;
      }

      auto* side_panel_ui = browser->GetFeatures().side_panel_ui();
      if (!side_panel_ui ||
          side_panel_ui->GetCurrentEntryId() != SidePanelEntryId::kPlaylist) {
        continue;
      }

      side_panel_ui->Close();
    }

    auto* service =
        sidebar::SidebarServiceFactory::GetForProfile(profile_.get());
    if (enabled) {
      const auto hidden_items = service->GetHiddenDefaultSidebarItems();
      const auto iter = base::ranges::find(
          hidden_items, sidebar::SidebarItem::BuiltInItemType::kPlaylist,
          &sidebar::SidebarItem::built_in_item_type);
      if (iter != hidden_items.end()) {
        service->AddItem(*iter);
      }
    } else {
      const auto visible_items = service->items();
      const auto iter = base::ranges::find(
          visible_items, sidebar::SidebarItem::BuiltInItemType::kPlaylist,
          &sidebar::SidebarItem::built_in_item_type);
      if (iter != visible_items.end()) {
        service->RemoveItemAt(iter - visible_items.begin());
      }
    }
#endif  // !BUILDFLAG(IS_ANDROID)
  }

 private:
  scoped_refptr<base::SequencedTaskRunner> GetOrCreateTaskRunner() {
    if (!task_runner_) {
      task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN});
    }
    return task_runner_;
  }

  void DecodeImageInIsolatedProcess(
      std::unique_ptr<std::string> image,
      base::OnceCallback<void(const gfx::Image&)> callback) {
    if (!image_decoder_) {
      image_decoder_ = std::make_unique<ImageDecoderImpl>();
    }

    image_decoder_->DecodeImage(std::move(*image),
                                gfx::Size() /* No particular size desired. */,
                                GetDataDecoder(), std::move(callback));
  }

  void EncodeAsPNG(
      base::OnceCallback<void(scoped_refptr<base::RefCountedBytes>)> callback,
      const gfx::Image& decoded_image) {
    auto encode = base::BindOnce(
        [](const SkBitmap& bitmap) {
          auto encoded = base::MakeRefCounted<base::RefCountedBytes>();
          if (auto result = gfx::PNGCodec::EncodeBGRASkBitmap(
                  bitmap,
                  /*discard_transparency=*/false)) {
            encoded->as_vector() = std::move(result).value();
          } else {
            DVLOG(2) << "Failed to encode image as PNG";
          }

          return encoded;
        },
        decoded_image.AsBitmap());

    GetOrCreateTaskRunner()->PostTaskAndReplyWithResult(
        FROM_HERE, std::move(encode), std::move(callback));
  }

  raw_ptr<Profile> profile_;

  std::unique_ptr<image_fetcher::ImageDecoder> image_decoder_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  base::WeakPtrFactory<PlaylistServiceDelegateImpl> weak_ptr_factory_{this};
};

}  // namespace

// static
PlaylistServiceFactory* PlaylistServiceFactory::GetInstance() {
  static base::NoDestructor<PlaylistServiceFactory> instance;
  return instance.get();
}

// static
PlaylistService* PlaylistServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  DCHECK(context);
  return static_cast<PlaylistService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

#if BUILDFLAG(IS_ANDROID)
// static
mojo::PendingRemote<mojom::PlaylistService>
PlaylistServiceFactory::GetForContext(content::BrowserContext* context) {
  return static_cast<PlaylistService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}
#endif  // BUILDFLAG(IS_ANDROID)

// static
void PlaylistServiceFactory::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  // Prefs for P3A
  registry->RegisterTimePref(kPlaylistFirstUsageTime, base::Time());
  registry->RegisterTimePref(kPlaylistLastUsageTime, base::Time());
  registry->RegisterBooleanPref(kPlaylistUsedSecondDay, false);
  registry->RegisterListPref(kPlaylistUsageWeeklyStorage);
}

void PlaylistServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  auto default_list = mojom::Playlist::New();
  default_list->id = kDefaultPlaylistID;

  base::Value::Dict playlists_value;
  playlists_value.Set(kDefaultPlaylistID,
                      playlist::ConvertPlaylistToValue(default_list));
  registry->RegisterDictionaryPref(kPlaylistsPref, std::move(playlists_value));

  auto order_list = base::Value::List();
  order_list.Append(kDefaultPlaylistID);
  registry->RegisterListPref(kPlaylistOrderPref, std::move(order_list));

  registry->RegisterDictionaryPref(kPlaylistItemsPref);
  registry->RegisterBooleanPref(kPlaylistEnabledPref, true);
  registry->RegisterBooleanPref(kPlaylistCacheByDefault, true);
  registry->RegisterStringPref(kPlaylistDefaultSaveTargetListID,
                               kDefaultPlaylistID);
}

PlaylistServiceFactory::PlaylistServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "PlaylistService",
          BrowserContextDependencyManager::GetInstance()) {}

PlaylistServiceFactory::~PlaylistServiceFactory() = default;

std::unique_ptr<KeyedService>
PlaylistServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    return nullptr;
  }

  auto* profile = Profile::FromBrowserContext(context);
  if (!profile->IsRegularProfile()) {
    return nullptr;
  }

  GetInstance()->PrepareMediaDetectorComponentManager();

  PrefService* local_state = g_browser_process->local_state();
  auto service = std::make_unique<PlaylistService>(
      context, local_state, media_detector_component_manager_.get(),
      std::make_unique<PlaylistServiceDelegateImpl>(profile),
      brave_stats::GetFirstRunTime(local_state));

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  content::URLDataSource::Add(
      context, std::make_unique<PlaylistDataSource>(profile, service.get()));
#endif

  return service;
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
