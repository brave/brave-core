/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/rand_util.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_browser_main_extra_parts.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/browser/net/brave_proxying_url_loader_factory.h"
#include "brave/browser/net/brave_proxying_web_socket.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/binance/browser/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_wallet/browser/buildflags/buildflags.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/services/brave_content_browser_overlay_manifest.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "components/services/heap_profiling/public/mojom/heap_profiling_client.mojom.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_names.mojom.h"
#include "extensions/buildflags/buildflags.h"
#include "net/cookies/site_for_cookies.h"
#include "services/service_manager/public/cpp/manifest_builder.h"
#include "ui/base/l10n/l10n_util.h"

using brave_shields::BraveShieldsWebContentsObserver;
using content::BrowserThread;
using content::ContentBrowserClient;
using content::RenderFrameHost;
using content::WebContents;

#if BUILDFLAG(BRAVE_ADS_ENABLED)
#include "brave/components/services/bat_ads/public/cpp/manifest.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/services/bat_ledger/public/cpp/manifest.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/chrome_content_browser_client_extensions_part.h"
#include "extensions/browser/extension_registry.h"
using extensions::ChromeContentBrowserClientExtensionsPart;
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/browser/content_browser_client_helper.h"
#include "brave/browser/extensions/brave_webtorrent_navigation_throttle.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/browser/rewards_protocol_handler.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_navigation_throttle.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/tor/switches.h"
#include "brave/components/services/tor/public/cpp/manifest.h"
#include "brave/components/services/tor/public/interfaces/tor.mojom.h"
#include "brave/components/services/tor/tor_launcher_service.h"
#endif

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/browser/extensions/brave_wallet_navigation_throttle.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_switches.h"
#include "brave/components/speedreader/speedreader_throttle.h"
#include "brave/components/speedreader/speedreader_whitelist.h"
#include "content/public/common/resource_type.h"
#endif

#if BUILDFLAG(BINANCE_ENABLED)
#include "brave/components/binance/browser/binance_protocol_handler.h"
#endif

namespace {

bool HandleURLReverseOverrideRewrite(GURL* url,
                                     content::BrowserContext* browser_context) {
  if (BraveContentBrowserClient::HandleURLOverrideRewrite(url, browser_context))
    return true;

  return false;
}

bool HandleURLRewrite(GURL* url, content::BrowserContext* browser_context) {
  if (BraveContentBrowserClient::HandleURLOverrideRewrite(url, browser_context))
    return true;

  return false;
}

}  // namespace

BraveContentBrowserClient::BraveContentBrowserClient(StartupData* startup_data)
    : ChromeContentBrowserClient(startup_data),
      session_token_(base::RandUint64()),
      incognito_session_token_(base::RandUint64()) {}

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

content::ContentBrowserClient::AllowWebBluetoothResult
BraveContentBrowserClient::AllowWebBluetooth(
    content::BrowserContext* browser_context,
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin) {
  return ContentBrowserClient::AllowWebBluetoothResult::BLOCK_GLOBALLY_DISABLED;
}

