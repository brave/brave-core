// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_search/backup_results_service_impl.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/byte_size.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/components/brave_search/browser/backup_results_allowed_urls.h"
#include "brave/components/brave_search/browser/backup_results_service.h"
#include "brave/components/brave_search/browser/prefs.h"
#include "brave/components/brave_search/common/features.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_extraction/inner_html.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/content_settings/page_specific_content_settings_delegate.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/restore_type.h"
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
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "ui/gfx/geometry/size.h"

#if BUILDFLAG(IS_ANDROID)
#include "ui/android/view_android.h"
#else
#include "ui/gfx/geometry/rect.h"
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

constexpr base::ByteSize kMaxResponseSize = base::MiBU(5);
constexpr base::TimeDelta kTimeout = base::Seconds(5);
constexpr base::TimeDelta kLoadAfterRestoreTimeout = base::Seconds(12);

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
      local_state_(g_browser_process->local_state()),
      backup_results_metrics_(local_state_) {
  profile_->AddObserver(this);
}
BackupResultsServiceImpl::~BackupResultsServiceImpl() = default;

// static
void BackupResultsServiceImpl::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kBackupResultsLastViewWidth, 0);
  registry->RegisterIntegerPref(prefs::kBackupResultsLastViewHeight, 0);
  registry->RegisterIntegerPref(prefs::kBackupResultsDailyRequestCount, 0);
  registry->RegisterTimePref(prefs::kBackupResultsDailyRequestWindowStart, {});
  BackupResultsMetrics::RegisterPrefs(registry);
}

// static
void BackupResultsServiceImpl::RecordLastViewSize(PrefService* local_state,
                                                  const gfx::Size& size) {
  if (size.IsEmpty()) {
    return;
  }
  local_state->SetInteger(prefs::kBackupResultsLastViewWidth, size.width());
  local_state->SetInteger(prefs::kBackupResultsLastViewHeight, size.height());
}

