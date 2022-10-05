/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLIST_PLAYLIST_PAGE_HANDLER_FACTORY_H_
#define BRAVE_BROWSER_PLAYLIST_PLAYLIST_PAGE_HANDLER_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/browser/ui/playlist/playlist_page_handler.h"
#include "brave/components/playlist/mojom/playlist.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class PlaylistPageHandler;

namespace playlist {

class PlaylistPageHandlerFactory : public BrowserContextKeyedServiceFactory {
 public:
  PlaylistPageHandlerFactory(const PlaylistPageHandlerFactory&) = delete;
  PlaylistPageHandlerFactory& operator=(const PlaylistPageHandlerFactory&) =
      delete;

  static mojo::PendingRemote<mojom::PageHandler> GetForContext(
      content::BrowserContext* context);
  static PlaylistPageHandler* GetServiceForContext(
      content::BrowserContext* context);
  static PlaylistPageHandlerFactory* GetInstance();
  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::PageHandler> receiver);

 private:
  friend struct base::DefaultSingletonTraits<PlaylistPageHandlerFactory>;

  PlaylistPageHandlerFactory();
  ~PlaylistPageHandlerFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_PLAYLIST_PLAYLIST_PAGE_HANDLER_FACTORY_H_
