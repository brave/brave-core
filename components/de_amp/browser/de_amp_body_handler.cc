/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_body_handler.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "brave/components/de_amp/browser/de_amp_util.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/page_navigator.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

namespace de_amp {

namespace {

constexpr const char kDeAmpHeaderName[] = "X-Brave-De-AMP";
constexpr size_t kMaxBytesToCheck = 3 * 65536;
constexpr size_t kMaxRedirectHops = 7;

base::Value LoadNavigationChain(const network::ResourceRequest& request) {
  std::optional<std::string> de_amp_header =
      request.headers.GetHeader(kDeAmpHeaderName);
  if (!de_amp_header) {
    return base::Value(
        base::Value::List().Append(base::Value(request.url.spec())));
  }
  auto value = base::JSONReader::Read(*de_amp_header);
  if (!value || !value->is_list()) {
    return base::Value(
        base::Value::List().Append(base::Value(request.url.spec())));
  }
  DCHECK(value->GetList().size() < kMaxRedirectHops);
  return std::move(*value);
}

bool FindUrlInNavigationChain(const GURL& url, const base::Value& chain) {
  const auto& list = chain.GetList();
  for (const auto& entry : list) {
    if (entry.GetString() == url.spec()) {
      return true;
    }
  }
  return false;
}

std::string AddUrlToNavigationChain(const GURL& url, base::Value chain) {
  chain.GetList().Append(url.spec());
  return base::WriteJson(chain).value_or(std::string());
}

}  // namespace

DeAmpBodyHandler::DeAmpBodyHandler(
    const network::ResourceRequest& request,
    const content::WebContents::Getter& wc_getter)
    : request_(request),
      wc_getter_(wc_getter),
      navigation_chain_(LoadNavigationChain(request_)) {}

DeAmpBodyHandler::~DeAmpBodyHandler() = default;

// static
std::unique_ptr<DeAmpBodyHandler> DeAmpBodyHandler::Create(
    const network::ResourceRequest& request,
    const content::WebContents::Getter& wc_getter) {
  auto* contents = wc_getter.Run();
  if (!contents) {
    return nullptr;
  }
  PrefService* prefs =
      user_prefs::UserPrefs::Get(contents->GetBrowserContext());
  if (!base::FeatureList::IsEnabled(de_amp::features::kBraveDeAMP) ||
      !prefs->GetBoolean(kDeAmpPrefEnabled)) {
    return nullptr;
  }

  return base::WrapUnique(new DeAmpBodyHandler(request, wc_getter));
}

bool DeAmpBodyHandler::OnRequest(network::ResourceRequest* request) {
  request->headers.RemoveHeader(kDeAmpHeaderName);
  return true;
}

bool DeAmpBodyHandler::ShouldProcess(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  // Only De-AMP HTML pages.
  std::string mime_type;
  if (!response_head || !response_head->headers ||
      !response_head->headers->GetMimeType(&mime_type) ||
      base::CompareCaseInsensitiveASCII(mime_type, "text/html")) {
    return false;
  }
  *defer = true;
  response_url_ = response_url;
  return navigation_chain_.GetList().size() < kMaxRedirectHops;
}

void DeAmpBodyHandler::OnBeforeSending() {}

void DeAmpBodyHandler::OnComplete() {}

DeAmpBodyHandler::Action DeAmpBodyHandler::OnBodyUpdated(
    const std::string& body,
    bool is_complete) {
  if (bytes_analyzed_ >= kMaxBytesToCheck) {
    return DeAmpBodyHandler::Action::kComplete;
  }

  bytes_analyzed_ = body.size();

  switch (state_) {
    case State::kCheckForAmp:
      if (!CheckIfAmpPage(std::string_view(body).substr(0, kMaxBytesToCheck))) {
        // if we did find AMP, complete the load.
        return DeAmpBodyHandler::Action::kComplete;
      }
      // Amp found, checking for canonical url.
      state_ = State::kFindForCanonicalUrl;
      [[fallthrough]];
    case State::kFindForCanonicalUrl:
      if (MaybeRedirectToCanonicalLink(body)) {
        // Only abort if we know we're successfully going to the canonical URL.
        return DeAmpBodyHandler::Action::kCancel;
      }
      break;
  }

  return (is_complete || (bytes_analyzed_ >= kMaxBytesToCheck))
             ? DeAmpBodyHandler::Action::kComplete
             : DeAmpBodyHandler::Action::kContinue;
}

bool DeAmpBodyHandler::IsTransformer() const {
  return false;
}

void DeAmpBodyHandler::DeAmpBodyHandler::Transform(
    std::string body,
    base::OnceCallback<void(std::string)> on_complete) {
  NOTREACHED();
}

void DeAmpBodyHandler::UpdateResponseHead(
    network::mojom::URLResponseHead* response_head) {}

bool DeAmpBodyHandler::MaybeRedirectToCanonicalLink(const std::string& body) {
  const auto canonical_link =
      FindCanonicalAmpUrl(std::string_view(body).substr(0, kMaxBytesToCheck));
  if (!canonical_link.has_value()) {
    VLOG(2) << __func__ << canonical_link.error();
    return false;
  }

  const GURL canonical_url(canonical_link.value());
  // Validate the found canonical AMP URL
  if (!VerifyCanonicalAmpUrl(canonical_url, response_url_)) {
    VLOG(2) << __func__ << " canonical link verification failed "
            << canonical_url;
    return false;
  }
  // Attempt to go to the canonical URL
  if (!OpenCanonicalURL(canonical_url)) {
    VLOG(2) << __func__ << " failed to open canonical url: " << canonical_url;
    return false;
  }

  VLOG(2) << __func__ << " de-amping and loading " << canonical_url;
  return true;
}

bool DeAmpBodyHandler::OpenCanonicalURL(const GURL& new_url) {
  auto* contents = wc_getter_.Run();

  if (!contents) {
    return false;
  }

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
  if (new_url == request_.referrer ||
      FindUrlInNavigationChain(new_url, navigation_chain_) ||
      FindUrlInNavigationChain(request_.referrer, navigation_chain_)) {
    return false;
  }

  content::OpenURLParams params(
      new_url,
      content::Referrer::SanitizeForRequest(new_url, entry->GetReferrer()),
      contents->GetPrimaryMainFrame()->GetFrameTreeNodeId(),
      WindowOpenDisposition::CURRENT_TAB, ui::PAGE_TRANSITION_CLIENT_REDIRECT,
      false);

  params.initiator_origin = request_.request_initiator;
  params.user_gesture = request_.has_user_gesture;
  auto redirect_chain = request_.navigation_redirect_chain;
  // This is added to check for server redirect loops
  redirect_chain.pop_back();
  params.redirect_chain = std::move(redirect_chain);

  params.extra_headers += base::StrCat(
      {kDeAmpHeaderName, ":",
       AddUrlToNavigationChain(new_url, std::move(navigation_chain_)), "\r\n"});

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<content::WebContents> web_contents,
                        const content::OpenURLParams& params) {
                       if (!web_contents) {
                         return;
                       }
                       web_contents->OpenURL(params,
                                             /*navigation_handle_callback=*/{});
                     },
                     contents->GetWeakPtr(), std::move(params)));
  return true;
}

}  // namespace de_amp
