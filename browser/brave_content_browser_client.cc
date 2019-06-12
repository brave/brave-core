/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_browser_main_extra_parts.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/browser/renderer_host/brave_navigation_ui_data.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/brave_cookie_blocking.h"
#include "brave/common/tor/switches.h"
#include "brave/common/tor/tor_launcher.mojom.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/browser/buildflags/buildflags.h"  // For STP
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/content_settings/core/browser/brave_cookie_settings.h"
#include "brave/components/services/bat_ads/public/cpp/manifest.h"
#include "brave/components/services/bat_ledger/public/cpp/manifest.h"
#include "brave/components/services/brave_content_browser_overlay_manifest.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/utility/tor/public/cpp/manifest.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/url_constants.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/services/heap_profiling/public/mojom/heap_profiling_client.mojom.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/service_names.mojom.h"
#include "extensions/buildflags/buildflags.h"
#include "services/service_manager/public/cpp/manifest_builder.h"
#include "ui/base/l10n/l10n_util.h"

using brave_shields::BraveShieldsWebContentsObserver;
using content::BrowserThread;
using content::ContentBrowserClient;
using content::RenderFrameHost;
using content::WebContents;

#if BUILDFLAG(BRAVE_ADS_ENABLED)
#include "brave/components/services/bat_ads/bat_ads_app.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/services/bat_ledger/bat_ledger_app.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/chrome_content_browser_client_extensions_part.h"
using extensions::ChromeContentBrowserClientExtensionsPart;
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/browser/content_browser_client_helper.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/tor/tor_launcher.mojom.h"
#include "brave/utility/tor/tor_launcher_service.h"
#endif

namespace {

bool HandleURLOverrideRewrite(GURL* url,
                              content::BrowserContext* browser_context) {
  // redirect sync-internals
  if (url->host() == chrome::kChromeUISyncInternalsHost ||
      url->host() == chrome::kChromeUISyncHost) {
    GURL::Replacements replacements;
    replacements.SetHostStr(chrome::kChromeUISyncHost);
    *url = url->ReplaceComponents(replacements);
    return true;
  }

  // no special win10 welcome page
  if (url->host() == chrome::kChromeUIWelcomeWin10Host ||
      url->host() == chrome::kChromeUIWelcomeHost) {
    *url = GURL(chrome::kChromeUIWelcomeURL);
    return true;
  }

  return false;
}

bool HandleURLReverseOverrideRewrite(GURL* url,
                                     content::BrowserContext* browser_context) {
  if (HandleURLOverrideRewrite(url, browser_context))
    return true;

  return false;
}

bool HandleURLRewrite(GURL* url, content::BrowserContext* browser_context) {
  if (HandleURLOverrideRewrite(url, browser_context))
    return true;

  return false;
}

}  // namespace

BraveContentBrowserClient::BraveContentBrowserClient(StartupData* startup_data)
    : ChromeContentBrowserClient(startup_data) {}

BraveContentBrowserClient::~BraveContentBrowserClient() {}

std::unique_ptr<content::BrowserMainParts>
BraveContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  std::unique_ptr<content::BrowserMainParts> main_parts =
      ChromeContentBrowserClient::CreateBrowserMainParts(parameters);
  ChromeBrowserMainParts* chrome_main_parts =
      static_cast<ChromeBrowserMainParts*>(main_parts.get());
  chrome_main_parts->AddParts(new BraveBrowserMainExtraParts());
  return main_parts;
}

void BraveContentBrowserClient::BrowserURLHandlerCreated(
    content::BrowserURLHandler* handler) {
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  handler->AddHandlerPair(&webtorrent::HandleMagnetURLRewrite,
                          content::BrowserURLHandler::null_handler());
  handler->AddHandlerPair(&webtorrent::HandleTorrentURLRewrite,
                          &webtorrent::HandleTorrentURLReverseRewrite);
#endif
  handler->AddHandlerPair(&HandleURLRewrite, &HandleURLReverseOverrideRewrite);
  ChromeContentBrowserClient::BrowserURLHandlerCreated(handler);
}

