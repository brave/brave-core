/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/playlist/playlist_event_router_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "base/scoped_observer.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/common/extensions/api/brave_playlist.h"
#include "brave/components/playlist/playlist_service.h"
#include "brave/components/playlist/playlist_service_observer.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/event_router.h"

namespace OnPlaylistItemStatusChanged =
    extensions::api::brave_playlist::OnPlaylistItemStatusChanged;

namespace playlist {

namespace {

PlaylistService* GetPlaylistService(content::BrowserContext* context) {
  return PlaylistServiceFactory::GetInstance()->GetForBrowserContext(context);
}

}  // namespace

class PlaylistEventRouterFactory::PlaylistEventRouter
    : public KeyedService,
      public extensions::EventRouter::Observer,
      public PlaylistServiceObserver {
 public:
  explicit PlaylistEventRouter(content::BrowserContext* context)
      : context_(context) {
    extensions::EventRouter::Get(context)->RegisterObserver(
        this, OnPlaylistItemStatusChanged::kEventName);
  }

  ~PlaylistEventRouter() override = default;

  PlaylistEventRouter(const PlaylistEventRouter&) = delete;
  PlaylistEventRouter& operator=(const PlaylistEventRouter&) = delete;

  // extensions::EventRouter::Observer overrides:
  void OnListenerAdded(const extensions::EventListenerInfo& details) override {
    DCHECK_EQ(details.event_name, OnPlaylistItemStatusChanged::kEventName);
    auto* service = GetPlaylistService(context_);
    DCHECK(service);
    observed_.Add(service);
    extensions::EventRouter::Get(context_)->UnregisterObserver(this);
  }

  // PlaylistServiceObserver overrides:
  void OnPlaylistItemStatusChanged(
      const PlaylistChangeParams& params) override {
    auto event = std::make_unique<extensions::Event>(
        extensions::events::BRAVE_PLAYLIST_ON_PLAYLIST_ITEM_STATUS_CHANGED,
        OnPlaylistItemStatusChanged::kEventName,
        OnPlaylistItemStatusChanged::Create(
            PlaylistChangeParams::GetPlaylistChangeTypeAsString(
                params.change_type),
            params.playlist_id),
        context_);

    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

 private:
  content::BrowserContext* context_;
  ScopedObserver<PlaylistService, PlaylistServiceObserver> observed_{this};
};

// static
PlaylistEventRouterFactory* PlaylistEventRouterFactory::GetInstance() {
  return base::Singleton<PlaylistEventRouterFactory>::get();
}

// static
PlaylistEventRouterFactory::PlaylistEventRouter*
PlaylistEventRouterFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  if (PlaylistServiceFactory::IsPlaylistEnabled(context)) {
    return static_cast<PlaylistEventRouter*>(
        GetInstance()->GetServiceForBrowserContext(context, true));
  }

  return nullptr;
}

PlaylistEventRouterFactory::PlaylistEventRouterFactory()
    : BrowserContextKeyedServiceFactory(
          "PlaylistEventRouter",
          BrowserContextDependencyManager::GetInstance()) {}

PlaylistEventRouterFactory::~PlaylistEventRouterFactory() {}

KeyedService* PlaylistEventRouterFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new PlaylistEventRouter(context);
}

}  // namespace playlist
