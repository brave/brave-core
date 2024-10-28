// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_BACKGROUND_ADAPTER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_BACKGROUND_ADAPTER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/webui/brave_new_tab/new_tab_page.mojom.h"

class CustomBackgroundFileManager;
class PrefService;

namespace ntp_background_images {
class ViewCounterService;
}

namespace brave_new_tab {

// Provides access to background-related APIs for usage by the new tab page.
class BackgroundAdapter {
 public:
  BackgroundAdapter(
      std::unique_ptr<CustomBackgroundFileManager> custom_file_manager,
      PrefService& pref_service,
      ntp_background_images::ViewCounterService* view_counter_service);

  ~BackgroundAdapter();

  BackgroundAdapter(const BackgroundAdapter&) = delete;
  BackgroundAdapter& operator=(const BackgroundAdapter&) = delete;

  std::vector<mojom::BraveBackgroundPtr> GetBraveBackgrounds();

  std::vector<std::string> GetCustomBackgrounds();

  mojom::SelectedBackgroundPtr GetSelectedBackground();

  mojom::SponsoredImageBackgroundPtr GetSponsoredImageBackground();

  void SelectBackground(mojom::SelectedBackgroundPtr background);

  void SaveCustomBackgrounds(std::vector<base::FilePath> paths,
                             base::OnceClosure callback);

  void RemoveCustomBackground(const std::string& background_url,
                              base::OnceClosure callback);

 private:
  void OnCustomBackgroundsSaved(base::OnceClosure callback,
                                std::vector<base::FilePath> paths);

  void OnCustomBackgroundRemoved(base::OnceClosure callback,
                                 base::FilePath path,
                                 bool success);

  std::unique_ptr<CustomBackgroundFileManager> custom_file_manager_;
  raw_ref<PrefService> pref_service_;
  raw_ptr<ntp_background_images::ViewCounterService> view_counter_service_;
  base::WeakPtrFactory<BackgroundAdapter> weak_factory_{this};
};

}  // namespace brave_new_tab

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_BACKGROUND_ADAPTER_H_
