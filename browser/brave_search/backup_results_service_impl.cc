// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_search/backup_results_service_impl.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_search/browser/backup_results_allowed_urls.h"
#include "brave/components/brave_search/browser/backup_results_service.h"
#include "brave/components/brave_search/common/features.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "chrome/browser/content_extraction/inner_html.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "net/cookies/site_for_cookies.h"
#include "services/network/public/cpp/header_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/blink/public/common/navigation/navigation_policy.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"

namespace brave_search {

namespace {

constexpr net::NetworkTrafficAnnotationTag kNetworkTrafficAnnotationTag =
    net::DefineNetworkTrafficAnnotation("brave_search_backup", R"(
      semantics {
        sender: "Brave Search Backup Results Service"
        description:
          "Requests results from a backup search "
          "provider for users that have opted into this feature."
        trigger:
          "Triggered by Brave Search or Web Discovery Project if a user has opted in."
        data:
          "Backup provider results."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: YES
        setting:
          "You can enable or disable these features on brave://settings/search "
          "and https://search.brave.com/settings"
        policy_exception_justification:
          "Not implemented."
      }
    )");

constexpr size_t kMaxResponseSize = 5 * 1024 * 1024;
constexpr base::TimeDelta kTimeout = base::Seconds(5);

class BackupResultsWebContentsObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BackupResultsWebContentsObserver> {
 public:
  void DidFinishNavigation(content::NavigationHandle* controller) override {
    const auto* response_headers = controller->GetResponseHeaders();
    if (!response_headers || !backup_results_service_) {
      return;
    }
    backup_results_service_->HandleWebContentsDidFinishNavigation(
        web_contents(), response_headers->response_code());
  }

  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override {
    if (!validated_url.SchemeIs(url::kHttpsScheme) ||
        !backup_results_service_) {
      return;
    }
    backup_results_service_->HandleWebContentsDidFinishLoad(web_contents());
  }

 private:
  friend class content::WebContentsUserData<BackupResultsWebContentsObserver>;

  BackupResultsWebContentsObserver(
      content::WebContents* web_contents,
      base::WeakPtr<BackupResultsServiceImpl> backup_results_service)
      : WebContentsObserver(web_contents),
        WebContentsUserData<BackupResultsWebContentsObserver>(*web_contents),
        backup_results_service_(backup_results_service) {}

  base::WeakPtr<BackupResultsServiceImpl> backup_results_service_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(BackupResultsWebContentsObserver);

}  // namespace

BackupResultsServiceImpl::BackupResultsServiceImpl(Profile* profile)
    : profile_(profile) {
  profile_->AddObserver(this);
}
BackupResultsServiceImpl::~BackupResultsServiceImpl() = default;

void BackupResultsServiceImpl::FetchBackupResults(
    const GURL& url,
    std::optional<net::HttpRequestHeaders> headers,
    BackupResultsCallback callback) {
  if (!profile_) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  bool should_render =
      !headers || !headers->HasHeader(net::HttpRequestHeaders::kCookie);

  if (should_render) {
    auto* host_content_settings_map =
        HostContentSettingsMapFactory::GetForProfile(profile_);
    if (host_content_settings_map &&
        brave_shields::GetNoScriptControlType(host_content_settings_map, url) ==
            brave_shields::ControlType::BLOCK) {
      std::move(callback).Run(std::nullopt);
      return;
    }
  }

  auto otr_profile_id =
      Profile::OTRProfileID::CreateUniqueForSearchBackupResults();
  auto* otr_profile = profile_->GetOffTheRecordProfile(otr_profile_id, true);

  std::unique_ptr<content::WebContents> web_contents;

  if (should_render) {
    auto create_params = content::WebContents::CreateParams(otr_profile);
    web_contents = content::WebContents::Create(create_params);
    auto web_preferences = web_contents->GetOrCreateWebPreferences();
    web_preferences.supports_multiple_windows = false;
    web_contents->SetWebPreferences(web_preferences);

    if (features::IsBackupResultsFullRenderEnabled()) {
      BackupResultsWebContentsObserver::CreateForWebContents(
          web_contents.get(), weak_ptr_factory_.GetWeakPtr());
    }
  }

  auto request = pending_requests_.emplace(pending_requests_.end(),
                                           std::move(web_contents), headers,
                                           otr_profile, std::move(callback));

  if (should_render) {
    auto load_url_params = content::NavigationController::LoadURLParams(url);
    // Disallow all downloads
    for (size_t i = 0;
         i <= static_cast<size_t>(blink::NavigationDownloadType::kMaxValue);
         i++) {
      blink::NavigationDownloadType type =
          static_cast<blink::NavigationDownloadType>(i);
      load_url_params.download_policy.SetDisallowed(type);
    }
    if (request->headers) {
      load_url_params.extra_headers = request->headers->ToString();
    }
    if (!request->web_contents->GetController().LoadURLWithParams(
            load_url_params)) {
      CleanupAndDispatchResult(request, std::nullopt);
      return;
    }
    request->timeout_timer.Start(
        FROM_HERE, kTimeout,
        base::BindOnce(&BackupResultsServiceImpl::CleanupAndDispatchResult,
                       base::Unretained(this), request, std::nullopt));
  } else {
    MakeSimpleURLLoaderRequest(request, url);
  }
}

BackupResultsServiceImpl::PendingRequest::PendingRequest(
    std::unique_ptr<content::WebContents> web_contents,
    std::optional<net::HttpRequestHeaders> headers,
    Profile* otr_profile,
    BackupResultsCallback callback)
    : headers(std::move(headers)),
      callback(std::move(callback)),
      web_contents(std::move(web_contents)),
      otr_profile(otr_profile) {}

BackupResultsServiceImpl::PendingRequest::~PendingRequest() = default;

BackupResultsServiceImpl::PendingRequestList::iterator
BackupResultsServiceImpl::FindPendingRequest(
    const content::WebContents* web_contents) {
  return base::ranges::find_if(pending_requests_, [&](const auto& request) {
    return request.web_contents.get() == web_contents;
  });
}

bool BackupResultsServiceImpl::HandleWebContentsStartRequest(
    const content::WebContents* web_contents,
    const GURL& url) {
  auto pending_request = FindPendingRequest(web_contents);
  if (pending_request == pending_requests_.end()) {
    return false;
  }
  if (!IsBackupResultURLAllowed(url)) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(&BackupResultsServiceImpl::CleanupAndDispatchResult,
                       weak_ptr_factory_.GetWeakPtr(), pending_request,
                       std::nullopt));
    return false;
  }
  if (features::IsBackupResultsFullRenderEnabled()) {
    return pending_request->requests_loaded <
           static_cast<size_t>(
               features::kBackupResultsFullRenderMaxRequests.Get());
  }
  if (!pending_request->initial_request_started) {
    pending_request->initial_request_started = true;
    return true;
  }

