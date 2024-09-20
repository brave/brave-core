/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/system/sys_info.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/brave_browser_main_extra_parts.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_shields/brave_farbling_service_factory.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/cosmetic_filters/cosmetic_filters_tab_helper.h"
#include "brave/browser/debounce/debounce_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/net/brave_proxying_url_loader_factory.h"
#include "brave/browser/net/brave_proxying_web_socket.h"
#include "brave/browser/profiles/brave_renderer_updater.h"
#include "brave/browser/profiles/brave_renderer_updater_factory.h"
#include "brave/browser/skus/skus_service_factory.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_page_ui.h"
#include "brave/browser/ui/webui/skus_internals_ui.h"
#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_rewriter/common/buildflags/buildflags.h"
#include "brave/components/body_sniffer/body_sniffer_throttle.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_rewards/browser/rewards_protocol_navigation_throttle.h"
#include "brave/components/brave_search/browser/brave_search_default_host.h"
#include "brave/components/brave_search/browser/brave_search_default_host_private.h"
#include "brave/components/brave_search/browser/brave_search_fallback_host.h"
#include "brave/components/brave_search/common/brave_search_default.mojom.h"
#include "brave/components/brave_search/common/brave_search_fallback.mojom.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/content/browser/domain_block_navigation_throttle.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/brave_wallet_p3a_private.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/ethereum_provider_impl.h"
#include "brave/components/brave_wallet/browser/solana_provider_impl.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/cosmetic_filters/browser/cosmetic_filters_resources.h"
#include "brave/components/cosmetic_filters/common/cosmetic_filters.mojom.h"
#include "brave/components/de_amp/browser/de_amp_body_handler.h"
#include "brave/components/debounce/content/browser/debounce_navigation_throttle.h"
#include "brave/components/decentralized_dns/content/decentralized_dns_navigation_throttle.h"
#include "brave/components/google_sign_in_permission/google_sign_in_permission_throttle.h"
#include "brave/components/google_sign_in_permission/google_sign_in_permission_util.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/skus/common/skus_internals.mojom.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/translate/core/common/brave_translate_switches.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_browser_interface_binders.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/branded_strings.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/embedder_support/switches.h"
#include "components/prefs/pref_service.h"
#include "components/services/heap_profiling/public/mojom/heap_profiling_client.mojom.h"
#include "components/user_prefs/user_prefs.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/weak_document_ptr.h"
#include "content/public/browser/web_ui_browser_interface_broker_registry.h"
#include "content/public/browser/web_ui_controller_interface_binder.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "extensions/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/cookies/site_for_cookies.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "third_party/blink/public/mojom/webpreferences/web_preferences.mojom.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_REQUEST_OTR)
#include "brave/browser/request_otr/request_otr_service_factory.h"
#include "brave/components/request_otr/browser/request_otr_navigation_throttle.h"
#include "brave/components/request_otr/browser/request_otr_storage_tab_helper.h"
#endif

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
#include "extensions/common/constants.h"

using extensions::ChromeContentBrowserClientExtensionsPart;
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/content/browser/ai_chat_throttle.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/settings_helper.mojom.h"
#if BUILDFLAG(IS_ANDROID)
#include "brave/components/ai_chat/core/browser/android/ai_chat_iap_subscription_android.h"
#endif
#endif

#if BUILDFLAG(ENABLE_AI_REWRITER)
#include "brave/browser/ui/webui/ai_rewriter/ai_rewriter_ui.h"
#include "brave/components/ai_rewriter/common/features.h"
#include "brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/browser/extensions/brave_webtorrent_navigation_throttle.h"
#include "brave/components/brave_webtorrent/browser/magnet_protocol_handler.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/onion_location_navigation_throttle.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/components/speedreader/speedreader_body_distiller.h"
#include "brave/components/speedreader/speedreader_distilled_page_producer.h"
#include "brave/components/speedreader/speedreader_util.h"
#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/webui/speedreader/speedreader_toolbar_ui.h"
#include "brave/components/speedreader/common/speedreader_toolbar.mojom.h"
#endif
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/brave_drm_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/ui/webui/brave_vpn/vpn_panel_ui.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service_factory.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/webui/brave_wallet/android/android_wallet_page_ui.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/new_tab/new_tab_shows_navigation_throttle.h"
#include "brave/browser/ui/geolocation/brave_geolocation_permission_tab_helper.h"
#include "brave/browser/ui/webui/brave_news_internals/brave_news_internals_ui.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_page_top_ui.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_panel_ui.h"
#include "brave/browser/ui/webui/brave_rewards/tip_panel_ui.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/browser/ui/webui/brave_shields/cookie_list_opt_in_ui.h"
#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_page_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"
#include "brave/browser/ui/webui/private_new_tab_page/brave_private_new_tab_ui.h"
#include "brave/components/brave_new_tab_ui/brave_new_tab_page.mojom.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_private_new_tab_ui/common/brave_private_new_tab.mojom.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/mojom/rewards_panel.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_tip_panel.mojom.h"
#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"
#include "brave/components/brave_shields/core/common/cookie_list_opt_in.mojom.h"
#include "brave/components/commands/common/commands.mojom.h"
#include "brave/components/commands/common/features.h"
#include "ui/webui/resources/cr_components/searchbox/searchbox.mojom.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/browser/playlist_background_web_contents_helper.h"
#include "brave/components/playlist/browser/playlist_media_handler.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/browser/ui/webui/playlist_ui.h"
#endif  // BUILDFLAG(ENABLE_PLAYLIST_WEBUI)

