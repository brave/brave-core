// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_NEW_TAB_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_NEW_TAB_PAGE_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/webui/brave_new_tab/update_observer.h"
#include "brave/components/brave_new_tab/new_tab_page.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;
class TemplateURLService;

namespace brave_private_cdn {
class PrivateCDNRequestHelper;
}

namespace tabs {
class TabInterface;
}

namespace brave_new_tab {

class BackgroundAdapter;
class CustomImageChooser;

class NewTabPageHandler : public mojom::NewTabPageHandler {
 public:
  NewTabPageHandler(
      mojo::PendingReceiver<mojom::NewTabPageHandler> receiver,
      std::unique_ptr<CustomImageChooser> custom_image_chooser,
      std::unique_ptr<BackgroundAdapter> background_adapter,
      std::unique_ptr<brave_private_cdn::PrivateCDNRequestHelper> pcdn_helper,
      tabs::TabInterface& tab,
      PrefService& pref_service,
      TemplateURLService& template_url_service);

  ~NewTabPageHandler() override;

  // mojom::NewTabPageHandler:
  void SetNewTabPage(mojo::PendingRemote<mojom::NewTabPage> page) override;
  void LoadResourceFromPcdn(const std::string& url,
                            LoadResourceFromPcdnCallback callback) override;
  void GetBackgroundsEnabled(GetBackgroundsEnabledCallback callback) override;
  void SetBackgroundsEnabled(bool enabled,
                             SetBackgroundsEnabledCallback callback) override;
  void GetBackgroundsCustomizable(
      GetBackgroundsCustomizableCallback callback) override;
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
  void GetShowSearchBox(GetShowSearchBoxCallback callback) override;
  void SetShowSearchBox(bool show_search_box,
                        SetShowSearchBoxCallback callback) override;
  void GetSearchSuggestionsEnabled(
      GetSearchSuggestionsEnabledCallback callback) override;
  void SetSearchSuggestionsEnabled(
      bool enabled,
      SetSearchSuggestionsEnabledCallback callback) override;
  void GetSearchSuggestionsPromptDismissed(
      GetSearchSuggestionsPromptDismissedCallback callback) override;
  void SetSearchSuggestionsPromptDismissed(
      bool dismissed,
      SetSearchSuggestionsPromptDismissedCallback callback) override;
  void GetLastUsedSearchEngine(
      GetLastUsedSearchEngineCallback callback) override;
  void SetLastUsedSearchEngine(
      const std::string& engine_host,
      SetLastUsedSearchEngineCallback callback) override;
  void GetAvailableSearchEngines(
      GetAvailableSearchEnginesCallback callback) override;
  void OpenSearch(const std::string& query,
                  const std::string& engine,
                  mojom::EventDetailsPtr details,
                  OpenSearchCallback callback) override;
  void OpenURLFromSearch(const std::string& url,
                         mojom::EventDetailsPtr details,
                         OpenURLFromSearchCallback callback) override;

 private:
  void OnCustomBackgroundsSelected(ShowCustomBackgroundChooserCallback callback,
                                   std::vector<base::FilePath> paths);

  void OnUpdate(UpdateObserver::Source update_source);

  mojo::Receiver<mojom::NewTabPageHandler> receiver_;
  mojo::Remote<mojom::NewTabPage> page_;
  UpdateObserver update_observer_;
  std::unique_ptr<CustomImageChooser> custom_image_chooser_;
  std::unique_ptr<BackgroundAdapter> background_adapter_;
  std::unique_ptr<brave_private_cdn::PrivateCDNRequestHelper> pcdn_helper_;
  raw_ref<tabs::TabInterface> tab_;
  raw_ref<PrefService> pref_service_;
  raw_ref<TemplateURLService> template_url_service_;
  base::WeakPtrFactory<NewTabPageHandler> weak_factory_{this};
};

}  // namespace brave_new_tab

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_NEW_TAB_PAGE_HANDLER_H_
