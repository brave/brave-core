/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_playlists_api.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "brave/browser/playlists/playlists_service_factory.h"
#include "brave/common/extensions/api/brave_playlists.h"
#include "brave/components/playlists/browser/playlists_controller.h"
#include "brave/components/playlists/browser/playlists_service.h"
#include "brave/components/playlists/browser/playlists_types.h"
#include "chrome/browser/profiles/profile.h"

using brave_playlists::CreatePlaylistParams;
using brave_playlists::PlaylistsController;
using brave_playlists::PlaylistsServiceFactory;

namespace CreatePlaylist = extensions::api::brave_playlists::CreatePlaylist;
namespace GetPlaylist = extensions::api::brave_playlists::GetPlaylist;
namespace DeletePlaylist = extensions::api::brave_playlists::DeletePlaylist;
namespace RequestDownload = extensions::api::brave_playlists::RequestDownload;

namespace {

constexpr char kNotInitializedError[] = "Not initialized";
constexpr char kAlreadyInitializedError[] = "Already initialized";
constexpr char kInvalidArgsError[] = "Invalid arguments";
constexpr char kUnknownError[] = "Unknown";
constexpr char kNotExistPlaylistError[] = "Playlist does not exist";

PlaylistsController* GetPlaylistsController(content::BrowserContext* context) {
  return PlaylistsServiceFactory::GetInstance()
      ->GetForProfile(Profile::FromBrowserContext(context))
      ->controller();
}

CreatePlaylistParams GetCreatePlaylistParamsFromCreateParams(
    const CreatePlaylist::Params::CreateParams& params) {
  CreatePlaylistParams p;
  p.playlist_name = params.playlist_name;
  p.playlist_thumbnail_url = params.thumbnail_url;

  for (const auto& file : params.video_media_files)
    p.video_media_files.emplace_back(file.url, file.title);
  for (const auto& file : params.audio_media_files)
    p.audio_media_files.emplace_back(file.url, file.title);
  return p;
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction BravePlaylistsCreatePlaylistFunction::Run() {
  if (!GetPlaylistsController(browser_context())->initialized())
    return RespondNow(Error(kNotInitializedError));

  std::unique_ptr<CreatePlaylist::Params> params(
      CreatePlaylist::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  if (GetPlaylistsController(browser_context())
          ->CreatePlaylist(
              GetCreatePlaylistParamsFromCreateParams(params->create_params))) {
    return RespondNow(NoArguments());
  }

  return RespondNow(Error(kInvalidArgsError));
}

ExtensionFunction::ResponseAction BravePlaylistsIsInitializedFunction::Run() {
  const bool initialized =
      GetPlaylistsController(browser_context())->initialized();
  return RespondNow(OneArgument(std::make_unique<base::Value>(initialized)));
}

ExtensionFunction::ResponseAction BravePlaylistsInitializeFunction::Run() {
  if (GetPlaylistsController(browser_context())->initialized())
    return RespondNow(Error(kAlreadyInitializedError));

  if (PlaylistsServiceFactory::GetInstance()
          ->GetForProfile(Profile::FromBrowserContext(browser_context()))
          ->Init()) {
    return RespondNow(NoArguments());
  }

  return RespondNow(Error(kUnknownError));
}

ExtensionFunction::ResponseAction BravePlaylistsGetAllPlaylistsFunction::Run() {
  if (!GetPlaylistsController(browser_context())->initialized())
    return RespondNow(Error(kNotInitializedError));

  if (GetPlaylistsController(browser_context())
          ->GetAllPlaylists(base::BindOnce(
              &BravePlaylistsGetAllPlaylistsFunction::OnGetAllPlaylists, this)))
    return RespondLater();

  return RespondNow(Error(kUnknownError));
}

void BravePlaylistsGetAllPlaylistsFunction::OnGetAllPlaylists(
    base::Value playlists) {
  if (playlists.is_list())
    Respond(OneArgument(base::Value::ToUniquePtrValue(std::move(playlists))));
  else
    Respond(Error(kNotExistPlaylistError));
}

ExtensionFunction::ResponseAction BravePlaylistsGetPlaylistFunction::Run() {
  if (!GetPlaylistsController(browser_context())->initialized())
    return RespondNow(Error(kNotInitializedError));

  std::unique_ptr<GetPlaylist::Params> params(
      GetPlaylist::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  if (GetPlaylistsController(browser_context())
          ->GetPlaylist(
              params->id,
              base::BindOnce(&BravePlaylistsGetPlaylistFunction::OnGetPlaylist,
                             this))) {
    return RespondLater();
  }

  return RespondNow(Error(kUnknownError));
}

ExtensionFunction::ResponseAction BravePlaylistsRecoverPlaylistFunction::Run() {
  if (!GetPlaylistsController(browser_context())->initialized())
    return RespondNow(Error(kNotInitializedError));

  std::unique_ptr<GetPlaylist::Params> params(
      GetPlaylist::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  GetPlaylistsController(browser_context())->RecoverPlaylist(params->id);
  return RespondNow(NoArguments());
}

void BravePlaylistsGetPlaylistFunction::OnGetPlaylist(base::Value playlist) {
  if (playlist.is_dict())
    Respond(OneArgument(base::Value::ToUniquePtrValue(std::move(playlist))));
  else
    Respond(Error(kNotExistPlaylistError));
}

ExtensionFunction::ResponseAction BravePlaylistsDeletePlaylistFunction::Run() {
  if (!GetPlaylistsController(browser_context())->initialized())
    return RespondNow(Error(kNotInitializedError));

  std::unique_ptr<DeletePlaylist::Params> params(
      DeletePlaylist::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  if (GetPlaylistsController(browser_context())->DeletePlaylist(params->id))
    return RespondNow(NoArguments());

  return RespondNow(Error(kUnknownError));
}

ExtensionFunction::ResponseAction
BravePlaylistsDeleteAllPlaylistsFunction::Run() {
  if (!GetPlaylistsController(browser_context())->initialized())
    return RespondNow(Error(kNotInitializedError));

  if (GetPlaylistsController(browser_context())
          ->DeleteAllPlaylists(base::BindOnce(
              &BravePlaylistsDeleteAllPlaylistsFunction::OnDeleteAllPlaylists,
              this))) {
    return RespondLater();
  }

  return RespondNow(Error(kUnknownError));
}

void BravePlaylistsDeleteAllPlaylistsFunction::OnDeleteAllPlaylists(
    bool deleted) {
  Respond(OneArgument(base::Value::ToUniquePtrValue(base::Value(deleted))));
}

ExtensionFunction::ResponseAction BravePlaylistsRequestDownloadFunction::Run() {
  if (!GetPlaylistsController(browser_context())->initialized())
    return RespondNow(Error(kNotInitializedError));

  std::unique_ptr<RequestDownload::Params> params(
      RequestDownload::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  if (GetPlaylistsController(browser_context())->RequestDownload(params->url))
    return RespondNow(NoArguments());

  return RespondNow(Error(kUnknownError));
}

ExtensionFunction::ResponseAction BravePlaylistsPlayFunction::Run() {
  if (!GetPlaylistsController(browser_context())->initialized())
    return RespondNow(Error(kNotInitializedError));

  std::unique_ptr<GetPlaylist::Params> params(
      GetPlaylist::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  GetPlaylistsController(browser_context())->Play(params->id);
  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
