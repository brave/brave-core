/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include <string>
#include <vector>

#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/shell_integration.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"

namespace {
// Sets current executable as default protocol handler in a system.
void SetupIPFSProtocolHandler(const std::string& protocol) {
  auto isDefaultCallback = [](const std::string& protocol,
                              shell_integration::DefaultWebClientState state) {
    if (state == shell_integration::IS_DEFAULT) {
      VLOG(1) << protocol << " already has a handler";
      return;
    }
    VLOG(1) << "Set as default handler for " << protocol;
    // The worker pointer is reference counted. While it is running, the
    // sequence it runs on will hold references it will be automatically
    // freed once all its tasks have finished.
    base::MakeRefCounted<shell_integration::DefaultProtocolClientWorker>(
        protocol)
        ->StartSetAsDefault(base::NullCallback());
  };

  base::MakeRefCounted<shell_integration::DefaultProtocolClientWorker>(protocol)
      ->StartCheckIsDefault(base::BindOnce(isDefaultCallback, protocol));
}
}  // namespace

namespace ipfs {

IPFSTabHelper::~IPFSTabHelper() = default;

IPFSTabHelper::IPFSTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  pref_service_ = user_prefs::UserPrefs::Get(web_contents->GetBrowserContext());
}

// static
bool IPFSTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  if (!ipfs::IpfsServiceFactory::GetForContext(
          web_contents->GetBrowserContext())) {
    return false;
  }

  CreateForWebContents(web_contents);
  return true;
}

void IPFSTabHelper::DidFinishNavigation(content::NavigationHandle* handle) {
  DCHECK(handle);
  if (!handle->IsInMainFrame()) {
    return;
  }

  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      pref_service_->GetInteger(kIPFSResolveMethod));
  auto* browser_context = web_contents()->GetBrowserContext();
  if (resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_ASK &&
      handle->GetResponseHeaders() &&
      handle->GetResponseHeaders()->HasHeader("x-ipfs-path") &&
      IsDefaultGatewayURL(handle->GetURL(), browser_context)) {
    auto infobar_count = pref_service_->GetInteger(kIPFSInfobarCount);
    if (!infobar_count) {
      pref_service_->SetInteger(kIPFSInfobarCount, infobar_count + 1);
      SetupIPFSProtocolHandler(ipfs::kIPFSScheme);
      SetupIPFSProtocolHandler(ipfs::kIPNSScheme);
    }
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(IPFSTabHelper)

}  // namespace ipfs
