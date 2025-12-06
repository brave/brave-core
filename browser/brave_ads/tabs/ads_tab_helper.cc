/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tabs/ads_tab_helper.h"

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/fixed_flat_set.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_results_page_util.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/default_search_manager.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/search_engine_utils.h"
#include "components/search_engines/template_url_service.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/sessions/core/session_id.h"
#include "components/variations/pref_names.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#endif

namespace brave_ads {

// TODO(tmancey): Persist this value across sessions. Currently, it is stored as
// a global variable for proof-of-concept simplicity.
static std::optional<base::Time> last_reported_search_query_metric_at;

// TODO(tmancey): Add Griffin feature flag.

namespace {

constexpr int kHttpClientErrorResponseStatusCodeClass = 4;
constexpr int kHttpServerErrorResponseStatusCodeClass = 5;

constexpr char16_t kSerializeDocumentToStringJavaScript[] =
    u"new XMLSerializer().serializeToString(document)";

constexpr char16_t kDocumentBodyInnerTextJavaScript[] =
    u"document?.body?.innerText";

// Returns 'false' if the navigation was a back/forward navigation or a reload,
// otherwise 'true'.
bool IsNewNavigation(content::NavigationHandle* const navigation_handle) {
  CHECK(navigation_handle);

  return ui::PageTransitionIsNewNavigation(
      navigation_handle->GetPageTransition());
}

// NOTE: DO NOT use this method before the navigation commit as it will return
// null. It is safe to use from `WebContentsObserver::DidFinishNavigation()`.
std::optional<int> HttpStatusCode(
    content::NavigationHandle* const navigation_handle) {
  CHECK(navigation_handle);

  if (const net::HttpResponseHeaders* const response_headers =
          navigation_handle->GetResponseHeaders()) {
    return response_headers->response_code();
  }

  return std::nullopt;
}

bool IsErrorPage(int http_status_code) {
  const int http_status_code_class = http_status_code / 100;
  return http_status_code_class == kHttpClientErrorResponseStatusCodeClass ||
         http_status_code_class == kHttpServerErrorResponseStatusCodeClass;
}

std::string MediaPlayerUuid(const content::MediaPlayerId& id) {
  return absl::StrFormat("%d%d%d", id.frame_routing_id.child_id,
                         id.frame_routing_id.frame_routing_id, id.player_id);
}

network::mojom::NetworkContext* GetNetworkContextForProfile(
    content::BrowserContext* context) {
  return context->GetDefaultStoragePartition()->GetNetworkContext();
}

constexpr auto kSupportedDefaultSearchEngines =
    base::MakeFixedFlatSet<std::u16string>({u"Brave", u"Google", u"DuckDuckGo",
                                            u"Qwant", u"Bing", u"Startpage",
                                            u"Ecosia"});

// TODO(tmancey): Remove KY, this was added temporarily for testing purposes
// during development of the proof-of-concept.
constexpr auto kSupportedCountries = base::MakeFixedFlatSet<std::string>(
    {"BR", "CA", "CO", "DE", "ES", "FR", "GB", "IN", "IT", "JP", "KY", "MX",
     "NL", "PH", "PL", "US"});

}  // namespace

AdsTabHelper::AdsTabHelper(content::WebContents* const web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AdsTabHelper>(*web_contents),
      session_id_(sessions::SessionTabHelper::IdForTab(web_contents)) {
  if (!session_id_.is_valid()) {
    return;
  }

  Profile* const profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  ads_service_ = AdsServiceFactory::GetForProfile(profile);
  if (!ads_service_) {
    return;
  }

  // TODO(tmancey): This should not be part of `AdsTabHelper` creation because
  // these objects are destroyed when the user closes the tab. Included here
  // only for proof-of-concept simplicity.
  network_client_ = std::make_unique<NetworkClient>(
      profile->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess(),
      base::BindRepeating(&GetNetworkContextForProfile,
                          web_contents->GetBrowserContext()));

#if !BUILDFLAG(IS_ANDROID)
  // See "background_helper_android.h" for Android.
  BrowserList::AddObserver(this);
#endif  // !BUILDFLAG(IS_ANDROID)

  MaybeSetBrowserIsActive();

  OnVisibilityChanged(web_contents->GetVisibility());
}

AdsTabHelper::~AdsTabHelper() {
#if !BUILDFLAG(IS_ANDROID)
  BrowserList::RemoveObserver(this);
#endif
}

void AdsTabHelper::SetAdsServiceForTesting(AdsService* const ads_service) {
  CHECK_IS_TEST();

  ads_service_ = ads_service;
}

bool AdsTabHelper::UserHasJoinedBraveRewards() const {
  const PrefService* const prefs =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext())
          ->GetPrefs();

  return prefs->GetBoolean(brave_rewards::prefs::kEnabled);
}

bool AdsTabHelper::UserHasOptedInToNotificationAds() const {
  const PrefService* const prefs =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext())
          ->GetPrefs();