namespace {

bool HandleURLReverseOverrideRewrite(GURL* url,
                                     content::BrowserContext* browser_context) {
  if (BraveContentBrowserClient::HandleURLOverrideRewrite(url,
                                                          browser_context)) {
    return true;
  }

// For wallet pages, return true to update the displayed URL to react-routed
// URL rather than showing brave://wallet for everything. This is needed
// because of a side effect from rewriting brave:// to chrome:// in
// HandleURLRewrite handler which makes brave://wallet the virtual URL here
// unless we return true to trigger an update of virtual URL here to the routed
// URL. For example, we will display brave://wallet/send instead of
// brave://wallet with this. This is Android only because currently both
// virtual and real URLs are chrome:// on desktop, so it doesn't have this
// issue.
#if BUILDFLAG(IS_ANDROID)
  if ((url->SchemeIs(content::kBraveUIScheme) ||
       url->SchemeIs(content::kChromeUIScheme)) &&
      url->host() == kWalletPageHost) {
    if (url->SchemeIs(content::kChromeUIScheme)) {
      GURL::Replacements replacements;
      replacements.SetSchemeStr(content::kBraveUIScheme);
      *url = url->ReplaceComponents(replacements);
    }
    return true;
  }
#endif

  return false;
}

bool HandleURLRewrite(GURL* url, content::BrowserContext* browser_context) {
  if (BraveContentBrowserClient::HandleURLOverrideRewrite(url,
                                                          browser_context)) {
    return true;
  }

// For wallet pages, return true so we can handle it in the reverse handler.
// Also update the real URL from brave:// to chrome://.
#if BUILDFLAG(IS_ANDROID)
  if ((url->SchemeIs(content::kBraveUIScheme) ||
       url->SchemeIs(content::kChromeUIScheme)) &&
      url->host() == kWalletPageHost) {
    if (url->SchemeIs(content::kBraveUIScheme)) {
      GURL::Replacements replacements;
      replacements.SetSchemeStr(content::kChromeUIScheme);
      *url = url->ReplaceComponents(replacements);
    }
    return true;
  }
#endif

  return false;
}

void BindCosmeticFiltersResourcesOnTaskRunner(
    mojo::PendingReceiver<cosmetic_filters::mojom::CosmeticFiltersResources>
        receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<cosmetic_filters::CosmeticFiltersResources>(
          g_brave_browser_process->ad_block_service()),
      std::move(receiver));
}

void BindCosmeticFiltersResources(
    content::RenderFrameHost* const frame_host,
    mojo::PendingReceiver<cosmetic_filters::mojom::CosmeticFiltersResources>
        receiver) {
  g_brave_browser_process->ad_block_service()->GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&BindCosmeticFiltersResourcesOnTaskRunner,
                                std::move(receiver)));
}

void MaybeBindWalletP3A(
    content::RenderFrameHost* const frame_host,
    mojo::PendingReceiver<brave_wallet::mojom::BraveWalletP3A> receiver) {
  auto* context = frame_host->GetBrowserContext();
  if (brave_wallet::IsAllowedForContext(frame_host->GetBrowserContext())) {
    brave_wallet::BraveWalletService* wallet_service =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(context);
    DCHECK(wallet_service);
    wallet_service->GetBraveWalletP3A()->Bind(std::move(receiver));
  } else {
    // Dummy API to avoid reporting P3A for OTR contexts
    mojo::MakeSelfOwnedReceiver(
        std::make_unique<brave_wallet::BraveWalletP3APrivate>(),
        std::move(receiver));
  }
}

void MaybeBindEthereumProvider(
    content::RenderFrameHost* const frame_host,
    mojo::PendingReceiver<brave_wallet::mojom::EthereumProvider> receiver) {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
          frame_host->GetBrowserContext());
  if (!brave_wallet_service) {
    return;
  }

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host);
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<brave_wallet::EthereumProviderImpl>(
          HostContentSettingsMapFactory::GetForProfile(
              Profile::FromBrowserContext(frame_host->GetBrowserContext())),
          brave_wallet_service,
          std::make_unique<brave_wallet::BraveWalletProviderDelegateImpl>(
              web_contents, frame_host),
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())),
      std::move(receiver));
}

void MaybeBindSolanaProvider(
    content::RenderFrameHost* const frame_host,
    mojo::PendingReceiver<brave_wallet::mojom::SolanaProvider> receiver) {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
          frame_host->GetBrowserContext());
  if (!brave_wallet_service) {
    return;
  }

  auto* json_rpc_service = brave_wallet_service->json_rpc_service();
  CHECK(json_rpc_service);

  auto* keyring_service = brave_wallet_service->keyring_service();
  CHECK(keyring_service);

  auto* tx_service = brave_wallet_service->tx_service();
  CHECK(tx_service);

  auto* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(
          frame_host->GetBrowserContext());
  if (!host_content_settings_map) {
    return;
  }

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host);
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<brave_wallet::SolanaProviderImpl>(
          *host_content_settings_map, brave_wallet_service,
          std::make_unique<brave_wallet::BraveWalletProviderDelegateImpl>(
              web_contents, frame_host)),
      std::move(receiver));
}

