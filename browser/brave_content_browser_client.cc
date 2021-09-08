/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/rand_util.h"
#include "base/system/sys_info.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_browser_main_extra_parts.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/net/brave_proxying_url_loader_factory.h"
#include "brave/browser/net/brave_proxying_web_socket.h"
#include "brave/browser/profiles/brave_renderer_updater.h"
#include "brave/browser/profiles/brave_renderer_updater_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/binance/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/rewards_protocol_handler.h"
#include "brave/components/brave_search/browser/brave_search_default_host.h"
#include "brave/components/brave_search/browser/brave_search_default_host_private.h"
#include "brave/components/brave_search/browser/brave_search_fallback_host.h"
#include "brave/components/brave_search/common/brave_search_default.mojom.h"
#include "brave/components/brave_search/common/brave_search_fallback.mojom.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/domain_block_navigation_throttle.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/cosmetic_filters/browser/cosmetic_filters_resources.h"
#include "brave/components/cosmetic_filters/common/cosmetic_filters.mojom.h"
#include "brave/components/decentralized_dns/buildflags/buildflags.h"
#include "brave/components/ftx/browser/buildflags/buildflags.h"
#include "brave/components/gemini/browser/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_browser_interface_binders.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/common/url_constants.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/services/heap_profiling/public/mojom/heap_profiling_client.mojom.h"
#include "components/user_prefs/user_prefs.h"
#include "components/version_info/version_info.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "content/browser/service_worker/service_worker_host.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "extensions/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/cookies/site_for_cookies.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "third_party/blink/public/mojom/webpreferences/web_preferences.mojom.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/base/l10n/l10n_util.h"

using blink::web_pref::WebPreferences;
using brave_shields::BraveShieldsWebContentsObserver;
using brave_shields::ControlType;
using brave_shields::GetBraveShieldsEnabled;
using brave_shields::GetFingerprintingControlType;
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
#include "brave/browser/extensions/brave_webtorrent_navigation_throttle.h"
#include "brave/components/brave_webtorrent/browser/content_browser_client_helper.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/browser/ipfs/content_browser_client_helper.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_navigation_throttle.h"
#endif

#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
#include "brave/components/decentralized_dns/decentralized_dns_navigation_throttle.h"
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

#if BUILDFLAG(ENABLE_FTX)
#include "brave/browser/ftx/ftx_protocol_handler.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/brave_drm_tab_helper.h"
#endif

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/rpc_controller_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#if !defined(OS_ANDROID)
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_page_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#else
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_android.h"
#endif
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN) && !defined(OS_ANDROID)
#include "brave/browser/ui/webui/brave_vpn/vpn_panel_ui.h"
#include "brave/components/brave_vpn/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service_factory.h"
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

void BindCosmeticFiltersResources(
    content::RenderFrameHost* const frame_host,
    mojo::PendingReceiver<cosmetic_filters::mojom::CosmeticFiltersResources>
        receiver) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(frame_host);
  if (!web_contents)
    return;

  auto* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  auto* settings_map = HostContentSettingsMapFactory::GetForProfile(profile);

  mojo::MakeSelfOwnedReceiver(
      std::make_unique<cosmetic_filters::CosmeticFiltersResources>(
          settings_map, g_brave_browser_process->ad_block_service()),
      std::move(receiver));
}

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
void MaybeBindBraveWalletProvider(
    content::RenderFrameHost* const frame_host,
    mojo::PendingReceiver<brave_wallet::mojom::BraveWalletProvider> receiver) {
  auto rpc_controller = brave_wallet::RpcControllerFactory::GetForContext(
      frame_host->GetBrowserContext());

  if (!rpc_controller)
    return;

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host);
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<brave_wallet::BraveWalletProviderImpl>(
          std::move(rpc_controller),
#if defined(OS_ANDROID)
          std::make_unique<
              brave_wallet::BraveWalletProviderDelegateImplAndroid>(
#else
          std::make_unique<brave_wallet::BraveWalletProviderDelegateImpl>(
#endif
              web_contents, frame_host),
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())),
      std::move(receiver));
}
#endif

void BindBraveSearchFallbackHost(
    int process_id,
    mojo::PendingReceiver<brave_search::mojom::BraveSearchFallback> receiver) {
  content::RenderProcessHost* render_process_host =
      content::RenderProcessHost::FromID(process_id);
  if (!render_process_host)
    return;

  content::BrowserContext* context = render_process_host->GetBrowserContext();
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<brave_search::BraveSearchFallbackHost>(
          context->GetDefaultStoragePartition()
              ->GetURLLoaderFactoryForBrowserProcess()),
      std::move(receiver));
}