bool BraveContentBrowserClient::AllowAccessCookie(
    const GURL& url,
    const GURL& first_party,
    content::ResourceContext* context,
    int render_process_id,
    int render_frame_id) {
  GURL tab_origin =
      BraveShieldsWebContentsObserver::GetTabURLFromRenderFrameInfo(
          render_process_id, render_frame_id, -1)
          .GetOrigin();
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(context);
  bool allow_brave_shields =
      brave_shields::IsAllowContentSettingWithIOData(
          io_data, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kBraveShields) &&
#if BUILDFLAG(ENABLE_EXTENSIONS)
      !first_party.SchemeIs(kChromeExtensionScheme);
#else
      true;
#endif
  bool allow_1p_cookies = brave_shields::IsAllowContentSettingWithIOData(
      io_data, tab_origin, GURL("https://firstParty/"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  bool allow_3p_cookies = brave_shields::IsAllowContentSettingWithIOData(
      io_data, tab_origin, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kCookies);
  content_settings::BraveCookieSettings* cookie_settings =
      (content_settings::BraveCookieSettings*)io_data->GetCookieSettings();
  bool allow =
      !ShouldBlockCookie(allow_brave_shields, allow_1p_cookies,
                         allow_3p_cookies, first_party, url,
                         cookie_settings->GetAllowGoogleAuth()) &&
      g_brave_browser_process->tracking_protection_service()->ShouldStoreState(
          cookie_settings, io_data->GetHostContentSettingsMap(),
          render_process_id, render_frame_id, url, first_party, tab_origin);
  return allow;
}

bool BraveContentBrowserClient::AllowGetCookie(
    const GURL& url,
    const GURL& first_party,
    const net::CookieList& cookie_list,
    content::ResourceContext* context,
    int render_process_id,
    int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bool allow = AllowAccessCookie(url, first_party, context, render_process_id,
                                 render_frame_id);
  OnCookiesRead(render_process_id, render_frame_id, url, first_party,
                cookie_list, !allow);

  return allow;
}

bool BraveContentBrowserClient::AllowSetCookie(
    const GURL& url,
    const GURL& first_party,
    const net::CanonicalCookie& cookie,
    content::ResourceContext* context,
    int render_process_id,
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
  return ContentBrowserClient::AllowWebBluetoothResult::BLOCK_GLOBALLY_DISABLED;
}

bool BraveContentBrowserClient::HandleExternalProtocol(
    const GURL& url,
    content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
    int child_id,
    content::NavigationUIData* navigation_data,
    bool is_main_frame,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    network::mojom::URLLoaderFactoryRequest* factory_request,
    network::mojom::URLLoaderFactory*& out_factory) {  // NOLINT
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  if (webtorrent::HandleMagnetProtocol(url, web_contents_getter,
                                       page_transition, has_user_gesture)) {
    return true;
  }
#endif

  return ChromeContentBrowserClient::HandleExternalProtocol(
      url, web_contents_getter, child_id, navigation_data, is_main_frame,
      page_transition, has_user_gesture, factory_request, out_factory);
}

std::unique_ptr<content::NavigationUIData>
BraveContentBrowserClient::GetNavigationUIData(
    content::NavigationHandle* navigation_handle) {
  std::unique_ptr<BraveNavigationUIData> navigation_ui_data =
      std::make_unique<BraveNavigationUIData>(navigation_handle);
#if BUILDFLAG(ENABLE_TOR)
  Profile* profile = Profile::FromBrowserContext(
      navigation_handle->GetWebContents()->GetBrowserContext());
  TorProfileServiceFactory::SetTorNavigationUIData(profile,
                                                   navigation_ui_data.get());
#endif
  return std::move(navigation_ui_data);
}

void BraveContentBrowserClient::RunServiceInstance(
    const service_manager::Identity& identity,
    mojo::PendingReceiver<service_manager::mojom::Service>* receiver) {
  ChromeContentBrowserClient::RunServiceInstance(identity, receiver);
  const std::string& service_name = identity.name();
#if BUILDFLAG(ENABLE_TOR)
  if (service_name == tor::mojom::kTorLauncherServiceName) {
    service_manager::Service::RunAsyncUntilTermination(
        std::make_unique<tor::TorLauncherService>(std::move(*receiver)));
    return;
  }
#endif
#if BUILDFLAG(BRAVE_ADS_ENABLED)
  if (service_name == bat_ads::mojom::kServiceName) {
    service_manager::Service::RunAsyncUntilTermination(
        std::make_unique<bat_ads::BatAdsApp>(std::move(*receiver)));
    return;
  }
#endif
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  if (service_name == bat_ledger::mojom::kServiceName) {
    service_manager::Service::RunAsyncUntilTermination(
        std::make_unique<bat_ledger::BatLedgerApp>(std::move(*receiver)));
    return;
  }
#endif
}

base::Optional<service_manager::Manifest>
BraveContentBrowserClient::GetServiceManifestOverlay(base::StringPiece name) {
  auto manifest = ChromeContentBrowserClient::GetServiceManifestOverlay(name);
  if (name == content::mojom::kBrowserServiceName) {
    manifest->Amend(GetBraveContentBrowserOverlayManifest());
  }
  return manifest;
}

std::vector<service_manager::Manifest>
BraveContentBrowserClient::GetExtraServiceManifests() {
  auto manifests = ChromeContentBrowserClient::GetExtraServiceManifests();

#if BUILDFLAG(ENABLE_TOR)
  manifests.push_back(tor::GetTorLauncherManifest());
#endif
#if BUILDFLAG(BRAVE_ADS_ENABLED)
  manifests.push_back(bat_ads::GetManifest());
#endif
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  manifests.push_back(bat_ledger::GetManifest());
#endif

  return manifests;
}

void BraveContentBrowserClient::AdjustUtilityServiceProcessCommandLine(
    const service_manager::Identity& identity,
    base::CommandLine* command_line) {
  ChromeContentBrowserClient::AdjustUtilityServiceProcessCommandLine(
      identity, command_line);

#if BUILDFLAG(ENABLE_TOR)
  if (identity.name() == tor::mojom::kTorLauncherServiceName) {
    base::FilePath path =
        g_brave_browser_process->tor_client_updater()->GetExecutablePath();
    DCHECK(!path.empty());
    command_line->AppendSwitchPath(tor::switches::kTorExecutablePath,
                                   path.BaseName());
  }
#endif
}

void BraveContentBrowserClient::MaybeHideReferrer(
    content::BrowserContext* browser_context,
    const GURL& request_url,
    const GURL& document_url,
    bool is_main_frame,
    content::Referrer* referrer) {
  DCHECK(referrer);
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (document_url.SchemeIs(kChromeExtensionScheme)) {
    return;
  }
#endif

  Profile* profile = Profile::FromBrowserContext(browser_context);
  const bool allow_referrers = brave_shields::IsAllowContentSettingsForProfile(
      profile, document_url, document_url, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kReferrers);
  const bool shields_up = brave_shields::IsAllowContentSettingsForProfile(
      profile, document_url, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields);
  // Top-level navigations get empty referrers (brave/brave-browser#3422).
  GURL replacement_referrer_url;
  if (!is_main_frame) {
    // But iframe navigations get spoofed instead (brave/brave-browser#3988).
    replacement_referrer_url = request_url.GetOrigin();
  }
  brave_shields::ShouldSetReferrer(
      allow_referrers, shields_up, referrer->url, document_url, request_url,
      replacement_referrer_url, referrer->policy, referrer);
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
