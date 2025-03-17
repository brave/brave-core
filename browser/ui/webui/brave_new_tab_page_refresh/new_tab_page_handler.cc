// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/new_tab_page_handler.h"

#include <utility>

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/background_facade.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/custom_image_chooser.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_new_tab_page_refresh {

NewTabPageHandler::NewTabPageHandler(
    mojo::PendingReceiver<mojom::NewTabPageHandler> receiver,
    std::unique_ptr<CustomImageChooser> custom_image_chooser,
    std::unique_ptr<BackgroundFacade> background_facade,
    PrefService& pref_service)
    : receiver_(this, std::move(receiver)),
      update_observer_(pref_service),
      custom_image_chooser_(std::move(custom_image_chooser)),
      background_facade_(std::move(background_facade)),
      pref_service_(pref_service) {
  CHECK(custom_image_chooser_);
  CHECK(background_facade_);

  update_observer_.SetCallback(base::BindRepeating(&NewTabPageHandler::OnUpdate,
                                                   weak_factory_.GetWeakPtr()));
}

NewTabPageHandler::~NewTabPageHandler() = default;

void NewTabPageHandler::SetNewTabPage(
    mojo::PendingRemote<mojom::NewTabPage> page) {
  page_.reset();
  page_.Bind(std::move(page));
}

void NewTabPageHandler::GetBackgroundsEnabled(
    GetBackgroundsEnabledCallback callback) {
  bool backgrounds_enabled = pref_service_->GetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage);
  std::move(callback).Run(backgrounds_enabled);
}

void NewTabPageHandler::SetBackgroundsEnabled(
    bool enabled,
    SetBackgroundsEnabledCallback callback) {
  pref_service_->SetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, enabled);
  std::move(callback).Run();
}

void NewTabPageHandler::GetSponsoredImagesEnabled(
    GetSponsoredImagesEnabledCallback callback) {
  bool sponsored_images_enabled = pref_service_->GetBoolean(
      ntp_background_images::prefs::
          kNewTabPageShowSponsoredImagesBackgroundImage);
  std::move(callback).Run(sponsored_images_enabled);
}

void NewTabPageHandler::SetSponsoredImagesEnabled(
    bool enabled,
    SetSponsoredImagesEnabledCallback callback) {
  pref_service_->SetBoolean(ntp_background_images::prefs::
                                kNewTabPageShowSponsoredImagesBackgroundImage,
                            enabled);
  std::move(callback).Run();
}

void NewTabPageHandler::GetBraveBackgrounds(
    GetBraveBackgroundsCallback callback) {
  std::move(callback).Run(background_facade_->GetBraveBackgrounds());
}

void NewTabPageHandler::GetCustomBackgrounds(
    GetCustomBackgroundsCallback callback) {
  std::move(callback).Run(background_facade_->GetCustomBackgrounds());
}

void NewTabPageHandler::GetSelectedBackground(
    GetSelectedBackgroundCallback callback) {
  std::move(callback).Run(background_facade_->GetSelectedBackground());
}

void NewTabPageHandler::GetSponsoredImageBackground(
    GetSponsoredImageBackgroundCallback callback) {
  std::move(callback).Run(background_facade_->GetSponsoredImageBackground());
}

void NewTabPageHandler::SelectBackground(
    mojom::SelectedBackgroundPtr background,
    SelectBackgroundCallback callback) {
  background_facade_->SelectBackground(std::move(background));
  std::move(callback).Run();
}

void NewTabPageHandler::ShowCustomBackgroundChooser(
    ShowCustomBackgroundChooserCallback callback) {
  custom_image_chooser_->ShowDialog(
      base::BindOnce(&NewTabPageHandler::OnCustomBackgroundsSelected,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NewTabPageHandler::RemoveCustomBackground(
    const std::string& background_url,
    RemoveCustomBackgroundCallback callback) {
  background_facade_->RemoveCustomBackground(background_url,
                                             std::move(callback));
}

void NewTabPageHandler::NotifySponsoredImageLogoClicked(
    const std::string& creative_instance_id,
    const std::string& destination_url,
    const std::string& wallpaper_id,
    NotifySponsoredImageLogoClickedCallback callback) {
  background_facade_->NotifySponsoredImageLogoClicked(
      creative_instance_id, destination_url, wallpaper_id);
  std::move(callback).Run();
}

void NewTabPageHandler::OnCustomBackgroundsSelected(
    ShowCustomBackgroundChooserCallback callback,
    std::vector<base::FilePath> paths) {
  // Before continuing, notify the caller of whether backgrounds were selected.
  // This allows the front-end to display a loading indicator while the save
  // operation is in progress.
  std::move(callback).Run(!paths.empty());

  if (!paths.empty()) {
    background_facade_->SaveCustomBackgrounds(std::move(paths),
                                              base::DoNothing());
  }
}

void NewTabPageHandler::OnUpdate(UpdateObserver::Source update_source) {
  if (!page_.is_bound()) {
    return;
  }
  switch (update_source) {
    case UpdateObserver::Source::kBackgrounds:
      page_->OnBackgroundsUpdated();
      break;
  }
}

}  // namespace brave_new_tab_page_refresh