void BindBraveSearchDefaultHost(
    content::RenderFrameHost* const frame_host,
    mojo::PendingReceiver<brave_search::mojom::BraveSearchDefault> receiver) {
  auto* context = frame_host->GetBrowserContext();
  auto* profile = Profile::FromBrowserContext(context);
  if (brave::IsRegularProfile(profile)) {
    auto* template_url_service =
        TemplateURLServiceFactory::GetForProfile(profile);
    const std::string host = frame_host->GetLastCommittedURL().host();
    mojo::MakeSelfOwnedReceiver(
        std::make_unique<brave_search::BraveSearchDefaultHost>(
            host, template_url_service, profile->GetPrefs()),
        std::move(receiver));
  } else {
    // Dummy API which always returns false for private contexts.
    mojo::MakeSelfOwnedReceiver(
        std::make_unique<brave_search::BraveSearchDefaultHostPrivate>(),
        std::move(receiver));
  }
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
#if BUILDFLAG(ENABLE_IPFS)
  handler->AddHandlerPair(&ipfs::HandleIPFSURLRewrite,
                          &ipfs::HandleIPFSURLReverseRewrite);
#endif
  handler->AddHandlerPair(&HandleURLRewrite, &HandleURLReverseOverrideRewrite);
  ChromeContentBrowserClient::BrowserURLHandlerCreated(handler);
}

void BraveContentBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  Profile* profile = Profile::FromBrowserContext(host->GetBrowserContext());
  BraveRendererUpdaterFactory::GetForProfile(profile)->InitializeRenderer(host);

  ChromeContentBrowserClient::RenderProcessWillLaunch(host);
}

bool BraveContentBrowserClient::BindAssociatedReceiverFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedInterfaceEndpointHandle* handle) {
  if (ChromeContentBrowserClient::BindAssociatedReceiverFromFrame(
          render_frame_host, interface_name, handle)) {
    return true;
  }

#if BUILDFLAG(ENABLE_WIDEVINE)
  if (interface_name == brave_drm::mojom::BraveDRM::Name_) {
    BraveDrmTabHelper::BindBraveDRM(
        mojo::PendingAssociatedReceiver<brave_drm::mojom::BraveDRM>(
            std::move(*handle)),
        render_frame_host);
    return true;
  }
#endif  // BUILDFLAG(ENABLE_WIDEVINE)

  if (interface_name == brave_shields::mojom::BraveShieldsHost::Name_) {
    brave_shields::BraveShieldsWebContentsObserver::BindBraveShieldsHost(
        mojo::PendingAssociatedReceiver<brave_shields::mojom::BraveShieldsHost>(
            std::move(*handle)),
        render_frame_host);
    return true;
  }

  return false;
}

content::ContentBrowserClient::AllowWebBluetoothResult
BraveContentBrowserClient::AllowWebBluetooth(
    content::BrowserContext* browser_context,
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin) {
  return ContentBrowserClient::AllowWebBluetoothResult::BLOCK_GLOBALLY_DISABLED;
}

void BraveContentBrowserClient::ExposeInterfacesToRenderer(
    service_manager::BinderRegistry* registry,
    blink::AssociatedInterfaceRegistry* associated_registry,
    content::RenderProcessHost* render_process_host) {
  ChromeContentBrowserClient::ExposeInterfacesToRenderer(
      registry, associated_registry, render_process_host);
  registry->AddInterface(base::BindRepeating(&BindBraveSearchFallbackHost,
                                             render_process_host->GetID()),
                         content::GetUIThreadTaskRunner({}));
}

void BraveContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
    content::RenderFrameHost* render_frame_host,
    mojo::BinderMapWithContext<content::RenderFrameHost*>* map) {
  ChromeContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
      render_frame_host, map);
  map->Add<cosmetic_filters::mojom::CosmeticFiltersResources>(
      base::BindRepeating(&BindCosmeticFiltersResources));
  if (brave_search::IsDefaultAPIEnabled()) {
    map->Add<brave_search::mojom::BraveSearchDefault>(
        base::BindRepeating(&BindBraveSearchDefaultHost));
  }
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  if (brave_wallet::IsNativeWalletEnabled()) {
    map->Add<brave_wallet::mojom::BraveWalletProvider>(
        base::BindRepeating(&MaybeBindBraveWalletProvider));
  }
