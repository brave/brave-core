// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_NEW_TAB_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_NEW_TAB_PAGE_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/update_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace brave_new_tab_page_refresh {

class BackgroundFacade;
class CustomImageChooser;

// Handler for messages from the NTP front end application. Interface method
// implementations should be fairly trivial. Any non-trivial operations should
// be delegated to a helper class.
class NewTabPageHandler : public mojom::NewTabPageHandler {
 public:
  NewTabPageHandler(mojo::PendingReceiver<mojom::NewTabPageHandler> receiver,
                    std::unique_ptr<CustomImageChooser> custom_image_chooser,
                    std::unique_ptr<BackgroundFacade> background_facade,
                    PrefService& pref_service);

  ~NewTabPageHandler() override;

  // mojom::NewTabPageHandler:
  void SetNewTabPage(mojo::PendingRemote<mojom::NewTabPage> page) override;
  void GetBackgroundsEnabled(GetBackgroundsEnabledCallback callback) override;
  void SetBackgroundsEnabled(bool enabled,
                             SetBackgroundsEnabledCallback callback) override;
  void GetSponsoredImagesEnabled(
      GetSponsoredImagesEnabledCallback callback) override;
  void SetSponsoredImagesEnabled(
      bool enabled,
      SetSponsoredImagesEnabledCallback callback) override;
  void GetBraveBackgrounds(GetBraveBackgroundsCallback callback) override;
  void GetCustomBackgrounds(GetCustomBackgroundsCallback callback) override;
  void GetSelectedBackground(GetSelectedBackgroundCallback callback) override;
  void GetSponsoredImageBackground(
      GetSponsoredImageBackgroundCallback callback) override;
  void SelectBackground(mojom::SelectedBackgroundPtr background,
                        SelectBackgroundCallback callback) override;
  void ShowCustomBackgroundChooser(
      ShowCustomBackgroundChooserCallback callback) override;
  void RemoveCustomBackground(const std::string& background_url,
                              RemoveCustomBackgroundCallback callback) override;
  void NotifySponsoredImageLogoClicked(
      const std::string& creative_instance_id,
      const std::string& destination_url,
      const std::string& wallpaper_id,
      NotifySponsoredImageLogoClickedCallback callback) override;

 private:
  void OnCustomBackgroundsSelected(ShowCustomBackgroundChooserCallback callback,
                                   std::vector<base::FilePath> paths);

  void OnUpdate(UpdateObserver::Source update_source);

  mojo::Receiver<mojom::NewTabPageHandler> receiver_;
  mojo::Remote<mojom::NewTabPage> page_;
  UpdateObserver update_observer_;
  std::unique_ptr<CustomImageChooser> custom_image_chooser_;
  std::unique_ptr<BackgroundFacade> background_facade_;
  raw_ref<PrefService> pref_service_;
  base::WeakPtrFactory<NewTabPageHandler> weak_factory_{this};
};

}  // namespace brave_new_tab_page_refresh

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_NEW_TAB_PAGE_HANDLER_H_
