// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_search/backup_results_service_impl.h"

#include <algorithm>
#include <utility>

#include "base/byte_count.h"
#include "base/functional/bind.h"
#include "brave/components/brave_search/browser/backup_results_allowed_urls.h"
#include "brave/components/brave_search/browser/backup_results_service.h"
#include "brave/components/brave_search/common/features.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_extraction/inner_html.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "net/cookies/site_for_cookies.h"
#include "services/network/public/cpp/header_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/url_constants.h"

#if BUILDFLAG(IS_ANDROID)
#include "ui/android/view_android.h"
#endif

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

constexpr base::ByteCount kMaxResponseSize = base::MiB(5);
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
    : profile_(profile),
      backup_results_metrics_(g_browser_process->local_state()) {
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

  std::unique_ptr<content::WebContents> web_contents_unique;
  std::unique_ptr<tabs::TabModel> tab_model;

#if BUILDFLAG(IS_ANDROID)
  auto view_android = std::make_unique<ui::ViewAndroid>();
#endif
  if (should_render) {
    auto create_params = content::WebContents::CreateParams(otr_profile);

#if BUILDFLAG(IS_ANDROID)
    // Create a temporary ViewAndroid to configure frame info
    view_android->UpdateFrameInfo(ui::FrameInfo{
        .viewport_size = gfx::SizeF(360., 488.), .content_offset = 0});
    create_params.context = view_android.get();

    LOG(ERROR) << "FetchBackupResults! ViewAndroid: "
               << create_params.context->viewport_size().width() << "x"
               << create_params.context->viewport_size().height();
#endif

    web_contents_unique = content::WebContents::Create(create_params);

    // int random_width = base::RandInt(800, 1920);
    // int random_height = base::RandInt(600, 1080);
    web_contents_unique->Resize({1360, 888});

    auto* browser_list = BrowserList::GetInstance();
    if (!browser_list->empty()) {
      auto* browser = browser_list->get(0);
      auto* model = browser->tab_strip_model();
      tab_model = std::make_unique<tabs::TabModel>(
          std::move(web_contents_unique), model);
      tab_model->OnAddedToModel(model);
    }

    if (features::IsBackupResultsFullRenderEnabled() && tab_model) {
      BackupResultsWebContentsObserver::CreateForWebContents(
          tab_model->GetContents(), weak_ptr_factory_.GetWeakPtr());
    }

    if (tab_model) {
      LOG(ERROR) << "FetchBackupResults! " << url.spec();
      auto size = tab_model->GetContents()->GetSize();
      LOG(ERROR) << "FetchBackupResults! Size: " << size.width() << "x"
                 << size.height();
      LOG(ERROR) << "is render widget host view present: "
                 << (tab_model->GetContents()->GetRenderWidgetHostView() !=
                     nullptr);
      size = tab_model->GetContents()->GetSize();
      LOG(ERROR) << "FetchBackupResults! Size2: " << size.width() << "x"
                 << size.height();
    }
    // auto visible_bounds =
    // web_contents->GetRenderWidgetHostView()->GetVisibleViewportSize();
    // LOG(ERROR) << "FetchBackupResults! Visible Bounds: " <<
    // visible_bounds.width() << "x" << visible_bounds.height();
  }

  auto request =
      pending_requests_.emplace(pending_requests_.end(), std::move(tab_model),
                                headers, otr_profile, std::move(callback), url
#if BUILDFLAG(IS_ANDROID)
                                ,
                                std::move(view_android)
#endif
      );

  if (should_render) {
    // Navigate to brave.com first, then we'll navigate to the original URL
    // after 3 seconds
    auto load_url_params =
        content::NavigationController::LoadURLParams(GURL("https://brave.com"));
    // Disallow all downloads
    // for (size_t i = 0;
    //      i <= static_cast<size_t>(blink::NavigationDownloadType::kMaxValue);
    //      i++) {
    //   blink::NavigationDownloadType type =
    //       static_cast<blink::NavigationDownloadType>(i);
    //   load_url_params.download_policy.SetDisallowed(type);
    // }
    // if (request->headers) {
    //   load_url_params.extra_headers = request->headers->ToString();
    // }
    if (!request->tab_model ||
        !request->tab_model->GetContents()->GetController().LoadURLWithParams(
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
    std::unique_ptr<tabs::TabModel> tab_model,
    std::optional<net::HttpRequestHeaders> headers,
    Profile* otr_profile,
    BackupResultsCallback callback,
    const GURL& original_url
#if BUILDFLAG(IS_ANDROID)
    ,
    std::unique_ptr<ui::ViewAndroid> view_android
#endif
    )
    : headers(std::move(headers)),
      callback(std::move(callback)),
      original_url(original_url),
      tab_model(std::move(tab_model)),
      otr_profile(otr_profile)
#if BUILDFLAG(IS_ANDROID)
      ,
      view_android(std::move(view_android))
#endif
{
}

BackupResultsServiceImpl::PendingRequest::~PendingRequest() = default;

BackupResultsServiceImpl::PendingRequestList::iterator
BackupResultsServiceImpl::FindPendingRequest(
    const content::WebContents* web_contents) {
  return std::ranges::find_if(pending_requests_, [&](const auto& request) {
    return request.tab_model &&
           request.tab_model->GetContents() == web_contents;
  });
}

bool BackupResultsServiceImpl::HandleWebContentsStartRequest(
    const content::WebContents* web_contents,
    const GURL& url) {
  auto pending_request = FindPendingRequest(web_contents);
  if (pending_request == pending_requests_.end()) {
    return false;
  }

  // Check if this is the brave.com request
  if (url.host() == "brave.com") {
    // Start 3-second timer to navigate to the original URL
    pending_request->brave_com_timer.Start(
        FROM_HERE, base::Seconds(3),
        base::BindOnce(&BackupResultsServiceImpl::NavigateToOriginalUrl,
                       base::Unretained(this), pending_request));
    return true;
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

  LOG(ERROR)
      << "HandleWebContentsStartRequest! is render widget host view present: "
      << (pending_request->tab_model &&
          pending_request->tab_model->GetContents()
                  ->GetRenderWidgetHostView() != nullptr);

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
    if (pending_request->tab_model) {
      content_extraction::GetInnerHtml(
          *pending_request->tab_model->GetContents()->GetPrimaryMainFrame(),
          base::BindOnce(
              &BackupResultsServiceImpl::HandleWebContentsContentExtraction,
              weak_ptr_factory_.GetWeakPtr(), pending_request));
    }
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
      kMaxResponseSize.InBytes());
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

void BackupResultsServiceImpl::NavigateToOriginalUrl(
    PendingRequestList::iterator pending_request) {
  if (pending_request == pending_requests_.end() ||
      !pending_request->tab_model) {
    return;
  }

  auto* web_contents = pending_request->tab_model->GetContents();
  LOG(ERROR) << "NavigateToOriginalUrl! is render widget host view present: "
             << (web_contents->GetRenderWidgetHostView() != nullptr);

  LOG(ERROR) << "NavigateToOriginalUrl! "
             << pending_request->original_url.spec();
  auto size = web_contents->GetSize();
  LOG(ERROR) << "NavigateToOriginalUrl! Size: " << size.width() << "x"
             << size.height();
  auto bounds = web_contents->GetRenderWidgetHostView()->GetViewBounds();
  LOG(ERROR) << "NavigateToOriginalUrl! Bounds: " << bounds.x() << ","
             << bounds.y() << "," << bounds.width() << "," << bounds.height();

  auto load_url_params = content::NavigationController::LoadURLParams(
      pending_request->original_url);
  if (!web_contents->GetController().LoadURLWithParams(load_url_params)) {
    CleanupAndDispatchResult(pending_request, std::nullopt);
  }
}

void BackupResultsServiceImpl::CleanupAndDispatchResult(
    PendingRequestList::iterator pending_request,
    std::optional<BackupResults> result) {
  auto* otr_profile = pending_request->otr_profile.get();

  // We own the web contents, so it will be automatically destroyed
  // when the pending request is erased

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
