/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_HANDLER_H_

#include <string>
#include <vector>

#include "brave/browser/psst/psst_ui_delegate_impl.h"
#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"
#include "brave/components/psst/common/psst_ui_common.mojom-shared.h"
#include "brave/components/psst/common/psst_ui_common.mojom.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace psst {

class BravePsstDialogUI;
class PsstTabWebContentsObserver;

class BravePsstDialogHandler : public psst::mojom::PsstConsentHelper,
                               public PsstUiDelegateImpl::Observer,
                               public TabStripModelObserver {
 public:
  explicit BravePsstDialogHandler(
      TabStripModel* tab_strip_model,
      BravePsstDialogUI* dialog_ui,
      mojo::PendingReceiver<psst::mojom::PsstConsentHelper> pending_receiver,
      mojo::PendingRemote<psst::mojom::PsstConsentDialog> client_page);

  BravePsstDialogHandler() = delete;
  BravePsstDialogHandler(const BravePsstDialogHandler&) = delete;
  BravePsstDialogHandler& operator=(const BravePsstDialogHandler&) = delete;

  ~BravePsstDialogHandler() override;

 private:
  friend class PsstTabWebContentsObserverBrowserTest;
  void ApplyChanges(
      const std::string& site_name,
      const std::vector<std::string>& selected_settings_list) override;
  void CloseDialog() override;

  void OnSetRequestDone(const std::string& url,
                        const std::optional<std::string>& error) override;
  void OnSetCompleted(
      const std::optional<std::vector<std::string>>& applied_checks,
      const std::optional<std::vector<std::string>>& errors) override;

  // TabStripModelObserver
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  base::WeakPtr<psst::PsstTabWebContentsObserver> active_tab_helper_;
  base::WeakPtr<PsstUiDelegateImpl> psst_dialog_delegate_;
  raw_ptr<BravePsstDialogUI> const dialog_ui_{nullptr};
  mojo::Receiver<psst::mojom::PsstConsentHelper> receiver_;
  mojo::Remote<psst::mojom::PsstConsentDialog> client_page_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_HANDLER_H_