void BindBraveSearchFallbackHost(
    int process_id,
    mojo::PendingReceiver<brave_search::mojom::BraveSearchFallback> receiver) {
  content::RenderProcessHost* render_process_host =
      content::RenderProcessHost::FromID(process_id);
  if (!render_process_host) {
    return;
  }

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
  if (profile->IsRegularProfile()) {
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

#if BUILDFLAG(IS_ANDROID)
void BindIAPSubscription(
    content::RenderFrameHost* const frame_host,
    mojo::PendingReceiver<ai_chat::mojom::IAPSubscription> receiver) {
  auto* context = frame_host->GetBrowserContext();
  auto* profile = Profile::FromBrowserContext(context);
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<ai_chat::AIChatIAPSubscription>(profile->GetPrefs()),
      std::move(receiver));
}
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
void MaybeBindBraveVpnImpl(
    content::RenderFrameHost* const frame_host,
    mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver) {
  auto* context = frame_host->GetBrowserContext();
  brave_vpn::BraveVpnServiceFactory::BindForContext(context,
                                                    std::move(receiver));
}
#endif
void MaybeBindSkusSdkImpl(
    content::RenderFrameHost* const frame_host,
    mojo::PendingReceiver<skus::mojom::SkusService> receiver) {
  auto* context = frame_host->GetBrowserContext();
  skus::SkusServiceFactory::BindForContext(context, std::move(receiver));
}

}  // namespace

BraveContentBrowserClient::BraveContentBrowserClient() = default;

BraveContentBrowserClient::~BraveContentBrowserClient() = default;

std::unique_ptr<content::BrowserMainParts>
BraveContentBrowserClient::CreateBrowserMainParts(bool is_integration_test) {
  std::unique_ptr<content::BrowserMainParts> main_parts =
      ChromeContentBrowserClient::CreateBrowserMainParts(is_integration_test);
  ChromeBrowserMainParts* chrome_main_parts =
      static_cast<ChromeBrowserMainParts*>(main_parts.get());
  chrome_main_parts->AddParts(std::make_unique<BraveBrowserMainExtraParts>());
  return main_parts;
}

bool BraveContentBrowserClient::AreIsolatedWebAppsEnabled(
    content::BrowserContext* browser_context) {
  return false;
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

void BraveContentBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  Profile* profile = Profile::FromBrowserContext(host->GetBrowserContext());
  // The BraveRendererUpdater might be null for some irregular profiles, e.g.
  // the System Profile.
  if (BraveRendererUpdater* service =
          BraveRendererUpdaterFactory::GetForProfile(profile)) {
    service->InitializeRenderer(host);
  }

  ChromeContentBrowserClient::RenderProcessWillLaunch(host);
}

void BraveContentBrowserClient::
    RegisterAssociatedInterfaceBindersForRenderFrameHost(
        content::RenderFrameHost& render_frame_host,                // NOLINT
        blink::AssociatedInterfaceRegistry& associated_registry) {  // NOLINT
#if BUILDFLAG(ENABLE_WIDEVINE)
  associated_registry.AddInterface<
      brave_drm::mojom::BraveDRM>(base::BindRepeating(
      [](content::RenderFrameHost* render_frame_host,
         mojo::PendingAssociatedReceiver<brave_drm::mojom::BraveDRM> receiver) {
        BraveDrmTabHelper::BindBraveDRM(std::move(receiver), render_frame_host);
      },
      &render_frame_host));
#endif  // BUILDFLAG(ENABLE_WIDEVINE)

#if !BUILDFLAG(IS_ANDROID)
  associated_registry.AddInterface<
      geolocation::mojom::BraveGeolocationPermission>(base::BindRepeating(
      [](content::RenderFrameHost* render_frame_host,
         mojo::PendingAssociatedReceiver<
             geolocation::mojom::BraveGeolocationPermission> receiver) {
        BraveGeolocationPermissionTabHelper::BindBraveGeolocationPermission(
            std::move(receiver), render_frame_host);
      },
      &render_frame_host));
#endif  // !BUILDFLAG(IS_ANDROID)

  associated_registry.AddInterface<
      brave_shields::mojom::BraveShieldsHost>(base::BindRepeating(
      [](content::RenderFrameHost* render_frame_host,
         mojo::PendingAssociatedReceiver<brave_shields::mojom::BraveShieldsHost>
             receiver) {
        brave_shields::BraveShieldsWebContentsObserver::BindBraveShieldsHost(
            std::move(receiver), render_frame_host);
      },
      &render_frame_host));

#if BUILDFLAG(ENABLE_SPEEDREADER)
  associated_registry.AddInterface<speedreader::mojom::SpeedreaderHost>(
      base::BindRepeating(
          [](content::RenderFrameHost* render_frame_host,
             mojo::PendingAssociatedReceiver<
                 speedreader::mojom::SpeedreaderHost> receiver) {
            speedreader::SpeedreaderTabHelper::BindSpeedreaderHost(
                std::move(receiver), render_frame_host);
          },
          &render_frame_host));
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
  // AI Chat page content extraction renderer -> browser interface
  associated_registry.AddInterface<ai_chat::mojom::PageContentExtractorHost>(
      base::BindRepeating(
          [](content::RenderFrameHost* render_frame_host,
             mojo::PendingAssociatedReceiver<
                 ai_chat::mojom::PageContentExtractorHost> receiver) {
            ai_chat::AIChatTabHelper::BindPageContentExtractorHost(
                render_frame_host, std::move(receiver));
          },
          &render_frame_host));
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
  associated_registry.AddInterface<playlist::mojom::PlaylistMediaResponder>(
      base::BindRepeating(
          &playlist::PlaylistMediaHandler::BindMediaResponderReceiver,
          &render_frame_host));
#endif  // BUILDFLAG(ENABLE_PLAYLIST)

  associated_registry.AddInterface<
      cosmetic_filters::mojom::CosmeticFiltersHandler>(base::BindRepeating(
      &cosmetic_filters::CosmeticFiltersTabHelper::BindCosmeticFiltersHandler,
      &render_frame_host));

  ChromeContentBrowserClient::
      RegisterAssociatedInterfaceBindersForRenderFrameHost(render_frame_host,
                                                           associated_registry);
}