bool BraveContentBrowserClient::HandleExternalProtocol(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    int child_id,
    content::NavigationUIData* navigation_data,
    bool is_main_frame,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const base::Optional<url::Origin>& initiating_origin,
    mojo::PendingRemote<network::mojom::URLLoaderFactory>* out_factory) {
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  if (webtorrent::IsMagnetProtocol(url)) {
    webtorrent::HandleMagnetProtocol(url, std::move(web_contents_getter),
                                     page_transition, has_user_gesture,
                                     initiating_origin);
    return true;
  }
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  if (brave_rewards::IsRewardsProtocol(url)) {
    brave_rewards::HandleRewardsProtocol(url, std::move(web_contents_getter),
                                         page_transition, has_user_gesture);
    return true;
  }
#endif

#if BUILDFLAG(BINANCE_ENABLED)
  if (binance::IsBinanceProtocol(url)) {
    binance::HandleBinanceProtocol(url, std::move(web_contents_getter),
                                   page_transition, has_user_gesture);
    return true;
  }
#endif

  return ChromeContentBrowserClient::HandleExternalProtocol(
      url, std::move(web_contents_getter), child_id, navigation_data,
      is_main_frame, page_transition, has_user_gesture, initiating_origin,
      out_factory);
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

void BraveContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  ChromeContentBrowserClient::AppendExtraCommandLineSwitches(command_line,
                                                             child_process_id);
  std::string process_type =
      command_line->GetSwitchValueASCII(switches::kProcessType);
  if (process_type == switches::kRendererProcess) {
    uint64_t session_token =
        12345;  // the kinda thing an idiot would have on his luggage
    if (!command_line->HasSwitch(switches::kTestType)) {
      content::RenderProcessHost* process =
          content::RenderProcessHost::FromID(child_process_id);
      Profile* profile =
          process ? Profile::FromBrowserContext(process->GetBrowserContext())
                  : nullptr;
      if (profile && !profile->IsOffTheRecord()) {
        session_token = session_token_;
      } else {
        session_token = incognito_session_token_;
      }
    }
    command_line->AppendSwitchASCII("brave_session_token",
                                    base::NumberToString(session_token));
  }

  if (process_type == switches::kUtilityProcess) {
#if BUILDFLAG(ENABLE_TOR)
      // This is not ideal because it adds the tor executable as a switch
      // for every utility process, but it should be ok until we land a
      // permanent fix
      base::FilePath path =
          g_brave_browser_process->tor_client_updater()->GetExecutablePath();
      if (!path.empty()) {
        command_line->AppendSwitchPath(tor::switches::kTorExecutablePath,
                                       path.BaseName());
      }
#endif
  }
}

std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
BraveContentBrowserClient::CreateURLLoaderThrottles(
    const network::ResourceRequest& request,
    content::BrowserContext* browser_context,
    const base::RepeatingCallback<content::WebContents*()>& wc_getter,
    content::NavigationUIData* navigation_ui_data,
    int frame_tree_node_id) {
  auto result = ChromeContentBrowserClient::CreateURLLoaderThrottles(
        request, browser_context, wc_getter, navigation_ui_data,
        frame_tree_node_id);
#if BUILDFLAG(ENABLE_SPEEDREADER)
  const auto* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(speedreader::kEnableSpeedreader)) {
    // Work only with casual main frame navigations.
    if (request.url.SchemeIsHTTPOrHTTPS() &&
        request.resource_type ==
        static_cast<int>(content::ResourceType::kMainFrame)) {
      // Note that we check the whitelist before any redirects, while distilling
      // will be performed on a final document (the last in the redirect chain).
      auto* whitelist = g_brave_browser_process->speedreader_whitelist();
      if (speedreader::IsWhitelisted(request.url) ||
          whitelist->IsWhitelisted(request.url)) {
        result.push_back(std::make_unique<speedreader::SpeedReaderThrottle>(
                         base::ThreadTaskRunnerHandle::Get()));
      }
    }
  }
#endif  // ENABLE_SPEEDREADER
  return result;
}

bool BraveContentBrowserClient::WillCreateURLLoaderFactory(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* frame,
    int render_process_id,
    URLLoaderFactoryType type,
    const url::Origin& request_initiator,
    base::Optional<int64_t> navigation_id,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
        header_client,
    bool* bypass_redirect_checks,
    bool* disable_secure_dns,
    network::mojom::URLLoaderFactoryOverridePtr* factory_override) {
  bool use_proxy = false;
  // TODO(iefremov): Skip proxying for certain requests?
  use_proxy = BraveProxyingURLLoaderFactory::MaybeProxyRequest(
      browser_context, frame,
      type == URLLoaderFactoryType::kNavigation ? -1 : render_process_id,
      factory_receiver);

  use_proxy |= ChromeContentBrowserClient::WillCreateURLLoaderFactory(
      browser_context, frame, render_process_id, type, request_initiator,
      std::move(navigation_id), factory_receiver, header_client,
      bypass_redirect_checks, disable_secure_dns, factory_override);

  return use_proxy;
}

