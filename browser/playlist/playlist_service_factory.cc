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
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_download_request_manager.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/playlist/browser/type_converter.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/playlist/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/image_fetcher/image_decoder_impl.h"
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
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
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
          if (!gfx::PNGCodec::EncodeBGRASkBitmap(
                  bitmap, /*discard_transparency=*/false, &encoded->data())) {
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
  if (IsPlaylistEnabled(context)) {
    GetInstance()->PrepareMediaDetectorComponentManager();

    return static_cast<PlaylistService*>(
        GetInstance()->GetServiceForBrowserContext(context, true));
  }

  return nullptr;
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
bool PlaylistServiceFactory::IsPlaylistEnabled(
    content::BrowserContext* context) {
  return base::FeatureList::IsEnabled(playlist::features::kPlaylist) &&
         brave::IsRegularProfile(context);
}

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
  registry->RegisterBooleanPref(kPlaylistCacheByDefault, true);
  registry->RegisterStringPref(kPlaylistDefaultSaveTargetListID,
                               kDefaultPlaylistID);
}

PlaylistServiceFactory::PlaylistServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "PlaylistService",
          BrowserContextDependencyManager::GetInstance()) {
  PlaylistDownloadRequestManager::SetPlaylistJavaScriptWorldId(
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

PlaylistServiceFactory::~PlaylistServiceFactory() = default;

KeyedService* PlaylistServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  DCHECK(media_detector_component_manager_);
  PrefService* local_state = g_browser_process->local_state();
  auto* service = new PlaylistService(
      context, local_state, media_detector_component_manager_.get(),
      std::make_unique<PlaylistServiceDelegateImpl>(
          Profile::FromBrowserContext(context)),
      brave_stats::GetFirstRunTime(local_state));

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  content::URLDataSource::Add(
      context, std::make_unique<PlaylistDataSource>(
                   Profile::FromBrowserContext(context), service));
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