void BraveContentBrowserClient::RegisterWebUIInterfaceBrokers(
    content::WebUIBrowserInterfaceBrokerRegistry& registry) {
  ChromeContentBrowserClient::RegisterWebUIInterfaceBrokers(registry);
#if BUILDFLAG(ENABLE_BRAVE_VPN) && !BUILDFLAG(IS_ANDROID)
  if (brave_vpn::IsBraveVPNFeatureEnabled()) {
    registry.ForWebUI<VPNPanelUI>()
        .Add<brave_vpn::mojom::PanelHandlerFactory>();
  }
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    registry.ForWebUI<playlist::PlaylistUI>()
        .Add<playlist::mojom::PageHandlerFactory>();
  }
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (ai_chat::features::IsAIChatEnabled()) {
    registry.ForWebUI<AIChatUI>()
        .Add<ai_chat::mojom::AIChatUIHandler>()
        .Add<ai_chat::mojom::Service>();
  }
#endif

#if BUILDFLAG(ENABLE_AI_REWRITER)
  if (ai_rewriter::features::IsAIRewriterEnabled()) {
    registry.ForWebUI<ai_rewriter::AIRewriterUI>()
        .Add<ai_rewriter::mojom::AIRewriterPageHandler>();
  }
#endif

  if (base::FeatureList::IsEnabled(skus::features::kSkusFeature)) {
    registry.ForWebUI<SkusInternalsUI>().Add<skus::mojom::SkusInternals>();
  }

  registry.ForWebUI<brave_rewards::RewardsPageUI>()
      .Add<brave_rewards::mojom::RewardsPageHandlerFactory>();

#if !BUILDFLAG(IS_ANDROID)
  auto ntp_registration =
      registry.ForWebUI<BraveNewTabUI>()
          .Add<brave_new_tab_page::mojom::PageHandlerFactory>()
          .Add<brave_news::mojom::BraveNewsController>();

  if (base::FeatureList::IsEnabled(features::kBraveNtpSearchWidget)) {
    ntp_registration.Add<searchbox::mojom::PageHandler>();
  }

  if (base::FeatureList::IsEnabled(
          brave_news::features::kBraveNewsFeedUpdate)) {
    registry.ForWebUI<BraveNewsInternalsUI>()
        .Add<brave_news::mojom::BraveNewsController>()
        .Add<brave_news::mojom::BraveNewsInternals>();
  }
#endif
}

std::optional<base::UnguessableToken>
BraveContentBrowserClient::GetEphemeralStorageToken(
    content::RenderFrameHost* render_frame_host,
    const url::Origin& origin) {
  DCHECK(render_frame_host);
  auto* wc = content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!wc) {
    return std::nullopt;
  }

  auto* es_tab_helper =
      ephemeral_storage::EphemeralStorageTabHelper::FromWebContents(wc);
  if (!es_tab_helper) {
    return std::nullopt;
  }

  return es_tab_helper->GetEphemeralStorageToken(origin);
}

bool BraveContentBrowserClient::AllowWorkerFingerprinting(
    const GURL& url,
    content::BrowserContext* browser_context) {
  return WorkerGetBraveShieldSettings(url, browser_context)->farbling_level !=
         brave_shields::mojom::FarblingLevel::MAXIMUM;
}

brave_shields::mojom::ShieldsSettingsPtr
BraveContentBrowserClient::WorkerGetBraveShieldSettings(
    const GURL& url,
    content::BrowserContext* browser_context) {
  const brave_shields::mojom::FarblingLevel farbling_level =
      brave_shields::GetFarblingLevel(
          HostContentSettingsMapFactory::GetForProfile(browser_context), url);

  PrefService* pref_service = user_prefs::UserPrefs::Get(browser_context);

  return brave_shields::mojom::ShieldsSettings::New(
      farbling_level, std::vector<std::string>(),
      brave_shields::IsReduceLanguageEnabledForProfile(pref_service));
}

content::ContentBrowserClient::AllowWebBluetoothResult
BraveContentBrowserClient::AllowWebBluetooth(
    content::BrowserContext* browser_context,
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin) {
  if (!base::FeatureList::IsEnabled(blink::features::kBraveWebBluetoothAPI)) {
    return ContentBrowserClient::AllowWebBluetoothResult::
        BLOCK_GLOBALLY_DISABLED;
  }
  return ChromeContentBrowserClient::AllowWebBluetooth(
      browser_context, requesting_origin, embedding_origin);
}