bool BraveContentBrowserClient::WillInterceptWebSocket(
    content::RenderFrameHost* frame) {
  return (frame != nullptr);
}

void BraveContentBrowserClient::CreateWebSocket(
    content::RenderFrameHost* frame,
    content::ContentBrowserClient::WebSocketFactory factory,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const base::Optional<std::string>& user_agent,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client) {
  auto* proxy = BraveProxyingWebSocket::ProxyWebSocket(
      frame,
      std::move(factory),
      url,
      site_for_cookies,
      user_agent,
      std::move(handshake_client));

  if (ChromeContentBrowserClient::WillInterceptWebSocket(frame)) {
    ChromeContentBrowserClient::CreateWebSocket(
        frame,
        proxy->web_socket_factory(),
        url,
        site_for_cookies,
        user_agent,
        proxy->handshake_client().Unbind());
  } else {
    proxy->Start();
  }
}

void BraveContentBrowserClient::MaybeHideReferrer(
    content::BrowserContext* browser_context,
    const GURL& request_url,
    const GURL& document_url,
    bool is_main_frame,
    blink::mojom::ReferrerPtr* referrer) {
  DCHECK(referrer && !referrer->is_null());
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (document_url.SchemeIs(kChromeExtensionScheme)) {
    return;
  }
#endif

  Profile* profile = Profile::FromBrowserContext(browser_context);
  const bool allow_referrers = brave_shields::AllowReferrers(profile,
                                                             document_url);
  const bool shields_up = brave_shields::GetBraveShieldsEnabled(profile,
                                                                document_url);
  // Top-level navigations get empty referrers (brave/brave-browser#3422).
  GURL replacement_referrer_url;
  if (!is_main_frame) {
    // But iframe navigations get spoofed instead (brave/brave-browser#3988).
    replacement_referrer_url = request_url.GetOrigin();
  }
  content::Referrer new_referrer;
  if (brave_shields::ShouldSetReferrer(
      allow_referrers, shields_up, (*referrer)->url, document_url, request_url,
          replacement_referrer_url, (*referrer)->policy, &new_referrer)) {
    (*referrer)->url = new_referrer.url;
    (*referrer)->policy = new_referrer.policy;
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

// [static]
bool BraveContentBrowserClient::HandleURLOverrideRewrite(GURL* url,
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
  if (url->host() == chrome::kChromeUIWelcomeHost) {
    *url = GURL(chrome::kChromeUIWelcomeURL);
    return true;
  }

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  if (url->SchemeIs(content::kChromeUIScheme) &&
      url->host() == ethereum_remote_client_host) {
    auto* registry = extensions::ExtensionRegistry::Get(browser_context);
    if (registry->ready_extensions().GetByID(
        ethereum_remote_client_extension_id)) {
      *url = GURL(ethereum_remote_client_base_url);
      return true;
    }
  }
#endif

  return false;
}

std::vector<std::unique_ptr<content::NavigationThrottle>>
BraveContentBrowserClient::CreateThrottlesForNavigation(
    content::NavigationHandle* handle) {
  std::vector<std::unique_ptr<content::NavigationThrottle>> throttles =
      ChromeContentBrowserClient::CreateThrottlesForNavigation(handle);

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  throttles.push_back(
      std::make_unique<extensions::BraveWalletNavigationThrottle>(handle));
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  throttles.push_back(
      std::make_unique<extensions::BraveWebTorrentNavigationThrottle>(handle));
#endif

#if BUILDFLAG(ENABLE_TOR)
  std::unique_ptr<content::NavigationThrottle> tor_navigation_throttle =
    tor::TorNavigationThrottle::MaybeCreateThrottleFor(handle);
  if (tor_navigation_throttle)
    throttles.push_back(std::move(tor_navigation_throttle));
#endif

  return throttles;
}