  return prefs->GetBoolean(brave_rewards::prefs::kEnabled) &&
         prefs->GetBoolean(prefs::kOptedInToNotificationAds);
}

bool AdsTabHelper::IsVisible() const {
  // The web contents must be visible and the browser must be active.
  return is_web_contents_visible_ && is_browser_active_.value_or(false);
}

void AdsTabHelper::MaybeSetBrowserIsActive() {
  if (is_browser_active_.has_value() && *is_browser_active_) {
    // Already active.
    return;
  }

  is_browser_active_ = true;

  MaybeNotifyBrowserDidBecomeActive();

  // Maybe notify of tab change after the browser's active state changes because
  // `OnVisibilityChanged` can be called before `OnBrowserSetLastActive`.
  MaybeNotifyTabDidChange();
}

void AdsTabHelper::MaybeSetBrowserIsNoLongerActive() {
  if (is_browser_active_.has_value() && !*is_browser_active_) {
    // Already inactive.
    return;
  }

  is_browser_active_ = false;

  MaybeNotifyBrowserDidResignActive();

  // Maybe notify of tab change after the browser's active state changes because
  // `OnVisibilityChanged` can be called before `OnBrowserNoLongerActive`.
  MaybeNotifyTabDidChange();
}

void AdsTabHelper::ProcessNavigation() {
  MaybeNotifyTabHtmlContentDidChange();
  MaybeNotifyTabTextContentDidChange();
}

void AdsTabHelper::ProcessSameDocumentNavigation() {
  MaybeNotifyTabHtmlContentDidChange();
}

void AdsTabHelper::ResetNavigationState() {
  redirect_chain_.clear();
  redirect_chain_.shrink_to_fit();

  http_status_code_.reset();

  media_players_.clear();
}

void AdsTabHelper::MaybeNotifyBrowserDidBecomeActive() {
  if (ads_service_) {
    ads_service_->NotifyBrowserDidBecomeActive();
  }
}

void AdsTabHelper::MaybeNotifyBrowserDidResignActive() {
  if (ads_service_) {
    ads_service_->NotifyBrowserDidResignActive();
  }
}

void AdsTabHelper::MaybeNotifyUserGestureEventTriggered(
    content::NavigationHandle* const navigation_handle) {
  CHECK(navigation_handle);

  if (!ads_service_) {
    return;
  }

  if (was_restored_) {
    // Don't notify user gesture events for restored tabs.
    return;
  }

  if (!navigation_handle->HasUserGesture() &&
      navigation_handle->IsRendererInitiated()) {
    // Some browser initiated navigations return `false` for `HasUserGesture` so
    // we must also check `IsRendererInitiated`. See crbug.com/617904.
    return;
  }

  const ui::PageTransition page_transition =
      navigation_handle->GetPageTransition();
  ads_service_->NotifyUserGestureEventTriggered(page_transition);
}

void AdsTabHelper::MaybeNotifyTabDidChange() {
  if (!ads_service_) {
    return;
  }

  if (redirect_chain_.empty()) {
    // Don't notify content changes if the tab redirect chain is empty, i.e.,
    // the web contents are still loading.
    return;
  }

  ads_service_->NotifyTabDidChange(/*tab_id=*/session_id_.id(), redirect_chain_,
                                   is_new_navigation_, was_restored_,
                                   IsVisible());
}

void AdsTabHelper::MaybeNotifyTabDidLoad() {
  CHECK(http_status_code_);

  if (!ads_service_) {
    // No-op if the ads service is unavailable.
    return;
  }

  ads_service_->NotifyTabDidLoad(/*tab_id=*/session_id_.id(),
                                 *http_status_code_);
}

bool AdsTabHelper::ShouldNotifyTabContentDidChange() const {
  // Don't notify about content changes if the ads service is not available, the
  // tab was restored, was a previously committed navigation, the web contents
  // are still loading, or an error page was displayed. `http_status_code_` can
  // be `std::nullopt` if the navigation never finishes which can occur if the
  // user constantly refreshes the page.
  return ads_service_ && !was_restored_ && is_new_navigation_ &&
         !redirect_chain_.empty() && http_status_code_ &&
         !IsErrorPage(*http_status_code_);
}

