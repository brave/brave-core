/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_browser_main_extra_parts.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/browser/renderer_host/brave_navigation_ui_data.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/brave_cookie_blocking.h"
#include "brave/common/webui_url_constants.h"
#include "brave/common/tor/tor_launcher.mojom.h"
#include "brave/common/tor/switches.h"
#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_webtorrent/browser/content_browser_client_helper.h"
#include "brave/components/content_settings/core/browser/brave_cookie_settings.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/extensions/chrome_content_browser_client_extensions_part.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/url_constants.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/service_names.mojom.h"
#include "extensions/buildflags/buildflags.h"
#include "services/service_manager/embedder/manifest_utils.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

using content::BrowserThread;
using content::RenderFrameHost;
using content::WebContents;
using brave_shields::BraveShieldsWebContentsObserver;

#if BUILDFLAG(BRAVE_ADS_ENABLED)
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
using extensions::ChromeContentBrowserClientExtensionsPart;
#endif

namespace {

bool HandleURLRewrite(GURL* url,
                      content::BrowserContext* browser_context) {
  if (url->SchemeIs(content::kChromeUIScheme) &&
      (url->host() == chrome::kChromeUIWelcomeHost ||
       url->host() == chrome::kChromeUIWelcomeWin10Host)) {
    *url = GURL(kBraveUIWelcomeURL);
    return true;
  }
  if (url->SchemeIs(content::kChromeUIScheme) &&
      (url->host() == kBraveUISyncHost)) {
    *url = GURL(kBraveUISyncURL);
    return true;
  }

  return false;
}

bool HandleURLReverseRewrite(GURL* url,
                             content::BrowserContext* browser_context) {
  if (url->spec() == kBraveUIWelcomeURL ||
      url->spec() == kBraveUISyncURL) {
    return true;
  }
  return false;
}

}  // namespace

BraveContentBrowserClient::BraveContentBrowserClient(ChromeFeatureListCreator* chrome_feature_list_creator) :
    ChromeContentBrowserClient(chrome_feature_list_creator)
{}

BraveContentBrowserClient::~BraveContentBrowserClient() {}

content::BrowserMainParts* BraveContentBrowserClient::CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) {
  ChromeBrowserMainParts* main_parts = (ChromeBrowserMainParts*)
      ChromeContentBrowserClient::CreateBrowserMainParts(parameters);
  main_parts->AddParts(new BraveBrowserMainExtraParts());
  return main_parts;
}

void BraveContentBrowserClient::BrowserURLHandlerCreated(
    content::BrowserURLHandler* handler) {
  handler->AddHandlerPair(&webtorrent::HandleMagnetURLRewrite,
                          content::BrowserURLHandler::null_handler());
  handler->AddHandlerPair(&webtorrent::HandleTorrentURLRewrite,
                          &webtorrent::HandleTorrentURLReverseRewrite);
  handler->AddHandlerPair(&HandleURLRewrite,
                          &HandleURLReverseRewrite);
  ChromeContentBrowserClient::BrowserURLHandlerCreated(handler);
}

