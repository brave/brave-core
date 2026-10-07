/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_HANDLER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/browser/psst/psst_tab_web_contents_observer.h"
#include "brave/browser/psst/psst_ui_delegate_impl.h"
#include "brave/components/psst/core/common/psst_ui_common.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
class WebContents;
}  // namespace content

namespace psst {

class BravePsstDialogUI;
class PsstTabWebContentsObserver;
class BravePsstDialogHandler : public psst::mojom::PsstConsentHelper,
                               public PsstUiDelegateImpl::Observer {
 public:
  explicit BravePsstDialogHandler(
      content::WebContents* initiator_web_contents,
      BravePsstDialogUI* dialog_ui,
      mojo::PendingReceiver<psst::mojom::PsstConsentHelper> pending_receiver,
      mojo::PendingRemote<psst::mojom::PsstConsentDialog> client_page,
      psst::mojom::PsstConsentFactory::CreatePsstConsentHandlerCallback
          callback);

  BravePsstDialogHandler() = delete;
  BravePsstDialogHandler(const BravePsstDialogHandler&) = delete;
  BravePsstDialogHandler& operator=(const BravePsstDialogHandler&) = delete;

  ~BravePsstDialogHandler() override;

 private:
  friend class PsstTabWebContentsObserverBrowserTest;
  void PerformPrivacyTuning(
      const std::vector<std::string>& perform_for_uids) override;
  void ReportFailedContent() override;
  void CloseDialog() override;

  void OnSetRequestStatus(const std::string& uid,
                          const std::optional<std::string>& error) override;
  void OnPsstErrorsReportSent() override;

  base::WeakPtr<psst::PsstTabWebContentsObserver> psst_tab_helper_;
  base::WeakPtr<PsstUiDelegateImpl> psst_dialog_delegate_;
  raw_ptr<BravePsstDialogUI> const dialog_ui_{nullptr};
  mojo::Receiver<psst::mojom::PsstConsentHelper> receiver_;
  mojo::Remote<psst::mojom::PsstConsentDialog> client_page_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_HANDLER_H_