void AdsTabHelper::MaybeNotifyTabHtmlContentDidChange() {
  if (!ShouldNotifyTabContentDidChange()) {
    return;
  }

  if (!UserHasJoinedBraveRewards()) {
    // HTML is not required because verifiable conversions are only supported
    // for Brave Rewards users. However, we must notify that the tab content has
    // changed with empty HTML to ensure that regular conversions are processed.
    return ads_service_->NotifyTabHtmlContentDidChange(
        /*tab_id=*/session_id_.id(), redirect_chain_, /*html=*/"");
  }

  // Only utilized for verifiable conversions, which requires the user to have
  // joined Brave Rewards.
  web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      kSerializeDocumentToStringJavaScript,
      base::BindOnce(&AdsTabHelper::OnMaybeNotifyTabHtmlContentDidChange,
                     weak_factory_.GetWeakPtr(), redirect_chain_),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void AdsTabHelper::OnMaybeNotifyTabHtmlContentDidChange(
    const std::vector<GURL>& redirect_chain,
    base::Value value) {
  if (ads_service_ && value.is_string()) {
    ads_service_->NotifyTabHtmlContentDidChange(/*tab_id=*/session_id_.id(),
                                                redirect_chain,
                                                /*html=*/value.GetString());
  }
}

void AdsTabHelper::MaybeNotifyTabTextContentDidChange() {
  if (!ShouldNotifyTabContentDidChange()) {
    return;
  }

  if (UserHasOptedInToNotificationAds()) {
    // Only utilized for text classification, which requires the user to have
    // joined Brave Rewards and opted into notification ads.
    web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
        kDocumentBodyInnerTextJavaScript,
        base::BindOnce(&AdsTabHelper::OnMaybeNotifyTabTextContentDidChange,
                       weak_factory_.GetWeakPtr(), redirect_chain_),
        ISOLATED_WORLD_ID_BRAVE_INTERNAL);
  }
}

void AdsTabHelper::OnMaybeNotifyTabTextContentDidChange(
    const std::vector<GURL>& redirect_chain,
    base::Value value) {
  if (ads_service_ && value.is_string()) {
    ads_service_->NotifyTabTextContentDidChange(/*tab_id=*/session_id_.id(),
                                                redirect_chain,
                                                /*text=*/value.GetString());
  }
}

void AdsTabHelper::MaybeNotifyTabDidStartPlayingMedia() {
  if (ads_service_) {
    ads_service_->NotifyTabDidStartPlayingMedia(/*tab_id=*/session_id_.id());
  }
}

void AdsTabHelper::MaybeNotifyTabDidStopPlayingMedia() {
  if (ads_service_) {
    ads_service_->NotifyTabDidStopPlayingMedia(/*tab_id=*/session_id_.id());
  }
}

void AdsTabHelper::MaybeNotifyTabdidClose() {
  if (ads_service_) {
    ads_service_->NotifyDidCloseTab(/*tab_id=*/session_id_.id());
  }
}

void AdsTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!ads_service_) {
    // No-op if the ads service is unavailable.
    return;
  }

  if (!navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }

  was_restored_ =
      navigation_handle->GetRestoreType() == content::RestoreType::kRestored;

  is_new_navigation_ = IsNewNavigation(navigation_handle);

  ResetNavigationState();
}

// This method is called when a navigation in the main frame or a subframe has
// completed. It indicates that the navigation has finished, but the document
// might still be loading resources.
void AdsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!ads_service_) {
    return;
  }

  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }

  redirect_chain_ = navigation_handle->GetRedirectChain();

  http_status_code_ = HttpStatusCode(navigation_handle).value_or(net::HTTP_OK);

  MaybeNotifyUserGestureEventTriggered(navigation_handle);

  // Notify of tab changes after navigation completes but before notifying that
  // the tab has loaded, so that any listeners can process the tab changes
  // before the tab is considered loaded.
  MaybeNotifyTabDidChange();

  MaybeNotifyTabDidLoad();

  MaybeReportSearchQueryMetric(navigation_handle);

  // Process same document navigations only when a document load is completed.
  // For navigations that lead to a document change, `ProcessNavigation` is
  // called from `DocumentOnLoadCompletedInPrimaryMainFrame`.
  if (navigation_handle->IsSameDocument() &&
      web_contents()->IsDocumentOnLoadCompletedInPrimaryMainFrame()) {
    ProcessSameDocumentNavigation();

    // Set `was_restored_` to `false` so that listeners are notified of tab
    // changes after the tab is restored.
    was_restored_ = false;
  }
}