bool BraveContentBrowserClient::AllowAccessCookie(const GURL& url, const GURL& first_party,
    content::ResourceContext* context, int render_process_id, int render_frame_id) {
  GURL tab_origin =
      BraveShieldsWebContentsObserver::GetTabURLFromRenderFrameInfo(
          render_process_id, render_frame_id, -1).GetOrigin();
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(context);
  bool allow_brave_shields =
      brave_shields::IsAllowContentSettingWithIOData(
          io_data, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kBraveShields) &&
      !first_party.SchemeIs(kChromeExtensionScheme);
  bool allow_1p_cookies = brave_shields::IsAllowContentSettingWithIOData(
      io_data, tab_origin, GURL("https://firstParty/"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  bool allow_3p_cookies = brave_shields::IsAllowContentSettingWithIOData(
      io_data, tab_origin, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kCookies);
  content_settings::BraveCookieSettings* cookie_settings =
      (content_settings::BraveCookieSettings*)io_data->GetCookieSettings();
  bool allow = !ShouldBlockCookie(allow_brave_shields, allow_1p_cookies,
                   allow_3p_cookies, first_party, url) &&
      cookie_settings->IsCookieAccessAllowed(url, first_party, tab_origin);
  return allow;
}

bool BraveContentBrowserClient::AllowGetCookie(const GURL& url,
    const GURL& first_party, const net::CookieList& cookie_list,
    content::ResourceContext* context, int render_process_id,
    int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bool allow = AllowAccessCookie(url, first_party, context, render_process_id,
                                 render_frame_id);
  OnCookiesRead(render_process_id, render_frame_id, url, first_party,
                cookie_list, !allow);

  return allow;
}

bool BraveContentBrowserClient::AllowSetCookie(const GURL& url,
    const GURL& first_party, const net::CanonicalCookie& cookie,
    content::ResourceContext* context, int render_process_id,
    int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bool allow = AllowAccessCookie(url, first_party, context, render_process_id,
                                 render_frame_id);
  OnCookieChange(render_process_id, render_frame_id, url, first_party, cookie,
                 !allow);
  return allow;
}

content::ContentBrowserClient::AllowWebBluetoothResult
BraveContentBrowserClient::AllowWebBluetooth(
    content::BrowserContext* browser_context,
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin) {
  return content::ContentBrowserClient::AllowWebBluetoothResult::BLOCK_GLOBALLY_DISABLED;
}

bool BraveContentBrowserClient::HandleExternalProtocol(
      const GURL& url,
      content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
      int child_id,
      content::NavigationUIData* navigation_data,
      bool is_main_frame,
      ui::PageTransition page_transition,
      bool has_user_gesture,
      const std::string& method,
      const net::HttpRequestHeaders& headers) {
  if (webtorrent::HandleMagnetProtocol(url, web_contents_getter,
        page_transition, has_user_gesture)) {
    return true;
  }

  return ChromeContentBrowserClient::HandleExternalProtocol(
      url, web_contents_getter, child_id, navigation_data, is_main_frame, page_transition, has_user_gesture, method, headers);
}

void BraveContentBrowserClient::RegisterOutOfProcessServices(
      OutOfProcessServiceMap* services) {
  ChromeContentBrowserClient::RegisterOutOfProcessServices(services);
  (*services)[tor::mojom::kTorLauncherServiceName] = base::BindRepeating(
    l10n_util::GetStringUTF16, IDS_UTILITY_PROCESS_TOR_LAUNCHER_NAME);
#if BUILDFLAG(BRAVE_ADS_ENABLED)
  (*services)[bat_ads::mojom::kServiceName] = base::BindRepeating(
    l10n_util::GetStringUTF16, IDS_SERVICE_BAT_ADS);
#endif
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  (*services)[bat_ledger::mojom::kServiceName] = base::BindRepeating(
    l10n_util::GetStringUTF16, IDS_UTILITY_PROCESS_LEDGER_NAME);
#endif
}

std::unique_ptr<content::NavigationUIData>
BraveContentBrowserClient::GetNavigationUIData(
      content::NavigationHandle* navigation_handle) {
  std::unique_ptr<BraveNavigationUIData> navigation_ui_data =
    std::make_unique<BraveNavigationUIData>(navigation_handle);
  Profile* profile =
    Profile::FromBrowserContext(navigation_handle->GetWebContents()
                                ->GetBrowserContext());
  TorProfileServiceFactory::SetTorNavigationUIData(profile,
                                                   navigation_ui_data.get());
  return std::move(navigation_ui_data);

}

std::unique_ptr<base::Value>
BraveContentBrowserClient::GetServiceManifestOverlay(base::StringPiece name) {
  auto chrome_overlay =
    ChromeContentBrowserClient::GetServiceManifestOverlay(name);

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  int id = -1;
  if (name == content::mojom::kBrowserServiceName)
    id = IDR_BRAVE_CONTENT_BROWSER_MANIFEST_OVERLAY;
  else if (name == content::mojom::kPackagedServicesServiceName)
    id = IDR_BRAVE_CONTENT_PACKAGED_SERVICES_MANIFEST_OVERLAY;
  else
    return chrome_overlay;

  base::StringPiece manifest_contents =
      rb.GetRawDataResourceForScale(id, ui::ScaleFactor::SCALE_FACTOR_NONE);

  auto brave_overlay = base::JSONReader::Read(manifest_contents);

  service_manager::MergeManifestWithOverlay(brave_overlay.get(),
                                            chrome_overlay.get());

  return brave_overlay;
}

void BraveContentBrowserClient::AdjustUtilityServiceProcessCommandLine(
    const service_manager::Identity& identity,
    base::CommandLine* command_line) {
  ChromeContentBrowserClient::AdjustUtilityServiceProcessCommandLine(
    identity, command_line);

  if (identity.name() == tor::mojom::kTorLauncherServiceName) {
    base::FilePath path =
      g_brave_browser_process->tor_client_updater()->GetExecutablePath();
    DCHECK(!path.empty());
    command_line->AppendSwitchPath(tor::switches::kTorExecutablePath,
                                   path.BaseName());
  }
}

GURL BraveContentBrowserClient::GetEffectiveURL(
    content::BrowserContext* browser_context,
    const GURL& url) {
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (!profile)
    return url;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  return ChromeContentBrowserClientExtensionsPart::GetEffectiveURL(profile,
                                                                   url);
#else
  return url;
#endif
}
