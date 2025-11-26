/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/brave_ephemeral_storage_service_delegate.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "brave/browser/brave_shields/brave_shields_settings_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browsing_data_filter_builder.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/features.h"
#include "net/base/schemeful_site.h"
#include "net/base/url_util.h"
#include "url/origin.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#else
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#endif

namespace {

void CollectTabsToClose(
    tabs::TabInterface* tab,
    const std::string& etldplusone,
    const int current_tab_index,
    std::vector<std::pair<int, tabs::TabInterface*>>& tabs_to_close) {
  if (!tab) {
    return;
  }
  content::WebContents* contents = tab->GetContents();
  if (!contents) {
    return;
  }

  const auto tab_tld =
      net::URLToEphemeralStorageDomain(contents->GetLastCommittedURL());
  if (tab_tld.empty() || tab_tld != etldplusone) {
    return;
  }

  if (auto* ephemeral_storage_tab_helper =
          ephemeral_storage::EphemeralStorageTabHelper::FromWebContents(
              contents)) {
    tabs_to_close.emplace_back(current_tab_index, tab);
    ephemeral_storage_tab_helper->EnforceFirstPartyStorageCleanup();
  }
}

}  // namespace

namespace ephemeral_storage {

BraveEphemeralStorageServiceDelegate::BraveEphemeralStorageServiceDelegate(
    content::BrowserContext* context,
    HostContentSettingsMap* host_content_settings_map,
    scoped_refptr<content_settings::CookieSettings> cookie_settings,
    brave_shields::BraveShieldsSettingsService* shields_settings_service)
    : context_(context),
      host_content_settings_map_(host_content_settings_map),
      cookie_settings_(std::move(cookie_settings)),
      shields_settings_service_(shields_settings_service) {
  DCHECK(context_);
  DCHECK(host_content_settings_map_);
  DCHECK(cookie_settings_);
  CHECK(shields_settings_service_);
}

BraveEphemeralStorageServiceDelegate::~BraveEphemeralStorageServiceDelegate() {
#if !BUILDFLAG(IS_ANDROID)
  BrowserList::RemoveObserver(this);
#endif
}

void BraveEphemeralStorageServiceDelegate::CleanupTLDEphemeralArea(
    const TLDEphemeralAreaKey& key) {
  DVLOG(1) << __func__ << " " << key.first << " " << key.second;
  auto* storage_partition = context_->GetStoragePartition(key.second);
  if (!storage_partition) {
    return;
  }
  auto filter = network::mojom::CookieDeletionFilter::New();
  filter->ephemeral_storage_domain = key.first;
  storage_partition->GetCookieManagerForBrowserProcess()->DeleteCookies(
      std::move(filter), base::NullCallback());

  const GURL https_url(base::StrCat({"https://", key.first}));
  if (brave_shields::GetCookieControlType(host_content_settings_map_,
                                          cookie_settings_.get(), https_url) ==
      brave_shields::ControlType::ALLOW) {
    // All cookies are allowed, Ephemeral Storage is effectively disabled.
    return;
  }

  const GURL http_url(base::StrCat({"http://", key.first}));

  // Only cleanup StorageKey-aware areas.
  content::BrowsingDataRemover::DataType data_to_remove =
      content::BrowsingDataRemover::DATA_TYPE_DOM_STORAGE;

  content::BrowsingDataRemover::OriginType origin_type =
      content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
      content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB;

  // Cookies are partitioned and cleaned separately.
  data_to_remove &= ~content::BrowsingDataRemover::DATA_TYPE_COOKIES;

  auto filter_builder = content::BrowsingDataFilterBuilder::Create(
      content::BrowsingDataFilterBuilder::Mode::kDelete,
      content::BrowsingDataFilterBuilder::OriginMatchingMode::
          kThirdPartiesOnly);
  filter_builder->AddOrigin(url::Origin::Create(https_url));
  filter_builder->AddOrigin(url::Origin::Create(http_url));
  filter_builder->SetStoragePartitionConfig(key.second);

  content::BrowsingDataRemover* remover = context_->GetBrowsingDataRemover();
  remover->RemoveWithFilter(base::Time(), base::Time::Max(), data_to_remove,
                            origin_type, std::move(filter_builder));
}

void BraveEphemeralStorageServiceDelegate::CleanupFirstPartyStorageArea(
    const TLDEphemeralAreaKey& key) {
  DVLOG(1) << __func__ << " " << key.first << " " << key.second;
  DCHECK(base::FeatureList::IsEnabled(
             net::features::kBraveForgetFirstPartyStorage) ||
         base::FeatureList::IsEnabled(
             net::features::kThirdPartyStoragePartitioning));

  content::BrowsingDataRemover::DataType data_to_remove =
      (content::BrowsingDataRemover::DATA_TYPE_ON_STORAGE_PARTITION &
       chrome_browsing_data_remover::FILTERABLE_DATA_TYPES);

  content::BrowsingDataRemover::OriginType origin_type =
      content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
      content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB;

  auto filter_builder = content::BrowsingDataFilterBuilder::Create(
      content::BrowsingDataFilterBuilder::Mode::kDelete);
  filter_builder->AddRegisterableDomain(key.first);
  filter_builder->SetStoragePartitionConfig(key.second);

  content::BrowsingDataRemover* remover = context_->GetBrowsingDataRemover();
  remover->RemoveWithFilter(base::Time(), base::Time::Max(), data_to_remove,
                            origin_type, std::move(filter_builder));
}

void BraveEphemeralStorageServiceDelegate::RegisterFirstWindowOpenedCallback(
    base::OnceClosure callback) {
  DCHECK(callback);
#if !BUILDFLAG(IS_ANDROID)
  BrowserList::AddObserver(this);
  first_window_opened_callback_ = std::move(callback);
#else
  std::move(callback).Run();
#endif  // !BUILDFLAG(IS_ANDROID)
}

#if !BUILDFLAG(IS_ANDROID)
void BraveEphemeralStorageServiceDelegate::OnBrowserAdded(Browser* browser) {
  if (browser->profile() != Profile::FromBrowserContext(context_)) {
    return;
  }

  if (first_window_opened_callback_) {
    std::move(first_window_opened_callback_).Run();
  }

  // No need to observe anymore.
  BrowserList::RemoveObserver(this);
}
#endif

void BraveEphemeralStorageServiceDelegate::PrepareTabsForStorageCleanup(
    const std::string& ephemeral_domain) {
  auto* profile = Profile::FromBrowserContext(context_);
  CHECK(profile);

#if !BUILDFLAG(IS_ANDROID)

  for (Browser* browser : *BrowserList::GetInstance()) {
    if (profile != browser->profile()) {
      continue;
    }
    BraveTabStripModel* tab_strip =
        static_cast<BraveTabStripModel*>(browser->tab_strip_model());
    if (!tab_strip) {
      continue;
    }

    std::vector<std::pair<int, tabs::TabInterface*>> tabs_to_close;
    for (auto tab_itr = tab_strip->begin(); tab_itr != tab_strip->end();
         ++tab_itr) {
      tabs::TabInterface* tab = *tab_itr;
      CollectTabsToClose(tab, ephemeral_domain,
                         tab_strip->GetIndexOfWebContents(tab->GetContents()),
                         tabs_to_close);
    }

    for (auto& [index, tab] : tabs_to_close) {
      tab->GetContents()->GetPrimaryMainFrame()->GetProcess()->Shutdown(0);
      tab_strip->CloseWebContentsAt(index, 0);
    }
  }
#else
  std::vector<std::pair<int, tabs::TabInterface*>> tabs_to_close;
  for (TabModel* model : TabModelList::models()) {
    const size_t tab_count = model->GetTabCount();
    for (size_t index = 0; index < tab_count; index++) {
      auto* tab = model->GetTabAt(index);
      // Do not process tabs from other profiles.
      if (!tab || profile != tab->profile()) {
        continue;
      }

      CollectTabsToClose(tab, ephemeral_domain, index, tabs_to_close);
    }
  }
  // Close all collected tabs
  for (auto& [index, tab] : tabs_to_close) {
    if (!tab) {
      continue;
    }
    tab->Close();
  }
#endif
}

bool BraveEphemeralStorageServiceDelegate::
    IsShieldsDisabledOnAnyHostMatchingDomainOf(const GURL& url) const {
  return shields_settings_service_->IsShieldsDisabledOnAnyHostMatchingDomainOf(
      url);
}

}  // namespace ephemeral_storage