// This method is called when the document's onload event has fired in the
// primary main frame. This means that the document and all its subresources
// have finished loading.
void AdsTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  if (!ads_service_) {
    return;
  }

  ProcessNavigation();

  // Set `was_restored_` to `false` so that listeners are notified of tab
  // changes after the tab is restored.
  was_restored_ = false;
}

bool AdsTabHelper::IsPlayingMedia(const std::string& media_player_uuid) {
  return media_players_.contains(media_player_uuid);
}

void AdsTabHelper::MediaStartedPlaying(const MediaPlayerInfo& /*video_type*/,
                                       const content::MediaPlayerId& id) {
  const std::string media_player_uuid = MediaPlayerUuid(id);

  if (IsPlayingMedia(media_player_uuid)) {
    // Already playing media.
    return;
  }

  media_players_.insert(media_player_uuid);
  if (media_players_.size() == 1) {
    // If this is the first media player that has started playing, notify that
    // the tab has started playing media.
    MaybeNotifyTabDidStartPlayingMedia();
  }
}

void AdsTabHelper::MediaStoppedPlaying(
    const MediaPlayerInfo& /*video_type*/,
    const content::MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason /*reason*/) {
  const std::string media_player_uuid = MediaPlayerUuid(id);

  if (!IsPlayingMedia(media_player_uuid)) {
    // Not playing media.
    return;
  }

  media_players_.erase(media_player_uuid);
  if (media_players_.empty()) {
    // If this is the last media player that has stopped playing, notify that
    // the tab has stopped playing media.
    MaybeNotifyTabDidStopPlayingMedia();
  }
}

void AdsTabHelper::OnVisibilityChanged(const content::Visibility visibility) {
  const bool last_is_web_contents_visible = is_web_contents_visible_;
  is_web_contents_visible_ = visibility == content::Visibility::VISIBLE;
  if (last_is_web_contents_visible != is_web_contents_visible_) {
    MaybeNotifyTabDidChange();
  }
}

void AdsTabHelper::WebContentsDestroyed() {
  MaybeNotifyTabdidClose();

  ads_service_ = nullptr;
}

#if !BUILDFLAG(IS_ANDROID)
// TODO(https://github.com/brave/brave-browser/issues/24970): Decouple
// BrowserListObserver.

void AdsTabHelper::OnBrowserSetLastActive(Browser* /*browser*/) {
  MaybeSetBrowserIsActive();
}

void AdsTabHelper::OnBrowserNoLongerActive(Browser* /*browser*/) {
  MaybeSetBrowserIsNoLongerActive();
}
#endif

std::optional<std::string> AdsTabHelper::MaybeBuildSearchQueryMetricPayload(
    const GURL& url,
    ui::PageTransition page_transition) {
  std::optional<std::string> default_search_engine = DefaultSearchEngine();
  if (!default_search_engine) {
    LOG(INFO) << "[METRIC][DEBUG]: Unsupported default search engine";
    return std::nullopt;
  }

  std::optional<std::string> search_engine = SearchEngine(url);
  if (!search_engine) {
    LOG(INFO) << "[METRIC][DEBUG]: Unsupported search engine";
    return std::nullopt;
  }

  std::optional<std::string> country = Country();
  if (!country) {
    LOG(INFO) << "[METRIC][DEBUG]: Unsupported country";
    return std::nullopt;
  }

  auto dict = base::Value::Dict()
                  .Set("country", *country)
                  .Set("createdAt", CreatedAt())
                  .Set("defaultSearchEngine", *default_search_engine)
                  .Set("entryPoint", EntryPoint(page_transition))
                  .Set("isDefaultBrowser", IsDefaultBrowser())
                  .Set("isFirstQuery", IsFirstQuery())
                  .Set("language", Language())
                  .Set("platform", Platform())
                  .Set("searchEngine", *search_engine)
                  .Set("transactionId", TransactionId())
                  .Set("type", "query");
  LOG(INFO) << "[METRIC][DEBUG]:\n" << dict;

  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  return json;
}

