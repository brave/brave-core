/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/ipfs_ui.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ipfs/ipfs_service.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/ipfs_ui/resources/grit/ipfs_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_ui_data_source.h"

IPFSDOMHandler::IPFSDOMHandler() : weak_ptr_factory_{this} {}

IPFSDOMHandler::~IPFSDOMHandler() {}

void IPFSDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "ipfs.getConnectedPeers",
      base::BindRepeating(&IPFSDOMHandler::HandleGetConnectedPeers,
                          base::Unretained(this)));
}

IPFSUI::IPFSUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kIpfsGenerated,
        kIpfsGeneratedSize, IDR_IPFS_HTML) {
  Profile* profile = Profile::FromWebUI(web_ui);
  PrefService* prefs = profile->GetPrefs();
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(prefs);
  // pref_change_registrar_->Add(kAdsBlocked,
  //  base::Bind(&IPFSUI::OnPreferenceChanged, base::Unretained(this)));
  web_ui->AddMessageHandler(std::make_unique<IPFSDOMHandler>());
}

IPFSUI::~IPFSUI() {
}

void IPFSUI::CustomizeWebUIProperties(
    content::RenderFrameHost* render_frame_host) {
  DCHECK(IsSafeToSetWebUIProperties());
  /*
  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* prefs = profile->GetPrefs();
  if (render_frame_host) {
    render_frame_host->SetWebUIProperty(
        "adsBlockedStat", std::to_string(prefs->GetUint64(kAdsBlocked) +
            prefs->GetUint64(kTrackersBlocked)));
  }
  */
}

void IPFSUI::UpdateWebUIProperties() {
  if (IsSafeToSetWebUIProperties()) {
    CustomizeWebUIProperties(GetRenderFrameHost());
    // web_ui()->CallJavascriptFunctionUnsafe("ipfs.statsUpdated");
  }
}

void IPFSUI::OnPreferenceChanged() {
  UpdateWebUIProperties();
}

void IPFSDOMHandler::HandleGetConnectedPeers(const base::ListValue* args) {
  DCHECK_EQ(args->GetSize(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetForContext(Profile::FromWebUI(web_ui()));
  if (!service) {
    return;
  }

  service->GetConnectedPeers(
      base::BindOnce(&IPFSDOMHandler::OnGetConnectedPeers,
                     weak_ptr_factory_.GetWeakPtr()));
}

void IPFSDOMHandler::OnGetConnectedPeers(bool success,
    const std::vector<std::string>& peers) {
  if (!web_ui()->CanCallJavascript() || !success)
    return;

  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetConnectedPeers",
      base::Value(static_cast<int>(peers.size())));
}
