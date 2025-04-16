/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_CONSENT_HELPER_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_CONSENT_HELPER_HANDLER_H_

#include "brave/browser/ui/brave_browser_window.h"
#include "brave/components/psst/browser/content/psst_tab_helper.h"
#include "brave/components/psst/browser/core/psst_consent_dialog.mojom.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

namespace psst {

class BravePsstDialogUI;

class BravePsstConsentHelperHandler
    : public psst_consent_dialog::mojom::PsstConsentHelper,
    public PsstDialogDelegate::Observer,
    public TabStripModelObserver {
 public:
  explicit BravePsstConsentHelperHandler(
      TabStripModel* tab_strip_model,
      BravePsstDialogUI* dialog_ui,
      mojo::PendingReceiver<psst_consent_dialog::mojom::PsstConsentHelper> pending_receiver,
      mojo::PendingRemote<psst_consent_dialog::mojom::PsstConsentDialog> client_page);

  BravePsstConsentHelperHandler() = delete;
  BravePsstConsentHelperHandler(const BravePsstConsentHelperHandler&) = delete;
  BravePsstConsentHelperHandler& operator=(
      const BravePsstConsentHelperHandler&) = delete;

  ~BravePsstConsentHelperHandler() override;

 private:
  void ApplyChanges(const std::vector<std::string>& selected_settings_list) override;
  void CloseDialog() override;

void OnSetRequestDone(const std::string& url, const std::optional<std::string>& error) override;
void OnSetCompleted(const std::vector<std::string>& applied_checks, const std::vector<std::string>& errors) override;

    // TabStripModelObserver
    void OnTabStripModelChanged(
        TabStripModel* tab_strip_model,
        const TabStripModelChange& change,
        const TabStripSelectionChange& selection) override;

  //raw_ptr<ConstrainedWebDialogDelegate> constrained_web_dlg_delegate_{nullptr};
  raw_ptr<psst::PsstTabHelper> active_tab_helper_{nullptr};
  raw_ptr<PsstDialogDelegate> psst_dialog_delegate_{nullptr};
  raw_ptr<BravePsstDialogUI> const dialog_ui_{nullptr};
  mojo::Receiver<psst_consent_dialog::mojom::PsstConsentHelper> receiver_;
  mojo::Remote<psst_consent_dialog::mojom::PsstConsentDialog> client_page_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_CONSENT_HELPER_HANDLER_H_