void AdsTabHelper::MaybeReportSearchQueryMetric(
    content::NavigationHandle* navigation_handle) {
  CHECK(navigation_handle);

  if (!navigation_handle->HasUserGesture()) {
    // Only report search query metrics for navigations with a user gesture.
    return;
  }

  const GURL& url = navigation_handle->GetURL();

  const ui::PageTransition page_transition =
      navigation_handle->GetPageTransition();

  if (url.host() != "search.brave.com") {
    // If the user navigates away from `search.brave.com`, reset the search
    // widget entry point.
    is_search_widget_entry_point_ = false;
  }

  if (IsSearchEngine(url) &&
      ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_AUTO_BOOKMARK)) {
    is_bookmark_entry_point_ = true;
  }

  if (!IsSearchEngineResultsPage(url)) {
    LOG(INFO) << "[METRIC][DEBUG]: Not a search engine results page";
    return;
  }

  LogEntryPointForDebugging(page_transition);

  if (std::optional<std::string> payload =
          MaybeBuildSearchQueryMetricPayload(url, page_transition)) {
    ReportSearchQueryMetric(*payload);
  }

  is_search_widget_entry_point_ = false;

  is_bookmark_entry_point_ = false;
}

void AdsTabHelper::ReportSearchQueryMetric(const std::string& payload) {
  last_reported_search_query_metric_at = base::Time::Now();

  mojom::UrlRequestInfoPtr mojom_url_request = mojom::UrlRequestInfo::New();
  mojom_url_request->url =
      GURL("https://ohttp.metrics.bravesoftware.com/v1/ohttp/gateway");
  mojom_url_request->headers = {"accept: application/json"};
  mojom_url_request->content = payload;
  mojom_url_request->content_type = "application/json";
  mojom_url_request->method = mojom::UrlRequestMethodType::kPost;
  // TODO(tmancey): After https://github.com/brave/brave-browser/issues/50085
  // merges, uncomment `use_ohttp = true` and add OHTTP key config, and relay
  // URL endpoint support for metrics.
  // mojom_url_request->use_ohttp = true;

  LOG(INFO) << UrlRequestToString(mojom_url_request);

  network_client_->SendRequest(
      std::move(mojom_url_request),
      base::BindOnce(&AdsTabHelper::ReportSearchQueryMetricCallback,
                     weak_factory_.GetWeakPtr()));
}

void AdsTabHelper::ReportSearchQueryMetricCallback(
    mojom::UrlResponseInfoPtr mojom_url_response) {
  CHECK(mojom_url_response);

  LOG(INFO) << UrlResponseToString(*mojom_url_response);

  if (mojom_url_response->code < 200 || mojom_url_response->code > 399) {
    LOG(INFO) << "[METRIC] Failed to report search query metric";
    return;
  }

  LOG(INFO) << "[METRIC] Successfully reported search query metric";
}

std::optional<std::string> AdsTabHelper::Country() const {
  std::string country =
      base::ToUpperASCII(g_browser_process->local_state()->GetString(
          variations::prefs::kVariationsCountry));
  if (!kSupportedCountries.contains(country)) {
    // Unsupported country.
    return std::nullopt;
  }

  return country;
}

std::string AdsTabHelper::CreatedAt() const {
  return base::TimeFormatAsIso8601(base::Time::Now());
}

std::optional<std::string> AdsTabHelper::DefaultSearchEngine() const {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile);
  const TemplateURL* template_url =
      template_url_service->GetDefaultSearchProvider();
  if (!template_url) {
    // No default search engine.
    return std::nullopt;
  }

  const std::u16string& short_name = template_url->short_name();
  if (!kSupportedDefaultSearchEngines.contains(short_name)) {
    // Unsupported default search engine.
    return std::nullopt;
  }

  return base::UTF16ToUTF8(short_name);
}

std::string AdsTabHelper::EntryPoint(ui::PageTransition page_transition) {
  if (is_search_widget_entry_point_) {
    return "NTP";
  }

  if (is_bookmark_entry_point_ ||
      ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_AUTO_BOOKMARK)) {
    return "Bookmark";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_GENERATED)) {
    return "Omnibox Search";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_KEYWORD)) {
    return "Shortcut";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_FORM_SUBMIT)) {
    return "Direct";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition, ui::PAGE_TRANSITION_LINK)) {
    return "Top Site";
  }

  // TODO(tmancey): Add support for mobile quick search entry point.

  // TODO(tmancey): Add support for omnibox history entry point.

  // TODO(tmancey): Should we return "Other" for an unrecognized entry point?
  return "Other";
}

