/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_body_handler.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "brave/components/de_amp/browser/de_amp_util.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/page_navigator.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

namespace de_amp {

namespace {

constexpr const char kDeAmpHeaderName[] = "X-Brave-De-AMP";
constexpr size_t kMaxBytesToCheck = 3 * 64 * 1024;
constexpr size_t kMaxRedirectHops = 7;

std::string SaveRedirectChain(const std::vector<GURL>& chain) {
  base::Value::List list;
  for (const auto& url : chain) {
    if (url.is_valid()) {
      list.Append(url.spec());
    }
    if (list.size() >= kMaxRedirectHops) {
      break;
    }
  }
  auto result = base::WriteJson(base::Value(std::move(list)));
  return result.value_or(std::string());
}

std::vector<GURL> LoadRedirectChain(const std::string& header) {
  auto value = base::JSONReader::Read(header);
  if (!value || !value->is_list()) {
    return {};
  }
  DCHECK(value->GetList().size() < kMaxRedirectHops);

  std::vector<GURL> chain;
  chain.reserve(value->GetList().size());
  for (const auto& entry : value->GetList()) {
    if (!entry.is_string()) {
      continue;
    }
    chain.push_back(GURL(entry.GetString()));
  }
  return chain;
}

bool FindUrlInRedirectChain(const GURL& url,
                            const network::ResourceRequest& request) {
  std::string header;
  if (!request.headers.GetHeader(kDeAmpHeaderName, &header)) {
    return false;
  }
  const auto chain = LoadRedirectChain(header);
  return std::find_if(chain.begin(), chain.end(), [&url](const auto& v) {
           return v == url;
         }) != chain.end();
}

}  // namespace

DeAmpBodyHandler::DeAmpBodyHandler(
    const network::ResourceRequest& request,
    const content::WebContents::Getter& wc_getter)
    : request_(request), wc_getter_(wc_getter) {}

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
    network::mojom::URLResponseHead* response_head) {
  response_url_ = response_url;
  return true;
}

void DeAmpBodyHandler::OnComplete() {}

DeAmpBodyHandler::Action DeAmpBodyHandler::OnBodyUpdated(
    const std::string& body,
    bool is_complete) {
  if (found_amp_ || bytes_analyzed_ >= kMaxBytesToCheck) {
    return DeAmpBodyHandler::Action::kComplete;
  }

  // If we are not already on an AMP page, check if this chunk has the AMP HTML
  found_amp_ = CheckIfAmpPage(body);
  bytes_analyzed_ = body.size();

  if (found_amp_ && MaybeRedirectToCanonicalLink(body)) {
    // Only abort if we know we're successfully going to the canonical URL
    return DeAmpBodyHandler::Action::kCancel;
  }

  return is_complete ? DeAmpBodyHandler::Action::kComplete
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
    network::mojom::URLResponseHead* response_head) {
  NOTREACHED();
}

bool DeAmpBodyHandler::MaybeRedirectToCanonicalLink(const std::string& body) {
  auto canonical_link = FindCanonicalAmpUrl(body);
  if (!canonical_link.has_value()) {
    VLOG(2) << __func__ << canonical_link.error();
    return false;
  }

  bool redirected = false;
  const GURL canonical_url(canonical_link.value());
  // Validate the found canonical AMP URL
  if (VerifyCanonicalAmpUrl(canonical_url, response_url_)) {
    // Attempt to go to the canonical URL
    VLOG(2) << __func__ << " de-amping and loading " << canonical_url;
    if (OpenCanonicalURL(canonical_url)) {
      redirected = true;
    } else {
      VLOG(2) << __func__ << " failed to open canonical url: " << canonical_url;
    }
  } else {
    VLOG(2) << __func__ << " canonical link verification failed "
            << canonical_url;
  }
  // At this point we've either redirected, or we should stop trying
  found_amp_ = false;
  return redirected;
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
      FindUrlInRedirectChain(request_.referrer, request_) ||
      FindUrlInRedirectChain(new_url, request_)) {
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
  params.extra_headers += base::StrCat(
      {kDeAmpHeaderName, ":", SaveRedirectChain(redirect_chain), "\r\n"});

  redirect_chain.pop_back();
  params.redirect_chain = std::move(redirect_chain);

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<content::WebContents> web_contents,
                        const content::OpenURLParams& params) {
                       if (!web_contents) {
                         return;
                       }
                       web_contents->OpenURL(params);
                     },
                     contents->GetWeakPtr(), std::move(params)));
  return true;
}

}  // namespace de_amp