void BackupResultsServiceImpl::FetchBackupResults(
    const GURL& url,
    std::optional<net::HttpRequestHeaders> headers,
    BackupResultsCallback callback,
    bool low_latency_required) {
  if (!profile_ || !base::FeatureList::IsEnabled(features::kBackupResults) ||
      UpdateDailyRequestCount()) {
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
    content_settings::PageSpecificContentSettings::CreateForWebContents(
        web_contents.get(),
        std::make_unique<PageSpecificContentSettingsDelegate>(
            web_contents.get()));
    brave_shields::BraveShieldsWebContentsObserver::CreateForWebContents(
        web_contents.get());

    int stored_width =
        local_state_->GetInteger(prefs::kBackupResultsLastViewWidth);
    int stored_height =
        local_state_->GetInteger(prefs::kBackupResultsLastViewHeight);
    gfx::Size view_size(
        stored_width > 0 ? stored_width : base::RandIntInclusive(800, 1920),
        stored_height > 0 ? stored_height : base::RandIntInclusive(600, 1080));
#if BUILDFLAG(IS_ANDROID)
    auto* native_view = web_contents->GetNativeView();
    float dip_scale = native_view->GetDipScale();
    native_view->OnSizeChanged(
        static_cast<int>(view_size.width() * dip_scale),
        static_cast<int>(view_size.height() * dip_scale));
#else
    web_contents->Resize(gfx::Rect(view_size));
#endif

    auto web_preferences = web_contents->GetOrCreateWebPreferences();
    web_preferences.supports_multiple_windows = false;
    web_contents->SetWebPreferences(web_preferences);

    SeedNavigationHistory(*web_contents, url);

    BackupResultsWebContentsObserver::CreateForWebContents(
        web_contents.get(), weak_ptr_factory_.GetWeakPtr());
  }

  auto request = pending_requests_.emplace(
      pending_requests_.end(), std::move(web_contents), headers, otr_profile,
      low_latency_required, std::move(callback));

  if (should_render) {
    const bool load_after_restore =
        features::kBackupResultsLoadAfterRestore.Get();
    request->target_url = url;
    if (!load_after_restore) {
      if (!LoadTargetUrl(request)) {
        return;
      }
    }
    const base::TimeDelta timeout =
        load_after_restore ? kLoadAfterRestoreTimeout : kTimeout;
    request->timeout_timer.Start(
        FROM_HERE, timeout,
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
    bool low_latency_required,
    BackupResultsCallback callback)
    : headers(std::move(headers)),
      callback(std::move(callback)),
      low_latency_required(low_latency_required),
      web_contents(std::move(web_contents)),
      otr_profile(otr_profile) {}

BackupResultsServiceImpl::PendingRequest::~PendingRequest() = default;

BackupResultsServiceImpl::PendingRequestList::iterator
BackupResultsServiceImpl::FindPendingRequest(
    const content::WebContents* web_contents) {
  return std::ranges::find_if(pending_requests_, [&](const auto& request) {
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
    if (features::kBackupResultsLoadAfterRestore.Get() &&
        url != pending_request->target_url) {
      return true;
    }
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

  if (features::IsBackupResultsFullRenderEnabled() &&
      pending_request->requests_loaded ==
          static_cast<size_t>(
              features::kBackupResultsFullRenderMaxRequests.Get())) {
    content_extraction::GetInnerHtml(
        *pending_request->web_contents->GetPrimaryMainFrame(),
        base::BindOnce(
            &BackupResultsServiceImpl::HandleWebContentsContentExtraction,
            weak_ptr_factory_.GetWeakPtr(), pending_request));
    return;
  }

  if (features::kBackupResultsLoadAfterRestore.Get() &&
      pending_request->web_contents->GetLastCommittedURL() !=
          pending_request->target_url) {
    // Root page has loaded; schedule load of the actual target URL after a
    // randomized delay.
    const base::TimeDelta delay_min =
        pending_request->low_latency_required
            ? features::kBackupResultsLoadAfterRestoreLowDelayMin.Get()
            : features::kBackupResultsLoadAfterRestoreDelayMin.Get();
    const base::TimeDelta delay_max =
        pending_request->low_latency_required
            ? features::kBackupResultsLoadAfterRestoreLowDelayMax.Get()
            : features::kBackupResultsLoadAfterRestoreDelayMax.Get();
    const base::TimeDelta delay = base::RandTimeDelta(delay_min, delay_max);
    pending_request->load_after_restore_timer.Start(
        FROM_HERE, delay,
        base::BindOnce(&BackupResultsServiceImpl::OnLoadAfterRestoreTimer,
                       base::Unretained(this), pending_request));
  }
}

base::WeakPtr<BackupResultsService> BackupResultsServiceImpl::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

net::HttpRequestHeaders BackupResultsServiceImpl::GetExtraHeaders(
    const std::optional<net::HttpRequestHeaders>& request_headers) {
  if (!feature_headers_) {
    feature_headers_.emplace();
    const std::string& headers_json = features::kBackupResultsHeaders.Get();
    if (!headers_json.empty()) {
      auto parsed = base::JSONReader::Read(headers_json, base::JSON_PARSE_RFC);
      if (parsed && parsed->is_dict()) {
        for (const auto [name, value] : parsed->GetDict()) {
          if (value.is_string()) {
            feature_headers_->SetHeader(name, value.GetString());
          }
        }
      }
    }
  }
  net::HttpRequestHeaders extra_headers = *feature_headers_;
  if (request_headers) {
    extra_headers.MergeFrom(*request_headers);
  }
  return extra_headers;
}

void BackupResultsServiceImpl::MaybeApplyUserAgentOverride(
    content::WebContents& web_contents,
    content::NavigationController::LoadURLParams& load_url_params) {
  if (!ua_override_) {
    const std::string& ua_string = features::kBackupResultsUAOverride.Get();
    if (ua_string.empty()) {
      return;
    }
    ua_override_.emplace();
    ua_override_->ua_string_override = ua_string;
    const std::string& encoded_ua = features::kBackupResultsUAMetadata.Get();
    if (!encoded_ua.empty()) {
      std::string decoded;
      if (base::Base64Decode(encoded_ua, &decoded)) {
        ua_override_->ua_metadata_override =
            blink::UserAgentMetadata::Demarshal(decoded);
      }
    }
  }
  web_contents.SetUserAgentOverride(*ua_override_,
                                    /*override_in_new_tabs=*/true);
  load_url_params.override_user_agent =
      content::NavigationController::UA_OVERRIDE_TRUE;
}

void BackupResultsServiceImpl::SeedNavigationHistory(
    content::WebContents& web_contents,
    const GURL& target_url) {
  if (!target_url.SchemeIsHTTPOrHTTPS()) {
    return;
  }
  GURL::Replacements replacements;
  replacements.SetPathStr("/");
  replacements.ClearQuery();
  replacements.ClearRef();
  GURL origin_root = target_url.ReplaceComponents(replacements);
  if (!origin_root.is_valid() || origin_root == target_url) {
    return;
  }

  std::unique_ptr<content::NavigationEntry> entry =
      content::NavigationController::CreateNavigationEntry(
          origin_root, content::Referrer(),
          /*initiator_origin=*/std::nullopt,
          /*initiator_base_url=*/std::nullopt, ui::PAGE_TRANSITION_TYPED,
          /*is_renderer_initiated=*/false,
          /*extra_headers=*/std::string(), web_contents.GetBrowserContext(),
          /*blob_url_loader_factory=*/nullptr);

  std::vector<std::unique_ptr<content::NavigationEntry>> entries;
  entries.push_back(std::move(entry));
  web_contents.GetController().Restore(
      /*selected_navigation=*/0, content::RestoreType::kRestored, &entries);

  if (features::kBackupResultsLoadAfterRestore.Get()) {
    web_contents.GetController().LoadIfNecessary();
  }
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

  resource_request->headers = GetExtraHeaders(pending_request->headers);

  if (ua_override_) {
    resource_request->headers.SetHeader(net::HttpRequestHeaders::kUserAgent,
                                        ua_override_->ua_string_override);
  }

  pending_request->simple_url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), kNetworkTrafficAnnotationTag);
  pending_request->simple_url_loader->DownloadToString(
      pending_request->shared_url_loader_factory.get(),
      base::BindOnce(&BackupResultsServiceImpl::HandleURLLoaderResponse,
                     base::Unretained(this), pending_request),
      kMaxResponseSize.InBytes());
}

void BackupResultsServiceImpl::OnLoadAfterRestoreTimer(
    PendingRequestList::iterator pending_request) {
  LoadTargetUrl(pending_request);
}

bool BackupResultsServiceImpl::LoadTargetUrl(
    PendingRequestList::iterator pending_request) {
  if (!pending_request->web_contents) {
    return false;
  }
  auto load_url_params =
      content::NavigationController::LoadURLParams(pending_request->target_url);
  load_url_params.transition_type = ui::PAGE_TRANSITION_LINK;
  for (size_t i = 0;
       i <= static_cast<size_t>(blink::NavigationDownloadType::kMaxValue);
       i++) {
    blink::NavigationDownloadType type =
        static_cast<blink::NavigationDownloadType>(i);
    load_url_params.download_policy.SetDisallowed(type);
  }
  auto extra_headers = GetExtraHeaders(pending_request->headers);
  if (!extra_headers.IsEmpty()) {
    load_url_params.extra_headers = extra_headers.ToString();
  }
  MaybeApplyUserAgentOverride(*pending_request->web_contents, load_url_params);
  if (!pending_request->web_contents->GetController().LoadURLWithParams(
          load_url_params)) {
    CleanupAndDispatchResult(pending_request, std::nullopt);
    return false;
  }
  return true;
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

  // Track query result (failure if result is nullopt, success otherwise)
  backup_results_metrics_.RecordQuery(!result);

  std::move(pending_request->callback).Run(result);
  pending_requests_.erase(pending_request);

  if (profile_) {
    profile_->DestroyOffTheRecordProfile(otr_profile);
  }
}

bool BackupResultsServiceImpl::UpdateDailyRequestCount() {
  const int limit = features::kBackupResultsMaxDailyRequests.Get();
  if (limit == -1) {
    return false;
  }

  const auto now = base::Time::Now();
  const auto window_start =
      local_state_->GetTime(prefs::kBackupResultsDailyRequestWindowStart);

  if (window_start.is_null() || (now - window_start) >= base::Days(1)) {
    local_state_->SetTime(prefs::kBackupResultsDailyRequestWindowStart, now);
    local_state_->SetInteger(prefs::kBackupResultsDailyRequestCount, 1);
    return false;
  }

  const int count =
      local_state_->GetInteger(prefs::kBackupResultsDailyRequestCount);
  if (count >= limit) {
    return true;
  }

  local_state_->SetInteger(prefs::kBackupResultsDailyRequestCount, count + 1);
  return false;
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
