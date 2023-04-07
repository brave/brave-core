/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_throttle.h"

#include <utility>

#include "base/feature_list.h"
#include "base/strings/stringprintf.h"
#include "brave/components/de_amp/browser/de_amp_url_loader.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/base/net_errors.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

namespace de_amp {

constexpr char kDeAmpHeaderName[] = "X-Brave-De-AMP";

// static
std::unique_ptr<DeAmpThrottle> DeAmpThrottle::MaybeCreateThrottleFor(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const network::ResourceRequest& request,
    const content::WebContents::Getter& wc_getter) {
  auto* contents = wc_getter.Run();

  if (!contents)
    return nullptr;

  PrefService* prefs =
      user_prefs::UserPrefs::Get(contents->GetBrowserContext());

  if (!base::FeatureList::IsEnabled(de_amp::features::kBraveDeAMP) ||
      !prefs->GetBoolean(kDeAmpPrefEnabled)) {
    return nullptr;
  }

  return std::make_unique<DeAmpThrottle>(task_runner, request, wc_getter);
}

DeAmpThrottle::DeAmpThrottle(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const network::ResourceRequest& request,
    const content::WebContents::Getter& wc_getter)
    : task_runner_(task_runner),
      request_(request),
      is_amp_redirect_(false),
      wc_getter_(wc_getter) {}

DeAmpThrottle::~DeAmpThrottle() = default;

void DeAmpThrottle::WillStartRequest(network::ResourceRequest* request,
                                     bool* defer) {
  if (request->headers.HasHeader(kDeAmpHeaderName)) {
    is_amp_redirect_ = true;
    request->headers.RemoveHeader(kDeAmpHeaderName);
  }
}

void DeAmpThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  if (is_amp_redirect_)
    return;

  VLOG(2) << "deamp throttling: " << response_url;
  *defer = true;

  mojo::PendingRemote<network::mojom::URLLoader> new_remote;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver;
  DeAmpURLLoader* de_amp_loader;
  std::tie(new_remote, new_receiver, de_amp_loader) =
      DeAmpURLLoader::CreateLoader(weak_factory_.GetWeakPtr(), response_url,
                                   task_runner_);
  InterceptAndStartLoader(std::move(new_remote), std::move(new_receiver),
                          de_amp_loader);
}

bool DeAmpThrottle::OpenCanonicalURL(const GURL& new_url,
                                     const GURL& response_url) {
  auto* contents = wc_getter_.Run();

  if (!contents)
    return false;

  // The pending entry is the one in progress i.e. the AMP link
  // The visible entry is the one visible in the address bar. If the AMP link
  // was clicked on a page, then this will be that page. If it's a direct
  // navigation, visible entry will be the same as pending entry.
  auto* entry = contents->GetController().GetPendingEntry();
  if (!entry) {
    if (contents->GetController().GetVisibleEntry()) {
      entry = contents->GetController().GetVisibleEntry();
    } else {
      return false;
    }
  }

  // If we've already navigated to the canonical URL last time, we
  // should stop De-AMPing. This is done to prevent redirect loops.
  // https://github.com/brave/brave-browser/issues/22610
  bool new_url_same_as_last_committed =
      contents->GetController().GetLastCommittedEntry()
          ? contents->GetController().GetLastCommittedEntry()->GetURL() ==
                new_url
          : false;
  if (new_url_same_as_last_committed)
    return false;

  delegate_->CancelWithError(net::ERR_ABORTED);

  content::OpenURLParams params(
      new_url,
      content::Referrer::SanitizeForRequest(new_url, entry->GetReferrer()),
      contents->GetPrimaryMainFrame()->GetFrameTreeNodeId(),
      WindowOpenDisposition::CURRENT_TAB, ui::PAGE_TRANSITION_CLIENT_REDIRECT,
      false);

  params.initiator_origin = request_.request_initiator;
  params.user_gesture = request_.has_user_gesture;
  auto redirect_chain = request_.navigation_redirect_chain;
  redirect_chain.pop_back();
  params.redirect_chain = std::move(redirect_chain);
  // This is added to check for server redirect loops
  params.extra_headers += base::StringPrintf("%s: true\r\n", kDeAmpHeaderName);

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<content::WebContents> web_contents,
                        const content::OpenURLParams& params) {
                       if (!web_contents)
                         return;
                       web_contents->OpenURL(params);
                     },
                     contents->GetWeakPtr(), std::move(params)));
  return true;
}

}  // namespace de_amp