bool BraveContentBrowserClient::CanCreateWindow(
    content::RenderFrameHost* opener,
    const GURL& opener_url,
    const GURL& opener_top_level_frame_url,
    const url::Origin& source_origin,
    content::mojom::WindowContainerType container_type,
    const GURL& target_url,
    const content::Referrer& referrer,
    const std::string& frame_name,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& features,
    bool user_gesture,
    bool opener_suppressed,
    bool* no_javascript_access) {
  // Check base implementation first
  bool can_create_window = ChromeContentBrowserClient::CanCreateWindow(
      opener, opener_url, opener_top_level_frame_url, source_origin,
      container_type, target_url, referrer, frame_name, disposition, features,
      user_gesture, opener_suppressed, no_javascript_access);

#if BUILDFLAG(ENABLE_REQUEST_OTR)
  // If the user has requested going off-the-record in this tab, don't allow
  // opening new windows via script
  if (content::WebContents* web_contents =
          content::WebContents::FromRenderFrameHost(opener)) {
    if (request_otr::
            RequestOTRStorageTabHelper* request_otr_storage_tab_helper =
                request_otr::RequestOTRStorageTabHelper::FromWebContents(
                    web_contents)) {
      if (request_otr_storage_tab_helper->has_requested_otr()) {
        *no_javascript_access = true;
      }
    }
  }
#endif

  return can_create_window && google_sign_in_permission::CanCreateWindow(
                                  opener, opener_url, target_url);
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

  map->Add<brave_wallet::mojom::BraveWalletP3A>(
      base::BindRepeating(&MaybeBindWalletP3A));
  if (brave_wallet::IsAllowedForContext(
          render_frame_host->GetBrowserContext())) {
    if (brave_wallet::IsNativeWalletEnabled()) {
      map->Add<brave_wallet::mojom::EthereumProvider>(
          base::BindRepeating(&MaybeBindEthereumProvider));
      map->Add<brave_wallet::mojom::SolanaProvider>(
          base::BindRepeating(&MaybeBindSolanaProvider));
    }
  }

  map->Add<skus::mojom::SkusService>(
      base::BindRepeating(&MaybeBindSkusSdkImpl));
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  map->Add<brave_vpn::mojom::ServiceHandler>(
      base::BindRepeating(&MaybeBindBraveVpnImpl));
#endif

#if BUILDFLAG(IS_ANDROID)
  content::RegisterWebUIControllerInterfaceBinder<
      brave_wallet::mojom::PageHandlerFactory, AndroidWalletPageUI>(map);
#endif

#if !BUILDFLAG(IS_ANDROID)
  content::RegisterWebUIControllerInterfaceBinder<
      brave_wallet::mojom::PageHandlerFactory, WalletPageUI>(map);
  content::RegisterWebUIControllerInterfaceBinder<
      brave_wallet::mojom::PanelHandlerFactory, WalletPanelUI>(map);
  content::RegisterWebUIControllerInterfaceBinder<
      brave_private_new_tab::mojom::PageHandler, BravePrivateNewTabUI>(map);
  content::RegisterWebUIControllerInterfaceBinder<
      brave_shields::mojom::PanelHandlerFactory, ShieldsPanelUI>(map);
  if (base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockCookieListOptIn)) {
    content::RegisterWebUIControllerInterfaceBinder<
        brave_shields::mojom::CookieListOptInPageHandlerFactory,
        CookieListOptInUI>(map);
  }
  content::RegisterWebUIControllerInterfaceBinder<
      brave_rewards::mojom::RewardsPageHandlerFactory,
      brave_rewards::RewardsPageTopUI>(map);
  content::RegisterWebUIControllerInterfaceBinder<
      brave_rewards::mojom::PanelHandlerFactory, brave_rewards::RewardsPanelUI>(
      map);
  content::RegisterWebUIControllerInterfaceBinder<
      brave_rewards::mojom::TipPanelHandlerFactory, brave_rewards::TipPanelUI>(
      map);
  if (base::FeatureList::IsEnabled(commands::features::kBraveCommands)) {
    content::RegisterWebUIControllerInterfaceBinder<
        commands::mojom::CommandsService, BraveSettingsUI>(map);
  }
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
  auto* prefs =
      user_prefs::UserPrefs::Get(render_frame_host->GetBrowserContext());
  if (ai_chat::IsAIChatEnabled(prefs) &&
      Profile::FromBrowserContext(render_frame_host->GetBrowserContext())
          ->IsRegularProfile()) {
    // WebUI -> Browser interface
    content::RegisterWebUIControllerInterfaceBinder<
        ai_chat::mojom::AIChatUIHandler, AIChatUI>(map);
#if !BUILDFLAG(IS_ANDROID)
    content::RegisterWebUIControllerInterfaceBinder<
        ai_chat::mojom::AIChatSettingsHelper, BraveSettingsUI>(map);
#endif
  }
#if BUILDFLAG(IS_ANDROID)
  if (ai_chat::IsAIChatEnabled(prefs)) {
    map->Add<ai_chat::mojom::IAPSubscription>(
        base::BindRepeating(&BindIAPSubscription));
  }
#endif
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER) && !BUILDFLAG(IS_ANDROID)
  content::RegisterWebUIControllerInterfaceBinder<
      speedreader::mojom::ToolbarFactory, SpeedreaderToolbarUI>(map);
#endif
}

