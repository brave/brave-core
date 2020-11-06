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
#include "brave/browser/net/brave_proxying_url_loader_factory.h"
#include "brave/browser/net/brave_proxying_web_socket.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/binance/browser/buildflags/buildflags.h"
#include "brave/components/gemini/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_wallet/buildflags/buildflags.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/url_constants.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/prefs/pref_service.h"
#include "components/services/heap_profiling/public/mojom/heap_profiling_client.mojom.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_names.mojom.h"
#include "extensions/buildflags/buildflags.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/cookies/site_for_cookies.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "ui/base/l10n/l10n_util.h"

using brave_shields::BraveShieldsWebContentsObserver;
using content::BrowserThread;
using content::ContentBrowserClient;
using content::RenderFrameHost;
using content::WebContents;

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/chrome_content_browser_client_extensions_part.h"
#include "extensions/browser/extension_registry.h"
using extensions::ChromeContentBrowserClientExtensionsPart;
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/browser/content_browser_client_helper.h"
#include "brave/browser/extensions/brave_webtorrent_navigation_throttle.h"
#endif

#if BUILDFLAG(IPFS_ENABLED)
#include "brave/browser/ipfs/content_browser_client_helper.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_navigation_throttle.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/browser/rewards_protocol_handler.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/onion_location_navigation_throttle_delegate.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/onion_location_navigation_throttle.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/components/speedreader/speedreader_throttle.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#endif

#if BUILDFLAG(BINANCE_ENABLED)
#include "brave/browser/binance/binance_protocol_handler.h"
#endif

#if BUILDFLAG(GEMINI_ENABLED)
#include "brave/browser/gemini/gemini_protocol_handler.h"
#endif

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/brave_wallet_constants.h"
#include "brave/components/brave_wallet/brave_wallet_service.h"
#endif

#if !defined(OS_ANDROID)
#include "brave/browser/new_tab/new_tab_shows_navigation_throttle.h"
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

BraveContentBrowserClient::BraveContentBrowserClient()
    : session_token_(base::RandUint64()),
      incognito_session_token_(base::RandUint64()) {}

BraveContentBrowserClient::~BraveContentBrowserClient() {}

std::unique_ptr<content::BrowserMainParts>
BraveContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  std::unique_ptr<content::BrowserMainParts> main_parts =
      ChromeContentBrowserClient::CreateBrowserMainParts(parameters);
  ChromeBrowserMainParts* chrome_main_parts =
      static_cast<ChromeBrowserMainParts*>(main_parts.get());
  chrome_main_parts->AddParts(std::make_unique<BraveBrowserMainExtraParts>());
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
#if BUILDFLAG(IPFS_ENABLED)
  handler->AddHandlerPair(
      &ipfs::HandleIPFSURLRewrite,
      &ipfs::HandleIPFSURLReverseRewrite);
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
                                   page_transition, has_user_gesture,
                                   initiating_origin);
    return true;
  }
#endif

#if BUILDFLAG(GEMINI_ENABLED)
  if (gemini::IsGeminiProtocol(url)) {
    gemini::HandleGeminiProtocol(url, std::move(web_contents_getter),
                                 page_transition, has_user_gesture,
                                 initiating_origin);
    return true;
  }
