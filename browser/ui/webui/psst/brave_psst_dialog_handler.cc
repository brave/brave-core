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
#include "base/values.h"
#include "brave/browser/psst/psst_ui_delegate_impl.h"
#include "brave/browser/ui/tabs/public/brave_tab_features.h"
#include "brave/browser/ui/webui/psst/brave_psst_dialog_ui.h"
#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"
#include "brave/components/psst/common/psst_ui_common.mojom-shared.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/web_contents.h"

namespace psst {

namespace {

// inline constexpr char kUserScriptResultTaskItemUrlPropName[] = "url";
// inline constexpr char kUserScriptResultTaskItemDescPropName[] =
// "description";

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

base::WeakPtr<psst::PsstTabWebContentsObserver> GetActivePsstTabHelperFromContext(
    content::WebContents* web_contents) {
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
  if(!tab_helper) {
    return nullptr;
  }

  return tab_helper->AsWeakPtr();
}

base::WeakPtr<PsstUiDelegateImpl> GetPsstUIDelegate(base::WeakPtr<psst::PsstTabWebContentsObserver> tab_helper) {
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
    mojo::PendingRemote<psst::mojom::PsstConsentDialog> client_page)
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
  if (!active_tab_helper_) {
    return;
  }

  psst_dialog_delegate_ = GetPsstUIDelegate(active_tab_helper_);
  if(!psst_dialog_delegate_) {
    return;
  }

  psst_dialog_delegate_->AddObserver(this);

  auto setting_card_data = psst_dialog_delegate_->GetShowDialogData();
  CHECK(setting_card_data);
  // Set initial dialog data
  client_page_->SetSettingsCardData(std::move(setting_card_data));
}

BravePsstDialogHandler::~BravePsstDialogHandler() = default;

void BravePsstDialogHandler::OnSetRequestDone(
    const std::string& url,
    const std::optional<std::string>& error) {
  if (!client_page_) {
    return;
  }
  client_page_->OnSetRequestDone(url, error);
}

void BravePsstDialogHandler::OnSetCompleted(
    const std::optional<std::vector<std::string>>& applied_checks,
    const std::optional<std::vector<std::string>>& errors) {
  if (!client_page_) {
    return;
  }
  client_page_->OnSetCompleted(applied_checks, errors);
}

void BravePsstDialogHandler::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  // TODO handle the Tab changing
  //  We need to switch handler to the new active tab
  if (selection.active_tab_changed() && psst_dialog_delegate_) {
    psst_dialog_delegate_->RemoveObserver(this);
  }

  if (selection.new_contents) {
    active_tab_helper_ =
        GetActivePsstTabHelperFromContext(selection.new_contents);
    if(!active_tab_helper_) {
      return;
    }

    psst_dialog_delegate_ = GetPsstUIDelegate(active_tab_helper_);
    if (!psst_dialog_delegate_) {
      return;
    }
    psst_dialog_delegate_->AddObserver(this);
  }
}

void BravePsstDialogHandler::ApplyChanges(const std::string& site_name,
    const std::vector<std::string>& selected_settings_list) {

  if (!psst_dialog_delegate_) {
    return;
  }

  base::ListValue list;
  for(const auto& setting : selected_settings_list) {
    list.Append(setting);
  }
LOG(INFO) << "[PSST] ApplyChanges called for site: " << site_name 
  << " url:" << GURL(site_name)
  << " origin:" << url::Origin::Create(GURL(site_name)).Serialize()
;
  psst_dialog_delegate_->OnUserAcceptedPsstSettings(
      url::Origin::Create(GURL(site_name)),
      std::move(list));
}

void BravePsstDialogHandler::CloseDialog() {
  CHECK(active_tab_helper_);
  if (psst_dialog_delegate_) {
    psst_dialog_delegate_->RemoveObserver(this);
  }
  ::psst::CloseDialog(active_tab_helper_->web_contents());
}

}  // namespace psst