bool BraveContentBrowserClient::HandleExternalProtocol(
    const GURL& url,
    content::WebContents::Getter web_contents_getter,
    int frame_tree_node_id,
    content::NavigationUIData* navigation_data,
    bool is_primary_main_frame,
    bool is_in_fenced_frame_tree,
    network::mojom::WebSandboxFlags sandbox_flags,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const std::optional<url::Origin>& initiating_origin,
    content::RenderFrameHost* initiator_document,
    mojo::PendingRemote<network::mojom::URLLoaderFactory>* out_factory) {
  return ChromeContentBrowserClient::HandleExternalProtocol(
      url, web_contents_getter, frame_tree_node_id, navigation_data,
      is_primary_main_frame, is_in_fenced_frame_tree, sandbox_flags,
      page_transition, has_user_gesture, initiating_origin, initiator_document,
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
      if (profile) {
        auto* brave_farbling_service =
            brave::BraveFarblingServiceFactory::GetForProfile(profile);
        if (brave_farbling_service) {
          session_token = brave_farbling_service->session_token();
        }
      }
      if (command_line->HasSwitch(switches::kEnableIsolatedWebAppsInRenderer)) {
        command_line->RemoveSwitch(switches::kEnableIsolatedWebAppsInRenderer);
      }
    }

    command_line->AppendSwitchASCII("brave_session_token",
                                    base::NumberToString(session_token));

    // Switches to pass to render processes.
    static const char* const kSwitchNames[] = {
        translate::switches::kBraveTranslateUseGoogleEndpoint,
    };
    command_line->CopySwitchesFrom(browser_command_line, kSwitchNames);
  }
}

std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
BraveContentBrowserClient::CreateURLLoaderThrottles(
    const network::ResourceRequest& request,
    content::BrowserContext* browser_context,
    const content::WebContents::Getter& wc_getter,
    content::NavigationUIData* navigation_ui_data,
    int frame_tree_node_id,
    std::optional<int64_t> navigation_id) {
  auto result = ChromeContentBrowserClient::CreateURLLoaderThrottles(
      request, browser_context, wc_getter, navigation_ui_data,
      frame_tree_node_id, navigation_id);
  content::WebContents* contents = wc_getter.Run();

  if (contents) {
    const bool isMainFrame =
        request.resource_type ==
        static_cast<int>(blink::mojom::ResourceType::kMainFrame);

    auto body_sniffer_throttle =
        std::make_unique<body_sniffer::BodySnifferThrottle>(
            base::SingleThreadTaskRunner::GetCurrentDefault());

    // Speedreader
#if BUILDFLAG(ENABLE_SPEEDREADER)
    auto* tab_helper =
        speedreader::SpeedreaderTabHelper::FromWebContents(contents);
    if (tab_helper && isMainFrame) {
      auto* speedreader_service =
          speedreader::SpeedreaderServiceFactory::GetForBrowserContext(
              browser_context);

      auto producer =
          speedreader::SpeedreaderDistilledPageProducer::MaybeCreate(
              tab_helper->GetWeakPtr());
      if (producer) {
        body_sniffer_throttle->SetBodyProducer(std::move(producer));
      }

      auto handler = speedreader::SpeedreaderBodyDistiller::MaybeCreate(
          g_brave_browser_process->speedreader_rewriter_service(),
          speedreader_service, tab_helper->GetWeakPtr());
      if (handler) {
        body_sniffer_throttle->AddHandler(std::move(handler));
      }
    }
#endif  // ENABLE_SPEEDREADER

    if (isMainFrame) {
      // De-AMP
      auto handler = de_amp::DeAmpBodyHandler::Create(request, wc_getter);
      if (handler) {
        body_sniffer_throttle->AddHandler(std::move(handler));
      }
    }

    result.push_back(std::move(body_sniffer_throttle));

    if (auto google_sign_in_permission_throttle =
            google_sign_in_permission::GoogleSignInPermissionThrottle::
                MaybeCreateThrottleFor(request, wc_getter)) {
      result.push_back(std::move(google_sign_in_permission_throttle));
    }
  }

  return result;
}

