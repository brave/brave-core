/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/playlist/playlist_api.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/notreached.h"
#include "brave/browser/playlist/desktop_playlist_player.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/common/extensions/api/playlist.h"
#include "brave/components/playlist/playlist_service.h"
#include "brave/components/playlist/playlist_types.h"

using playlist::CreatePlaylistParams;
using playlist::PlaylistService;
using playlist::PlaylistServiceFactory;

namespace GetPlaylistItem = extensions::api::playlist::GetPlaylistItem;
namespace DeletePlaylistItem = extensions::api::playlist::DeletePlaylistItem;
namespace RecoverPlaylistItem = extensions::api::playlist::RecoverPlaylistItem;
namespace PlayItem = extensions::api::playlist::PlayItem;
namespace RequestDownload = extensions::api::playlist::RequestDownload;

namespace {

constexpr char kNotExistPlaylistError[] = "Playlist does not exist";
constexpr char kFeatureDisabled[] = "Playlist feature is disabled";

PlaylistService* GetPlaylistService(content::BrowserContext* context) {
  return PlaylistServiceFactory::GetInstance()->GetForBrowserContext(context);
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction PlaylistGetAllPlaylistItemsFunction::Run() {
  auto* service = GetPlaylistService(browser_context());
  if (!service) {
    return RespondNow(Error(kFeatureDisabled));
  }

  return RespondNow(OneArgument(service->GetAllPlaylistItems()));
}

ExtensionFunction::ResponseAction PlaylistGetPlaylistItemFunction::Run() {
  auto* service = GetPlaylistService(browser_context());
  if (!service) {
    return RespondNow(Error(kFeatureDisabled));
  }

  std::unique_ptr<GetPlaylistItem::Params> params(
      GetPlaylistItem::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  base::Value playlist = service->GetPlaylistItem(params->id);
  DCHECK(playlist.is_dict());

  if (playlist.DictEmpty())
    return RespondNow(Error(kNotExistPlaylistError));

  return RespondNow(OneArgument(std::move(playlist)));
}

ExtensionFunction::ResponseAction PlaylistRecoverPlaylistItemFunction::Run() {
  auto* service = GetPlaylistService(browser_context());
  if (!service) {
    return RespondNow(Error(kFeatureDisabled));
  }

  std::unique_ptr<RecoverPlaylistItem::Params> params(
      RecoverPlaylistItem::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  service->RecoverPlaylistItem(params->id);
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction PlaylistDeletePlaylistItemFunction::Run() {
  auto* service = GetPlaylistService(browser_context());
  if (!service) {
    return RespondNow(Error(kFeatureDisabled));
  }

  std::unique_ptr<DeletePlaylistItem::Params> params(
      DeletePlaylistItem::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  service->DeletePlaylistItem(params->id);
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
PlaylistDeleteAllPlaylistItemsFunction::Run() {
  auto* service = GetPlaylistService(browser_context());
  if (!service) {
    return RespondNow(Error(kFeatureDisabled));
  }

  service->DeleteAllPlaylistItems();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction PlaylistRequestDownloadFunction::Run() {
  auto* service = GetPlaylistService(browser_context());
  if (!service) {
    return RespondNow(Error(kFeatureDisabled));
  }

  std::unique_ptr<RequestDownload::Params> params(
      RequestDownload::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  service->RequestDownload(params->url);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction PlaylistPlayItemFunction::Run() {
  auto* service = GetPlaylistService(browser_context());
  if (!service) {
    return RespondNow(Error(kFeatureDisabled));
  }

  std::unique_ptr<PlayItem::Params> params(PlayItem::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  ::playlist::DesktopPlaylistPlayer player(browser_context());
  player.Play(params->id);
  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