#endif

  return ChromeContentBrowserClient::HandleExternalProtocol(
      url, std::move(web_contents_getter), child_id, navigation_data,
      is_main_frame, page_transition, has_user_gesture, initiating_origin,
      out_factory);
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
  content::WebContents* contents = wc_getter.Run();
  if (!contents) {
    return result;
  }
  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents);
  if (tab_helper && tab_helper->IsActiveForMainFrame()
      && request.resource_type
          == static_cast<int>(blink::mojom::ResourceType::kMainFrame)) {
    result.push_back(std::make_unique<speedreader::SpeedReaderThrottle>(
        g_brave_browser_process->speedreader_rewriter_service(),
        base::ThreadTaskRunnerHandle::Get()));
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
    ukm::SourceIdObj ukm_source_id,
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
      std::move(navigation_id), ukm_source_id, factory_receiver, header_client,
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
    const std::string& method,
    blink::mojom::ReferrerPtr* referrer) {
  DCHECK(referrer && !referrer->is_null());
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (document_url.SchemeIs(kChromeExtensionScheme) ||
      request_url.SchemeIs(kChromeExtensionScheme)) {
    return;
  }
#endif
  if (document_url.SchemeIs(content::kChromeUIScheme) ||
      request_url.SchemeIs(content::kChromeUIScheme)) {
    return;
  }

  Profile* profile = Profile::FromBrowserContext(browser_context);
  const bool allow_referrers = brave_shields::AllowReferrers(
      HostContentSettingsMapFactory::GetForProfile(profile),
      document_url);
  const bool shields_up = brave_shields::GetBraveShieldsEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile),
      document_url);

  // Some top-level navigations get empty referrers (brave/brave-browser#3422).
  network::mojom::ReferrerPolicy policy = (*referrer)->policy;
  if (is_main_frame &&
      brave_shields::ShouldCleanReferrerForTopLevelNavigation(method,
                                                              (*referrer)->url,
                                                              request_url)) {
    policy = network::mojom::ReferrerPolicy::kNever;
  }

  content::Referrer new_referrer;
  if (brave_shields::MaybeChangeReferrer(
          allow_referrers, shields_up, (*referrer)->url, request_url, policy,
          &new_referrer)) {
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

  if (url->host() == chrome::kChromeUISyncHost) {
    GURL::Replacements replacements;
    replacements.SetHostStr(chrome::kChromeUISettingsHost);
    replacements.SetPathStr(kBraveSyncPath);
    *url = url->ReplaceComponents(replacements);
    return true;
  }

  // no special win10 welcome page
  if (url->host() == chrome::kChromeUIWelcomeHost) {
    *url = GURL(chrome::kChromeUIWelcomeURL);
    return true;
  }

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  // If the Crypto Wallets extension is loaded, then it replaces the WebUI
  Profile* profile = Profile::FromBrowserContext(browser_context);
  auto* service = BraveWalletServiceFactory::GetForProfile(profile);
  if (service->IsCryptoWalletsReady() &&
      url->SchemeIs(content::kChromeUIScheme) &&
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

#if !defined(OS_ANDROID)
  std::unique_ptr<content::NavigationThrottle> ntp_shows_navigation_throttle =
      NewTabShowsNavigationThrottle::MaybeCreateThrottleFor(handle);
  if (ntp_shows_navigation_throttle)
    throttles.push_back(std::move(ntp_shows_navigation_throttle));
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  throttles.push_back(
      std::make_unique<extensions::BraveWebTorrentNavigationThrottle>(handle));
#endif

#if BUILDFLAG(ENABLE_TOR) ||BUILDFLAG(IPFS_ENABLED)
  content::BrowserContext* context =
      handle->GetWebContents()->GetBrowserContext();
#endif

#if BUILDFLAG(ENABLE_TOR)
  std::unique_ptr<content::NavigationThrottle> tor_navigation_throttle =
    tor::TorNavigationThrottle::MaybeCreateThrottleFor(handle,
        TorProfileServiceFactory::GetForContext(context),
        context->IsTor());
  if (tor_navigation_throttle)
    throttles.push_back(std::move(tor_navigation_throttle));
  std::unique_ptr<tor::OnionLocationNavigationThrottleDelegate>
      onion_location_navigation_throttle_delegate =
          std::make_unique<tor::OnionLocationNavigationThrottleDelegate>();
  std::unique_ptr<content::NavigationThrottle>
      onion_location_navigation_throttle =
          tor::OnionLocationNavigationThrottle::MaybeCreateThrottleFor(
              handle, TorProfileServiceFactory::IsTorDisabled(),
              std::move(onion_location_navigation_throttle_delegate),
              context->IsTor());
  if (onion_location_navigation_throttle)
    throttles.push_back(std::move(onion_location_navigation_throttle));
#endif

#if BUILDFLAG(IPFS_ENABLED)
  std::unique_ptr<content::NavigationThrottle> ipfs_navigation_throttle =
      ipfs::IpfsNavigationThrottle::MaybeCreateThrottleFor(handle,
          ipfs::IpfsServiceFactory::GetForContext(context),
          g_brave_browser_process->GetApplicationLocale());
  if (ipfs_navigation_throttle)
    throttles.push_back(std::move(ipfs_navigation_throttle));
#endif

  return throttles;
}