void BraveContentBrowserClient::WillCreateURLLoaderFactory(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* frame,
    int render_process_id,
    URLLoaderFactoryType type,
    const url::Origin& request_initiator,
    const net::IsolationInfo& isolation_info,
    std::optional<int64_t> navigation_id,
    ukm::SourceIdObj ukm_source_id,
    network::URLLoaderFactoryBuilder& factory_builder,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
        header_client,
    bool* bypass_redirect_checks,
    bool* disable_secure_dns,
    network::mojom::URLLoaderFactoryOverridePtr* factory_override,
    scoped_refptr<base::SequencedTaskRunner> navigation_response_task_runner) {
  // TODO(iefremov): Skip proxying for certain requests?
  BraveProxyingURLLoaderFactory::MaybeProxyRequest(
      browser_context, frame,
      type == URLLoaderFactoryType::kNavigation ? -1 : render_process_id,
      factory_builder, navigation_response_task_runner);

  ChromeContentBrowserClient::WillCreateURLLoaderFactory(
      browser_context, frame, render_process_id, type, request_initiator,
      isolation_info, std::move(navigation_id), ukm_source_id, factory_builder,
      header_client, bypass_redirect_checks, disable_secure_dns,
      factory_override, navigation_response_task_runner);
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
    const std::optional<std::string>& user_agent,
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
  if (document_url.SchemeIs(extensions::kExtensionScheme) ||
      request_url.SchemeIs(extensions::kExtensionScheme)) {
    return;
  }
#endif
  if (document_url.SchemeIs(content::kChromeUIScheme) ||
      request_url.SchemeIs(content::kChromeUIScheme)) {
    return;
  }

  Profile* profile = Profile::FromBrowserContext(browser_context);
  const bool allow_referrers = brave_shields::AreReferrersAllowed(
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
  if (!profile) {
    return url;
  }

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (extensions::ChromeContentBrowserClientExtensionsPart::
          AreExtensionsDisabledForProfile(profile)) {
    return url;
  }
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
  // Some of these rewrites are for WebUI pages with URL that has moved.
  // After rewrite happens, GetWebUIFactoryFunction() will work as expected.
  // (see browser\ui\webui\brave_web_ui_controller_factory.cc for more info)
  //
  // Scope of schema is intentionally narrower than content::HasWebUIScheme(url)
  // which also allows both `chrome-untrusted` and `chrome-devtools`.
  if (!url->SchemeIs(content::kBraveUIScheme) &&
      !url->SchemeIs(content::kChromeUIScheme)) {
    return false;
  }

  // brave://sync => brave://settings/braveSync
  if (url->host() == chrome::kChromeUISyncHost) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    replacements.SetHostStr(chrome::kChromeUISettingsHost);
    replacements.SetPathStr(kBraveSyncPath);
    *url = url->ReplaceComponents(replacements);
    return true;
  }

#if !BUILDFLAG(IS_ANDROID)
  // brave://adblock => brave://settings/shields/filters
  if (url->host() == kAdblockHost) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    replacements.SetHostStr(chrome::kChromeUISettingsHost);
    replacements.SetPathStr(kContentFiltersPath);
    *url = url->ReplaceComponents(replacements);
    return false;
  }
#endif

  // no special win10 welcome page
  if (url->host() == chrome::kChromeUIWelcomeHost) {
    *url = GURL(chrome::kChromeUIWelcomeURL);
    return true;
  }

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED) && BUILDFLAG(ENABLE_EXTENSIONS)
  auto* prefs = user_prefs::UserPrefs::Get(browser_context);
  brave_wallet::mojom::DefaultWallet default_wallet =
      brave_wallet::GetDefaultEthereumWallet(prefs);
  if (!brave_wallet::IsNativeWalletEnabled() ||
      default_wallet == brave_wallet::mojom::DefaultWallet::CryptoWallets) {
    // If the Crypto Wallets extension is loaded, then it replaces the WebUI
    auto* service =
        EthereumRemoteClientServiceFactory::GetForContext(browser_context);
    if (service->IsCryptoWalletsReady() &&
        url->SchemeIs(content::kChromeUIScheme) &&
        url->host() == kEthereumRemoteClientHost) {
      auto* registry = extensions::ExtensionRegistry::Get(browser_context);
      if (registry && registry->ready_extensions().GetByID(
                          kEthereumRemoteClientExtensionId)) {
        *url = GURL(kEthereumRemoteClientBaseUrl);
        return true;
      }
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

  // inserting the navigation throttle at the fist position before any java
  // navigation happens
  throttles.insert(
      throttles.begin(),
      std::make_unique<brave_rewards::RewardsProtocolNavigationThrottle>(
          handle));

#if !BUILDFLAG(IS_ANDROID)
  std::unique_ptr<content::NavigationThrottle> ntp_shows_navigation_throttle =
      NewTabShowsNavigationThrottle::MaybeCreateThrottleFor(handle);
  if (ntp_shows_navigation_throttle) {
    throttles.push_back(std::move(ntp_shows_navigation_throttle));
  }
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
  if (tor_navigation_throttle) {
    throttles.push_back(std::move(tor_navigation_throttle));
  }
  std::unique_ptr<content::NavigationThrottle>
      onion_location_navigation_throttle =
          tor::OnionLocationNavigationThrottle::MaybeCreateThrottleFor(
              handle, TorProfileServiceFactory::IsTorDisabled(context),
              context->IsTor());
  if (onion_location_navigation_throttle) {
    throttles.push_back(std::move(onion_location_navigation_throttle));
  }
#endif

  std::unique_ptr<content::NavigationThrottle>
      decentralized_dns_navigation_throttle =
          decentralized_dns::DecentralizedDnsNavigationThrottle::
              MaybeCreateThrottleFor(handle, g_browser_process->local_state(),
                                     g_browser_process->GetApplicationLocale());
  if (decentralized_dns_navigation_throttle) {
    throttles.push_back(std::move(decentralized_dns_navigation_throttle));
  }

  // Debounce
  if (auto debounce_throttle =
          debounce::DebounceNavigationThrottle::MaybeCreateThrottleFor(
              handle, debounce::DebounceServiceFactory::GetForBrowserContext(
                          context))) {
    throttles.push_back(std::move(debounce_throttle));
  }

  // The HostContentSettingsMap might be null for some irregular profiles, e.g.
  // the System Profile.
  auto* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(context);
  if (host_content_settings_map) {
    if (std::unique_ptr<content::NavigationThrottle>
            domain_block_navigation_throttle = brave_shields::
                DomainBlockNavigationThrottle::MaybeCreateThrottleFor(
                    handle, g_brave_browser_process->ad_block_service(),
                    g_brave_browser_process->ad_block_service()
                        ->custom_filters_provider(),
                    EphemeralStorageServiceFactory::GetForContext(context),
                    host_content_settings_map,
                    g_browser_process->GetApplicationLocale())) {
      throttles.push_back(std::move(domain_block_navigation_throttle));
    }
  }

#if BUILDFLAG(ENABLE_REQUEST_OTR)
  // Request Off-The-Record
  if (auto request_otr_throttle =
          request_otr::RequestOTRNavigationThrottle::MaybeCreateThrottleFor(
              handle,
              request_otr::RequestOTRServiceFactory::GetForBrowserContext(
                  context),
              EphemeralStorageServiceFactory::GetForContext(context),
              Profile::FromBrowserContext(context)->GetPrefs(),
              g_browser_process->GetApplicationLocale())) {
    throttles.push_back(std::move(request_otr_throttle));
  }
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (Profile::FromBrowserContext(context)->IsRegularProfile()) {
    if (auto ai_chat_throttle =
            ai_chat::AiChatThrottle::MaybeCreateThrottleFor(handle)) {
      throttles.push_back(std::move(ai_chat_throttle));
    }
  }
#endif  // ENABLE_AI_CHAT

  return throttles;
}

bool PreventDarkModeFingerprinting(WebContents* web_contents,
                                   WebPreferences* prefs) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  // The HostContentSettingsMap might be null for some irregular profiles, e.g.
  // the System Profile.
  auto* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!host_content_settings_map) {
    return false;
  }
  const GURL url = web_contents->GetLastCommittedURL();
  const bool shields_up =
      brave_shields::GetBraveShieldsEnabled(host_content_settings_map, url);
  auto fingerprinting_type = brave_shields::GetFingerprintingControlType(
      host_content_settings_map, url);
  // https://github.com/brave/brave-browser/issues/15265
  // Always use color scheme Light if fingerprinting mode strict
  if (base::FeatureList::IsEnabled(
          brave_shields::features::kBraveDarkModeBlock) &&
      shields_up && fingerprinting_type == ControlType::BLOCK &&
      prefs->preferred_color_scheme !=
          blink::mojom::PreferredColorScheme::kLight) {
    prefs->preferred_color_scheme = blink::mojom::PreferredColorScheme::kLight;
    return true;
  }
  return false;
}