bool AdsTabHelper::IsDefaultBrowser() const {
  shell_integration::DefaultWebClientState state =
      shell_integration::GetDefaultBrowser();
  return state == shell_integration::IS_DEFAULT ||
         state == shell_integration::OTHER_MODE_IS_DEFAULT;
}

bool AdsTabHelper::IsFirstQuery() const {
  if (!last_reported_search_query_metric_at) {
    // First search query ever.
    return true;
  }

  base::Time::Exploded now_exploded;
  base::Time::Now().LocalExplode(&now_exploded);

  base::Time::Exploded last_reported_at_exploded;
  last_reported_search_query_metric_at->LocalExplode(
      &last_reported_at_exploded);

  return now_exploded.year != last_reported_at_exploded.year ||
         now_exploded.month != last_reported_at_exploded.month ||
         now_exploded.day_of_month != last_reported_at_exploded.day_of_month;
}

std::string AdsTabHelper::Language() const {
  return g_browser_process->GetApplicationLocale();
}

std::string AdsTabHelper::Platform() const {
#if BUILDFLAG(IS_MAC)
  return "macOS";
#elif BUILDFLAG(IS_WIN)
  return "Windows";
#elif BUILDFLAG(IS_LINUX)
  return "Linux";
#elif BUILDFLAG(IS_ANDROID)
  return "Android";
#elif BUILDFLAG(IS_IOS)
  return "iOS";
#else
  return "Unknown";
#endif
}

std::optional<std::string> AdsTabHelper::SearchEngine(const GURL& url) const {
  if (!url.is_valid()) {
    // Invalid URL.
    return std::nullopt;
  }

  switch (search_engine_utils::GetEngineType(url)) {
    case SEARCH_ENGINE_BRAVE:
      return "Brave";
    case SEARCH_ENGINE_GOOGLE:
      return "Google";
    case SEARCH_ENGINE_BING:
      return "Bing";
    case SEARCH_ENGINE_DUCKDUCKGO:
      return "DuckDuckGo";
    default:
      break;
  }

  if (url.host() == "yahoo.co.jp") {
    return "Yahoo JP";
  }

  if (url.host() == "chatgpt.com") {
    return "ChatGPT";
  }

  if (url.host() == "perplexity.ai") {
    return "Perplexity";
  }

  // Unsupported search engine.
  return std::nullopt;
}

std::string AdsTabHelper::TransactionId() const {
  return base::Uuid::GenerateRandomV4().AsLowercaseString();
}

void AdsTabHelper::LogEntryPointForDebugging(
    ui::PageTransition page_transition) const {
  LOG(INFO) << "[METRIC][DEBUG] Entry point for debugging:";

  if (ui::PageTransitionCoreTypeIs(page_transition, ui::PAGE_TRANSITION_LINK)) {
    LOG(INFO) << "  PAGE_TRANSITION_LINK";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_TYPED)) {
    LOG(INFO) << "  PAGE_TRANSITION_TYPED";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_AUTO_BOOKMARK)) {
    LOG(INFO) << "  PAGE_TRANSITION_AUTO_BOOKMARK";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_AUTO_SUBFRAME)) {
    LOG(INFO) << "  PAGE_TRANSITION_AUTO_SUBFRAME";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_MANUAL_SUBFRAME)) {
    LOG(INFO) << "  PAGE_TRANSITION_MANUAL_SUBFRAME";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_GENERATED)) {
    LOG(INFO) << "  PAGE_TRANSITION_GENERATED";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_AUTO_TOPLEVEL)) {
    LOG(INFO) << "  PAGE_TRANSITION_AUTO_TOPLEVEL";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_FORM_SUBMIT)) {
    LOG(INFO) << "  PAGE_TRANSITION_FORM_SUBMIT";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_RELOAD)) {
    LOG(INFO) << "  PAGE_TRANSITION_RELOAD";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_KEYWORD)) {
    LOG(INFO) << "  PAGE_TRANSITION_KEYWORD";
  }

  if (ui::PageTransitionCoreTypeIs(page_transition,
                                   ui::PAGE_TRANSITION_KEYWORD_GENERATED)) {
    LOG(INFO) << "  PAGE_TRANSITION_KEYWORD_GENERATED";
  }

  if ((ui::PageTransitionGetQualifier(page_transition) &
       ui::PAGE_TRANSITION_FROM_ADDRESS_BAR) != 0) {
    LOG(INFO) << "  PAGE_TRANSITION_FROM_ADDRESS_BAR";
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdsTabHelper);

}  // namespace brave_ads
