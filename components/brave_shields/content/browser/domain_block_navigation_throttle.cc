// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/domain_block_navigation_throttle.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/content/browser/domain_block_controller_client.h"
#include "brave/components/brave_shields/content/browser/domain_block_page.h"
#include "brave/components/brave_shields/content/browser/domain_block_tab_storage.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "brave/content/public/browser/devtools/adblock_devtools_instumentation.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "net/base/net_errors.h"
#include "url/gurl.h"

namespace brave_shields {

struct DomainBlockNavigationThrottle::BlockResult {
  BlockResult() = default;
  BlockResult(const BlockResult&) = default;
  BlockResult(BlockResult&&) = default;
  ~BlockResult() = default;

  bool should_block = false;
  std::string new_url;
  std::optional<content::devtools_instrumentation::AdblockInfo> info;
};

}  // namespace brave_shields

namespace {
brave_shields::DomainBlockNavigationThrottle::BlockResult
ShouldBlockDomainOnTaskRunner(brave_shields::AdBlockService* ad_block_service,
                              const GURL& url,
                              bool aggressive_setting) {
  SCOPED_UMA_HISTOGRAM_TIMER("Brave.DomainBlock.ShouldBlock");
  brave_shields::DomainBlockNavigationThrottle::BlockResult block_result;
  // force aggressive blocking to `true` for domain blocking - these requests
  // are all "first-party", but the throttle is already only called when
  // necessary.
  bool aggressive_for_engine = true;
  auto result = ad_block_service->ShouldStartRequest(
      url, blink::mojom::ResourceType::kMainFrame, std::string(url.host()),
      aggressive_for_engine, false, false, false);

  block_result.should_block =
      result.important || (result.matched && !result.has_exception);

  block_result.new_url = aggressive_setting && result.rewritten_url.has_value
                             ? std::string(result.rewritten_url.value)
                             : std::string();

  if (block_result.should_block || result.has_exception) {
    content::devtools_instrumentation::AdblockInfo info;
    info.request_url = url;
    info.checked_url = url;
    info.source_host = url.host();
    info.resource_type = blink::mojom::ResourceType::kMainFrame;
    info.aggressive = aggressive_for_engine;
    info.blocked = block_result.should_block;
    info.did_match_important_rule = result.important;
    info.did_match_rule = result.matched;
    info.did_match_exception = result.has_exception;
    info.has_mock_data = false;
    if (!block_result.new_url.empty()) {
      info.rewritten_url = block_result.new_url;
    }
    block_result.info = std::move(info);
  }

  return block_result;
}

}  // namespace

namespace brave_shields {

// static
void DomainBlockNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry,
    AdBlockService* ad_block_service,
    AdBlockCustomFiltersProvider* ad_block_custom_filters_provider,
    ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
    HostContentSettingsMap* content_settings,
    const std::string& locale) {
  if (!ad_block_service || !ad_block_custom_filters_provider) {
    return;
  }
  if (!base::FeatureList::IsEnabled(
          brave_shields::features::kBraveDomainBlock)) {
    return;
  }
  // Don't block subframes.
  if (!registry.GetNavigationHandle().IsInPrimaryMainFrame()) {
    return;
  }
  registry.AddThrottle(std::make_unique<DomainBlockNavigationThrottle>(
      registry, ad_block_service, ad_block_custom_filters_provider,
      ephemeral_storage_service, content_settings, locale));
}

DomainBlockNavigationThrottle::DomainBlockNavigationThrottle(
    content::NavigationThrottleRegistry& registry,
    AdBlockService* ad_block_service,
    AdBlockCustomFiltersProvider* ad_block_custom_filters_provider,
    ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
    HostContentSettingsMap* content_settings,
    const std::string& locale)
    : content::NavigationThrottle(registry),
      ad_block_service_(ad_block_service),
      ad_block_custom_filters_provider_(ad_block_custom_filters_provider),
      ephemeral_storage_service_(ephemeral_storage_service),
      content_settings_(content_settings),
      locale_(locale) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

DomainBlockNavigationThrottle::~DomainBlockNavigationThrottle() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

content::NavigationThrottle::ThrottleCheckResult
DomainBlockNavigationThrottle::WillStartRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::NavigationHandle* handle = navigation_handle();
  DCHECK(handle->IsInMainFrame());
  GURL request_url = handle->GetURL();

  DomainBlockingType domain_blocking_type =
      brave_shields::GetDomainBlockingType(content_settings_, request_url);
  content::WebContents* web_contents = handle->GetWebContents();
  // Maybe don't block based on Brave Shields settings
  if (domain_blocking_type == DomainBlockingType::kNone) {
    DomainBlockTabStorage* tab_storage =
        DomainBlockTabStorage::FromWebContents(web_contents);
    if (tab_storage) {
      tab_storage->DropBlockedDomain1PESLifetime();
    }
    return content::NavigationThrottle::PROCEED;
  }

  // If user has just chosen to proceed on our interstitial, don't show
  // another one.
  DomainBlockTabStorage* tab_storage =
      DomainBlockTabStorage::GetOrCreate(web_contents);
  if (tab_storage->IsProceeding()) {
    return content::NavigationThrottle::PROCEED;
  }

  bool aggressive_mode =
      brave_shields::GetCosmeticFilteringControlType(
          content_settings_, request_url) == brave_shields::ControlType::BLOCK;

  // Otherwise, call the ad block service on a task runner to determine whether
  // this domain should be blocked.
  ad_block_service_->GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ShouldBlockDomainOnTaskRunner, ad_block_service_,
                     request_url, aggressive_mode),
      base::BindOnce(&DomainBlockNavigationThrottle::OnShouldBlockDomain,
                     weak_ptr_factory_.GetWeakPtr(), domain_blocking_type));

  // Since the call to the ad block service is asynchronous, we defer the final
  // decision of whether to allow or block this navigation. The callback from
  // the task runner will call a method to give our final answer.
  return content::NavigationThrottle::DEFER;
}