bool BraveContentBrowserClient::OverrideWebPreferencesAfterNavigation(
    WebContents* web_contents,
    WebPreferences* prefs) {
  bool changed =
      ChromeContentBrowserClient::OverrideWebPreferencesAfterNavigation(
          web_contents, prefs);
  return PreventDarkModeFingerprinting(web_contents, prefs) || changed;
}

void BraveContentBrowserClient::OverrideWebkitPrefs(WebContents* web_contents,
                                                    WebPreferences* web_prefs) {
  ChromeContentBrowserClient::OverrideWebkitPrefs(web_contents, web_prefs);
  PreventDarkModeFingerprinting(web_contents, web_prefs);
  // This will stop NavigatorPlugins from returning fixed plugins data and will
  // allow us to return our farbled data
  web_prefs->allow_non_empty_navigator_plugins = true;

#if BUILDFLAG(ENABLE_PLAYLIST)
  if (playlist::PlaylistBackgroundWebContentsHelper::FromWebContents(
          web_contents)) {
    web_prefs->force_cosmetic_filtering = true;
  }
#endif
}

blink::UserAgentMetadata BraveContentBrowserClient::GetUserAgentMetadata() {
  blink::UserAgentMetadata metadata =
      ChromeContentBrowserClient::GetUserAgentMetadata();
  // Expect the brand version lists to have brand version, chromium_version, and
  // greased version.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(embedder_support::kUserAgent)) {
    return metadata;
  }
  DCHECK_EQ(3UL, metadata.brand_version_list.size());
  DCHECK_EQ(3UL, metadata.brand_full_version_list.size());
  // Zero out the last 3 version components in full version list versions.
  for (auto& brand_version : metadata.brand_full_version_list) {
    base::Version version(brand_version.version);
    brand_version.version =
        base::StrCat({base::NumberToString(version.components()[0]), ".0.0.0"});
  }
  // Zero out the last 3 version components in full version.
  base::Version version(metadata.full_version);
  metadata.full_version =
      base::StrCat({base::NumberToString(version.components()[0]), ".0.0.0"});
  return metadata;
}

GURL BraveContentBrowserClient::SanitizeURL(
    content::RenderFrameHost* render_frame_host,
    const GURL& url) {
  if (!base::FeatureList::IsEnabled(features::kBraveCopyCleanLinkFromJs)) {
    return url;
  }
  CHECK(render_frame_host);
  CHECK(render_frame_host->GetBrowserContext());
  auto* url_sanitizer_service =
      brave::URLSanitizerServiceFactory::GetForBrowserContext(
          render_frame_host->GetBrowserContext());
  CHECK(url_sanitizer_service);
  if (!url_sanitizer_service->CheckJsPermission(
          render_frame_host->GetLastCommittedURL())) {
    return url;
  }
  return url_sanitizer_service->SanitizeURL(url);
}

bool BraveContentBrowserClient::AllowSignedExchange(
    content::BrowserContext* context) {
  // This override has been introduced due to the deletion of the flag
  // `features::kSignedHTTPExchange`, which was being used to disable signed
  // exchanges.
  return false;
}