#if !defined(OS_ANDROID)
  chrome::internal::RegisterWebUIControllerInterfaceBinder<
      brave_wallet::mojom::PanelHandlerFactory, WalletPanelUI>(map);
  chrome::internal::RegisterWebUIControllerInterfaceBinder<
      brave_wallet::mojom::PageHandlerFactory, WalletPageUI>(map);
#endif
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN) && !defined(OS_ANDROID)
  if (brave_vpn::IsBraveVPNEnabled()) {
    chrome::internal::RegisterWebUIControllerInterfaceBinder<
        brave_vpn::mojom::PanelHandlerFactory, VPNPanelUI>(map);
  }
#endif
}

bool BraveContentBrowserClient::HandleExternalProtocol(
    const GURL& url,
    content::WebContents::Getter web_contents_getter,
    int child_id,
    int frame_tree_node_id,
    content::NavigationUIData* navigation_data,
    bool is_main_frame,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const absl::optional<url::Origin>& initiating_origin,
    mojo::PendingRemote<network::mojom::URLLoaderFactory>* out_factory) {
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  if (webtorrent::IsMagnetProtocol(url)) {
    webtorrent::HandleMagnetProtocol(url, web_contents_getter, page_transition,
                                     has_user_gesture, initiating_origin);
    return true;
  }
#endif

  if (brave_rewards::IsRewardsProtocol(url)) {
    brave_rewards::HandleRewardsProtocol(url, web_contents_getter,
                                         page_transition, has_user_gesture);
    return true;
  }

#if BUILDFLAG(BINANCE_ENABLED)
  if (binance::IsBinanceProtocol(url)) {
    binance::HandleBinanceProtocol(url, web_contents_getter, page_transition,
                                   has_user_gesture, initiating_origin);
    return true;
  }
#endif

#if BUILDFLAG(GEMINI_ENABLED)
  if (gemini::IsGeminiProtocol(url)) {
    gemini::HandleGeminiProtocol(url, web_contents_getter, page_transition,
                                 has_user_gesture, initiating_origin);
    return true;
  }
#endif

#if BUILDFLAG(ENABLE_FTX)
  if (ftx::IsFTXProtocol(url)) {
    ftx::HandleFTXProtocol(url, web_contents_getter, page_transition,
                           has_user_gesture, initiating_origin);
    return true;
  }
#endif

  return ChromeContentBrowserClient::HandleExternalProtocol(
      url, web_contents_getter, child_id, frame_tree_node_id, navigation_data,
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

    // Command line parameters from the browser process are propagated to the
    // renderers *after* ContentBrowserClient::AppendExtraCommandLineSwitches()
    // is called from RenderProcessHostImpl::AppendRendererCommandLine(). This
    // means we have to inspect the main browser process' parameters for the
    // |switches::kTestType| as it will be too soon to find it on command_line.
    const base::CommandLine& browser_command_line =
        *base::CommandLine::ForCurrentProcess();
    if (!browser_command_line.HasSwitch(switches::kTestType)) {
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
    const content::WebContents::Getter& wc_getter,
    content::NavigationUIData* navigation_ui_data,
    int frame_tree_node_id) {
  auto result = ChromeContentBrowserClient::CreateURLLoaderThrottles(
      request, browser_context, wc_getter, navigation_ui_data,
      frame_tree_node_id);
#if BUILDFLAG(ENABLE_SPEEDREADER)
  using DistillState = speedreader::DistillState;
  content::WebContents* contents = wc_getter.Run();
  if (!contents) {
    return result;
  }
  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents);
  if (tab_helper) {
    const auto state = tab_helper->PageDistillState();
    if (speedreader::PageWantsDistill(state) &&
        request.resource_type ==
            static_cast<int>(blink::mojom::ResourceType::kMainFrame)) {
      // Only check for disabled sites if we are in Speedreader mode
      const bool check_disabled_sites =
          state == DistillState::kSpeedreaderModePending;
      std::unique_ptr<speedreader::SpeedReaderThrottle> throttle =
          speedreader::SpeedReaderThrottle::MaybeCreateThrottleFor(
              g_brave_browser_process->speedreader_rewriter_service(),
              HostContentSettingsMapFactory::GetForProfile(
                  Profile::FromBrowserContext(browser_context)),
              tab_helper->GetWeakPtr(), request.url, check_disabled_sites,
              base::ThreadTaskRunnerHandle::Get());
      if (throttle)
        result.push_back(std::move(throttle));
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
    absl::optional<int64_t> navigation_id,
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
    const absl::optional<std::string>& user_agent,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client) {
  auto* proxy = BraveProxyingWebSocket::ProxyWebSocket(
      frame, std::move(factory), url, site_for_cookies, user_agent,
      std::move(handshake_client));

  if (ChromeContentBrowserClient::WillInterceptWebSocket(frame)) {
    ChromeContentBrowserClient::CreateWebSocket(
        frame, proxy->web_socket_factory(), url, site_for_cookies, user_agent,
        proxy->handshake_client().Unbind());
  } else {
    proxy->Start();
  }
}

void BraveContentBrowserClient::MaybeHideReferrer(
    content::BrowserContext* browser_context,
    const GURL& request_url,
    const GURL& document_url,
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
      HostContentSettingsMapFactory::GetForProfile(profile), document_url);
  const bool shields_up = brave_shields::GetBraveShieldsEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile), document_url);

  content::Referrer new_referrer;
  if (brave_shields::MaybeChangeReferrer(allow_referrers, shields_up,
                                         (*referrer)->url, request_url,
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
bool BraveContentBrowserClient::HandleURLOverrideRewrite(
    GURL* url,
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

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED) && BUILDFLAG(ENABLE_EXTENSIONS)
  // If the Crypto Wallets extension is loaded, then it replaces the WebUI
  auto* service =
      EthereumRemoteClientServiceFactory::GetForContext(browser_context);
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

  content::BrowserContext* context =
      handle->GetWebContents()->GetBrowserContext();

#if BUILDFLAG(ENABLE_TOR)
  std::unique_ptr<content::NavigationThrottle> tor_navigation_throttle =
      tor::TorNavigationThrottle::MaybeCreateThrottleFor(handle,
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

#if BUILDFLAG(ENABLE_IPFS)
  std::unique_ptr<content::NavigationThrottle> ipfs_navigation_throttle =
      ipfs::IpfsNavigationThrottle::MaybeCreateThrottleFor(
          handle, ipfs::IpfsServiceFactory::GetForContext(context),
          user_prefs::UserPrefs::Get(context),
          g_browser_process->GetApplicationLocale());
  if (ipfs_navigation_throttle)
    throttles.push_back(std::move(ipfs_navigation_throttle));
#endif

#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
  std::unique_ptr<content::NavigationThrottle>
      decentralized_dns_navigation_throttle =
          decentralized_dns::DecentralizedDnsNavigationThrottle::
              MaybeCreateThrottleFor(handle, g_browser_process->local_state(),
                                     g_browser_process->GetApplicationLocale());
  if (decentralized_dns_navigation_throttle)
    throttles.push_back(std::move(decentralized_dns_navigation_throttle));
#endif

  if (std::unique_ptr<
          content::NavigationThrottle> domain_block_navigation_throttle =
          brave_shields::DomainBlockNavigationThrottle::MaybeCreateThrottleFor(
              handle, g_brave_browser_process->ad_block_service(),
              g_brave_browser_process->ad_block_custom_filters_service(),
              HostContentSettingsMapFactory::GetForProfile(
                  Profile::FromBrowserContext(context)),
              g_browser_process->GetApplicationLocale()))
    throttles.push_back(std::move(domain_block_navigation_throttle));

  return throttles;
}

bool BraveContentBrowserClient::OverrideWebPreferencesAfterNavigation(
    WebContents* web_contents,
    WebPreferences* prefs) {
  bool changed =
      ChromeContentBrowserClient::OverrideWebPreferencesAfterNavigation(
          web_contents, prefs);
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  const GURL url = web_contents->GetLastCommittedURL();
  const bool shields_up = brave_shields::GetBraveShieldsEnabled(
      HostContentSettingsMapFactory::GetForProfile(profile), url);
  auto fingerprinting_type = brave_shields::GetFingerprintingControlType(
      HostContentSettingsMapFactory::GetForProfile(profile), url);
  // https://github.com/brave/brave-browser/issues/15265
  // Always use color scheme Light if fingerprinting mode strict
  if (base::FeatureList::IsEnabled(
          brave_shields::features::kBraveDarkModeBlock) &&
      shields_up && fingerprinting_type == ControlType::BLOCK) {
    prefs->preferred_color_scheme = blink::mojom::PreferredColorScheme::kLight;
    changed = true;
  }
  return changed;
}