content::NavigationThrottle::ThrottleCheckResult
DomainBlockNavigationThrottle::WillRedirectRequest() {
  return WillStartRequest();
}

content::NavigationThrottle::ThrottleCheckResult
DomainBlockNavigationThrottle::WillProcessResponse() {
  // If there is an DomainBlockTabStorage associated to |web_contents_|, clear
  // the IsProceeding flag.
  DomainBlockTabStorage* tab_storage = DomainBlockTabStorage::FromWebContents(
      navigation_handle()->GetWebContents());
  if (tab_storage) {
    tab_storage->SetIsProceeding(false);
  }
  return content::NavigationThrottle::PROCEED;
}

void DomainBlockNavigationThrottle::OnShouldBlockDomain(
    DomainBlockingType domain_blocking_type,
    const BlockResult& block_result) {
  const bool should_block = block_result.should_block;
  const GURL new_url(block_result.new_url);

  if (should_block && block_result.info) {
    content::devtools_instrumentation::SendAdblockInfo(
        this->navigation_handle(), block_result.info.value());
  }

  if (!should_block && !new_url.is_valid()) {
    DomainBlockTabStorage* tab_storage = DomainBlockTabStorage::FromWebContents(
        navigation_handle()->GetWebContents());
    if (tab_storage) {
      tab_storage->DropBlockedDomain1PESLifetime();
    }
    // Navigation was deferred while we called the ad block service on a task
    // runner, but now we know that we want to allow navigation to continue.
    Resume();
    return;
  } else if (new_url.is_valid()) {
    RestartNavigation(new_url);
  } else {
    switch (domain_blocking_type) {
      case DomainBlockingType::k1PES:
        Enable1PESAndResume();
        break;
      case DomainBlockingType::kAggressive:
        ShowInterstitial();
        break;
      case DomainBlockingType::kNone:
        NOTREACHED();
    }
  }
}

void DomainBlockNavigationThrottle::ShowInterstitial() {
  content::NavigationHandle* handle = navigation_handle();
  content::WebContents* web_contents = handle->GetWebContents();
  const GURL& request_url = handle->GetURL();
  content::BrowserContext* context =
      handle->GetWebContents()->GetBrowserContext();
  PrefService* pref_service = user_prefs::UserPrefs::Get(context);

  // The controller client implements the actual logic to "go back" or "proceed"
  // from the interstitial.
  auto controller_client = std::make_unique<DomainBlockControllerClient>(
      web_contents, request_url, ad_block_custom_filters_provider_,
      ephemeral_storage_service_, pref_service, locale_);

  // This handles populating the HTML template of the interstitial page with
  // localized strings and other information we only know at runtime,
  // including the URL of the page we're blocking. Once the user interacts with
  // the interstitial, this translates those actions into method calls on the
  // controller client.
  auto blocked_page = std::make_unique<DomainBlockPage>(
      web_contents, request_url, std::move(controller_client));

  // Get the page content before giving up ownership of |blocked_page|.
  std::string blocked_page_content = blocked_page->GetHTMLContents();

  // Replace the tab contents with our interstitial page.
  security_interstitials::SecurityInterstitialTabHelper::AssociateBlockingPage(
      handle, std::move(blocked_page));

  // Navigation was deferred rather than canceled outright because the
  // call to the ad blocking service happens on a task runner, but now we
  // know that we definitely want to cancel the navigation.
  CancelDeferredNavigation(content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL, net::ERR_BLOCKED_BY_CLIENT,
      blocked_page_content));
}

void DomainBlockNavigationThrottle::Enable1PESAndResume() {
  DCHECK(ephemeral_storage_service_);
  DomainBlockTabStorage* tab_storage = DomainBlockTabStorage::FromWebContents(
      navigation_handle()->GetWebContents());
  if (ephemeral_storage_service_->Is1PESEnabledForUrl(
          navigation_handle()->GetURL())) {
    Resume();
    return;
  } else if (tab_storage) {
    tab_storage->Enable1PESForUrlIfPossible(
        ephemeral_storage_service_, navigation_handle()->GetURL(),
        base::BindOnce(&DomainBlockNavigationThrottle::On1PESState,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void DomainBlockNavigationThrottle::On1PESState(bool is_1pes_enabled) {
  if (is_1pes_enabled) {
    RestartNavigation(navigation_handle()->GetURL());
  } else {
    Resume();
    return;
  }
}

void DomainBlockNavigationThrottle::RestartNavigation(const GURL& url) {
  content::NavigationHandle* handle = navigation_handle();

  content::OpenURLParams params =
      content::OpenURLParams::FromNavigationHandle(handle);

  content::WebContents* contents = handle->GetWebContents();

  params.url = url;
  params.transition = static_cast<ui::PageTransition>(
      params.transition | ui::PAGE_TRANSITION_CLIENT_REDIRECT);
  // We get a DCHECK here if we don't clear the redirect chain because
  // technically this is a new navigation
  params.redirect_chain.clear();

  // Cancel without an error status to surface any real errors during page
  // load.
  CancelDeferredNavigation(content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL));

  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
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
}

const char* DomainBlockNavigationThrottle::GetNameForLogging() {
  return "DomainBlockNavigationThrottle";
}

}  // namespace brave_shields