  MakeSimpleURLLoaderRequest(pending_request, url);
  return false;
}

void BackupResultsServiceImpl::HandleWebContentsDidFinishNavigation(
    const content::WebContents* web_contents,
    int response_code) {
  auto pending_request = FindPendingRequest(web_contents);
  if (pending_request == pending_requests_.end()) {
    return;
  }
  pending_request->last_response_code = response_code;
}

void BackupResultsServiceImpl::HandleWebContentsDidFinishLoad(
    const content::WebContents* web_contents) {
  auto pending_request = FindPendingRequest(web_contents);
  if (pending_request == pending_requests_.end()) {
    return;
  }
  pending_request->requests_loaded++;
  if (pending_request->requests_loaded ==
      static_cast<size_t>(
          features::kBackupResultsFullRenderMaxRequests.Get())) {
    content_extraction::GetInnerHtml(
        *pending_request->web_contents->GetPrimaryMainFrame(),
        base::BindOnce(
            &BackupResultsServiceImpl::HandleWebContentsContentExtraction,
            weak_ptr_factory_.GetWeakPtr(), pending_request));
  }
}

base::WeakPtr<BackupResultsService> BackupResultsServiceImpl::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void BackupResultsServiceImpl::MakeSimpleURLLoaderRequest(
    PendingRequestList::iterator pending_request,
    const GURL& url) {
  pending_request->timeout_timer.Stop();
  pending_request->shared_url_loader_factory =
      pending_request->otr_profile->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess();

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;

  if (pending_request->headers &&
      pending_request->headers->HasHeader(net::HttpRequestHeaders::kCookie)) {
    resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  } else {
    resource_request->credentials_mode =
        network::mojom::CredentialsMode::kInclude;
    resource_request->site_for_cookies = net::SiteForCookies::FromUrl(url);
  }

  if (pending_request->headers) {
    resource_request->headers = *pending_request->headers;
  }

  pending_request->simple_url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), kNetworkTrafficAnnotationTag);
  pending_request->simple_url_loader->DownloadToString(
      pending_request->shared_url_loader_factory.get(),
      base::BindOnce(&BackupResultsServiceImpl::HandleURLLoaderResponse,
                     base::Unretained(this), pending_request),
      kMaxResponseSize);
}

void BackupResultsServiceImpl::HandleURLLoaderResponse(
    PendingRequestList::iterator pending_request,
    std::optional<std::string> html) {
  auto* response_info = pending_request->simple_url_loader->ResponseInfo();

  std::optional<BackupResults> result;
  if (html && pending_request->simple_url_loader->NetError() == net::OK &&
      response_info && response_info->headers) {
    auto response_code = response_info->headers->response_code();
    result = BackupResults(response_code, *html);
  }

  CleanupAndDispatchResult(pending_request, result);
}

void BackupResultsServiceImpl::HandleWebContentsContentExtraction(
    PendingRequestList::iterator pending_request,
    const std::optional<std::string>& content) {
  std::optional<BackupResults> result;
  if (content) {
    result = BackupResults(
        pending_request->last_response_code,
        base::StrCat({"<!doctype html><html>", *content, "</html>"}));
  }

  CleanupAndDispatchResult(pending_request, result);
}

void BackupResultsServiceImpl::CleanupAndDispatchResult(
    PendingRequestList::iterator pending_request,
    std::optional<BackupResults> result) {
  auto* otr_profile = pending_request->otr_profile.get();

  std::move(pending_request->callback).Run(result);
  pending_requests_.erase(pending_request);

  if (profile_) {
    profile_->DestroyOffTheRecordProfile(otr_profile);
  }
}

void BackupResultsServiceImpl::OnProfileWillBeDestroyed(Profile* profile) {
  Shutdown();
}

void BackupResultsServiceImpl::Shutdown() {
  if (profile_) {
    profile_->RemoveObserver(this);
    for (auto& request : pending_requests_) {
      request.web_contents = nullptr;
      auto* otr_profile = request.otr_profile.get();
      request.otr_profile = nullptr;
      profile_->DestroyOffTheRecordProfile(otr_profile);
    }
    pending_requests_.clear();
    profile_ = nullptr;
  }

  weak_ptr_factory_.InvalidateWeakPtrs();
}

}  // namespace brave_search
