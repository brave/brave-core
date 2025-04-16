/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/brave_psst_consent_helper_handler.h"

#include "base/values.h"
#include "brave/browser/ui/tabs/public/tab_features.h"
#include "brave/components/psst/browser/content/psst_tab_helper.h"
#include "brave/components/psst/browser/core/psst_consent_dialog.mojom-forward.h"
#include "brave/components/psst/common/psst_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/public/tab_interface.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "brave/browser/ui/webui/psst/brave_psst_dialog_ui.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/constrained_window/constrained_window_views.h"
#include "content/public/browser/web_contents.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/aura/window_delegate.h"
#include "ui/views/widget/widget.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"

namespace psst {

namespace {
void CloseDialog(content::WebContents* initiator_contents) {
  DCHECK(initiator_contents);
  auto* top_level_web_contents =
      constrained_window::GetTopLevelWebContents(initiator_contents);
  if (!top_level_web_contents) {
    return;
  }

  auto* manager = web_modal::WebContentsModalDialogManager::FromWebContents(
      top_level_web_contents);
  if (!manager) {
    return;
  }

  manager->CloseAllDialogs();
}

psst::PsstTabHelper* GetActivePsstTabHelperFromContext(
    content::WebContents* web_contents) {
  auto* tab_interface = tabs::TabInterface::GetFromContents(web_contents);
  if (!tab_interface) {
    return nullptr;
  }

  return tab_interface->GetTabFeatures()->GetPsstTabHelper();
}

}  // namespace

BravePsstConsentHelperHandler::BravePsstConsentHelperHandler(
    TabStripModel* tab_strip_model,
    BravePsstDialogUI* dialog_ui,
    mojo::PendingReceiver<psst_consent_dialog::mojom::PsstConsentHelper>
        pending_receiver,
    mojo::PendingRemote<psst_consent_dialog::mojom::PsstConsentDialog>
        client_page)
    : dialog_ui_(dialog_ui),
      receiver_(this, std::move(pending_receiver)),
      client_page_(std::move(client_page)) {
  DCHECK(tab_strip_model);
  tab_strip_model->AddObserver(this);
  auto* web_contents = tab_strip_model->GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  active_tab_helper_ = GetActivePsstTabHelperFromContext(web_contents);
  if(!active_tab_helper_) {
    return;
  }

  psst_dialog_delegate_ = active_tab_helper_->GetPsstDialogDelegate();
  if(!psst_dialog_delegate_) {
    return;
  }

  psst_dialog_delegate_->AddObserver(this);

  auto* sdd = psst_dialog_delegate_->GetShowDialogData();
  if(!sdd) {
    return;
  }

  auto scd = psst_consent_dialog::mojom::SettingCardData::New();
  scd->site_name = sdd->site_name;

  for (auto& task_item : sdd->request_infos) {
    if (!task_item.is_dict()) {
      continue;
    }

    const auto& item_dict = task_item.GetDict();
    const auto* description =
        item_dict.FindString(kUserScriptResultTaskItemDescPropName);
    const auto* url =
        item_dict.FindString(kUserScriptResultTaskItemUrlPropName);

    if (description && url) {
      scd->items.push_back(psst_consent_dialog::mojom::SettingCardDataItem::New(
          *description, *url));
    }
  }
  client_page_->SetSettingsCardData(std::move(scd));
}

BravePsstConsentHelperHandler::~BravePsstConsentHelperHandler() = default;

void BravePsstConsentHelperHandler::OnSetRequestDone(const std::string& url, const std::optional<std::string>& error){
  if (!client_page_) {
    return;
  }
  client_page_->OnSetRequestDone(url, error);
}

void BravePsstConsentHelperHandler::OnSetCompleted(const std::vector<std::string>& applied_checks, const std::vector<std::string>& errors) {
  if (!client_page_) {
    return;
  }
//  LOG(INFO) << "[PSST] BravePsstConsentHelperHandler::OnSetRequestDone applied_checks:" << applied_checks.size() << " errors:" << errors.size();
  client_page_->OnSetCompleted(applied_checks, errors);
}

void BravePsstConsentHelperHandler::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  // TODO handle the Tab changing
  //  We need to switch handler to the new active tab
  if (selection.active_tab_changed()) {
    psst_dialog_delegate_->RemoveObserver(this);
    psst_dialog_delegate_ = nullptr;
    active_tab_helper_ = nullptr;
  }

  if (selection.new_contents) {
    active_tab_helper_ = GetActivePsstTabHelperFromContext(selection.new_contents);
    
    psst_dialog_delegate_ = active_tab_helper_->GetPsstDialogDelegate();;
    if(!psst_dialog_delegate_) {
      return;
    }
    psst_dialog_delegate_->AddObserver(this);
  }
}

void BravePsstConsentHelperHandler::ApplyChanges(const std::vector<std::string>& selected_settings_list) {
  auto* sdd = psst_dialog_delegate_->GetShowDialogData();
  if(!sdd) {
    return;
  }

  std::move(sdd->apply_changes_callback).Run(selected_settings_list);
}

void BravePsstConsentHelperHandler::CloseDialog() {
  DCHECK(active_tab_helper_);
  psst_dialog_delegate_->RemoveObserver(this);
  ::psst::CloseDialog(active_tab_helper_->web_contents());
}


}  // namespace psst
