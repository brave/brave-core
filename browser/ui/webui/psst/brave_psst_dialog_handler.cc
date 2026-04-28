/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/brave_psst_dialog_handler.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/notimplemented.h"
#include "brave/browser/psst/psst_ui_delegate_impl.h"
#include "brave/browser/ui/tabs/public/brave_tab_features.h"
#include "brave/browser/ui/webui/psst/brave_psst_dialog_ui.h"
#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/constrained_window/constrained_window_views.h"
#include "content/public/browser/web_contents.h"

namespace psst {

namespace {

base::WeakPtr<psst::PsstTabWebContentsObserver>
GetActivePsstTabHelperFromContext(content::WebContents* web_contents) {
  auto* tab_interface = tabs::TabInterface::GetFromContents(web_contents);
  if (!tab_interface) {
    return nullptr;
  }

  auto* brave_tab_features =
      tabs::BraveTabFeatures::FromTabFeatures(tab_interface->GetTabFeatures());
  if (!brave_tab_features) {
    return nullptr;
  }

  auto* tab_helper = brave_tab_features->psst_web_contents_observer();
  if (!tab_helper) {
    return nullptr;
  }

  return tab_helper->AsWeakPtr();
}

base::WeakPtr<PsstUiDelegateImpl> GetPsstUIDelegate(
    base::WeakPtr<psst::PsstTabWebContentsObserver> tab_helper) {
  if (!tab_helper) {
    return nullptr;
  }

  auto* psst_ui_delegate =
      static_cast<PsstUiDelegateImpl*>(tab_helper->GetPsstUiDelegate());
  if (!psst_ui_delegate) {
    return nullptr;
  }

  return psst_ui_delegate->AsWeakPtr();
}

}  // namespace

BravePsstDialogHandler::BravePsstDialogHandler(
    TabStripModel* tab_strip_model,
    BravePsstDialogUI* dialog_ui,
    mojo::PendingReceiver<psst::mojom::PsstConsentHelper> pending_receiver,
    mojo::PendingRemote<psst::mojom::PsstConsentDialog> client_page,
    psst::mojom::PsstConsentFactory::CreatePsstConsentHandlerCallback callback)
    : tab_strip_model_(tab_strip_model),
      dialog_ui_(dialog_ui),
      receiver_(this, std::move(pending_receiver)),
      client_page_(std::move(client_page)) {
  CHECK(dialog_ui_);
  CHECK(tab_strip_model_);
  tab_strip_model_->AddObserver(this);
  auto* web_contents = tab_strip_model_->GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  active_tab_helper_ = GetActivePsstTabHelperFromContext(web_contents);
  if (!active_tab_helper_) {
    return;
  }

  psst_dialog_delegate_ = GetPsstUIDelegate(active_tab_helper_);
  if (!psst_dialog_delegate_) {
    return;
  }

  psst_dialog_delegate_->AddObserver(this);

  // Set initial dialog data
  std::move(callback).Run(psst_dialog_delegate_->GetShowDialogData());
}

BravePsstDialogHandler::~BravePsstDialogHandler() {
  if (psst_dialog_delegate_) {
    psst_dialog_delegate_->RemoveObserver(this);
  }
  if (tab_strip_model_) {
    tab_strip_model_->RemoveObserver(this);
  }
}

void BravePsstDialogHandler::OnSetRequestStatus(
    const std::string& uid,
    const std::optional<std::string>& error) {
  if (!client_page_ || !client_page_.is_bound() ||
      !client_page_.is_connected()) {
    return;
  }
  client_page_->OnSetRequestStatus(uid, error);
}

void BravePsstDialogHandler::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed() && psst_dialog_delegate_) {
    psst_dialog_delegate_->RemoveObserver(this);
  }

  if (selection.new_contents) {
    active_tab_helper_ =
        GetActivePsstTabHelperFromContext(selection.new_contents);
    if (!active_tab_helper_) {
      return;
    }

    psst_dialog_delegate_ = GetPsstUIDelegate(active_tab_helper_);
    if (!psst_dialog_delegate_) {
      return;
    }
    psst_dialog_delegate_->AddObserver(this);
  }
}

void BravePsstDialogHandler::PerformPrivacyTuning(
    const std::vector<std::string>& perform_for_uids) {
  if (!psst_dialog_delegate_) {
    return;
  }

  psst_dialog_delegate_->OnUserAcceptedPsstSettings(perform_for_uids);
}

void BravePsstDialogHandler::ReportFailedContent() {
  if (!psst_dialog_delegate_) {
    return;
  }

  // Report Submission Implementation
  NOTIMPLEMENTED();
}

void BravePsstDialogHandler::CloseDialog() {
  if (!psst_dialog_delegate_ || !dialog_ui_) {
    return;
  }

  psst_dialog_delegate_->RemoveObserver(this);
  dialog_ui_->Close();
}

}  // namespace psst
