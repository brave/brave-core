// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_PAGE_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"
#include "brave/components/brave_new_tab_ui/brave_new_tab_page.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace base {
class FilePath;
}  // namespace base

namespace content {
class WebContents;
}  // namespace content

class CustomBackgroundFileManager;
class NtpCustomBackgroundService;
class Profile;

class BraveNewTabPageHandler : public brave_new_tab_page::mojom::PageHandler,
                               public ui::SelectFileDialog::Listener,
                               public TemplateURLServiceObserver {
 public:
  BraveNewTabPageHandler(
      mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandler>
          pending_page_handler,
      mojo::PendingRemote<brave_new_tab_page::mojom::Page> pending_page,
      Profile* profile,
      content::WebContents* web_contents);
  ~BraveNewTabPageHandler() override;

  BraveNewTabPageHandler(const BraveNewTabPageHandler&) = delete;
  BraveNewTabPageHandler& operator=(const BraveNewTabPageHandler&) = delete;

 private:
  // brave_new_tab_page::mojom::PageHandler overrides:
  void ChooseLocalCustomBackground() override;
  void UseCustomImageBackground(
      const std::string& selected_background) override;
  void GetCustomImageBackgrounds(
      GetCustomImageBackgroundsCallback callback) override;
  void RemoveCustomImageBackground(const std::string& background) override;

  void UseBraveBackground(const std::string& selected_background) override;
  void GetBraveBackgrounds(GetBraveBackgroundsCallback callback) override;
  void TryBraveSearchPromotion(const std::string& input,
                               bool open_new_tab) override;
  void DismissBraveSearchPromotion() override;
  void IsSearchPromotionEnabled(
      IsSearchPromotionEnabledCallback callback) override;
  void UseColorBackground(const std::string& color,
                          bool use_random_color) override;
  void GetSearchEngines(GetSearchEnginesCallback callback) override;
  void SearchWhatYouTyped(const std::string& host,
                          const std::string& query,
                          bool alt_key,
                          bool ctrl_key,
                          bool meta_key,
                          bool shift_key) override;
  void RefreshVPNState() override;
  void LaunchVPNPanel() override;
  void OpenVPNAccountPage() override;
  void ReportVPNWidgetUsage() override;

  // Observe BraveNTPCustomBackgroundService.
  void OnBackgroundUpdated();
  void OnCustomImageBackgroundsUpdated();

  // SelectFileDialog::Listener overrides:
  void FileSelected(const ui::SelectedFileInfo& file, int index) override;
  void MultiFilesSelected(
      const std::vector<ui::SelectedFileInfo>& files) override;
  void FileSelectionCanceled() override;

  // TemplateURLServiceObserver overrides:
  void OnTemplateURLServiceChanged() override;
  void OnTemplateURLServiceShuttingDown() override;

  bool IsCustomBackgroundImageEnabled() const;
  bool IsColorBackgroundEnabled() const;
  void OnSavedCustomImage(const base::FilePath& path);
  void OnRemoveCustomImageBackground(const base::FilePath& path, bool success);

  void OnSearchPromotionDismissed();
  void NotifySearchPromotionDisabledIfNeeded() const;
  void InitForSearchPromotion();

  PrefChangeRegistrar pref_change_registrar_;
  base::ScopedObservation<TemplateURLService, TemplateURLServiceObserver>
      template_url_service_observation_{this};
  mojo::Receiver<brave_new_tab_page::mojom::PageHandler> page_handler_;
  mojo::Remote<brave_new_tab_page::mojom::Page> page_;
  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<content::WebContents> web_contents_ = nullptr;

  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  std::unique_ptr<CustomBackgroundFileManager> file_manager_;

  base::WeakPtrFactory<BraveNewTabPageHandler> weak_factory_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_PAGE_HANDLER_H_
