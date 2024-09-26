/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_content_renderer_client.h"

#include <utility>

#include "base/feature_list.h"
#include "base/ranges/algorithm.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "brave/components/brave_search/renderer/brave_search_render_frame_observer.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/cosmetic_filters/renderer/cosmetic_filters_js_render_frame_observer.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/safe_builtins/renderer/safe_builtins.h"
#include "brave/components/script_injector/renderer/script_injector_render_frame_observer.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/skus/renderer/skus_render_frame_observer.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/renderer/brave_render_thread_observer.h"
#include "brave/renderer/brave_wallet/brave_wallet_render_frame_observer.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/renderer/chrome_render_thread_observer.h"
#include "chrome/renderer/process_state.h"
#include "chrome/renderer/url_loader_throttle_provider_impl.h"
#include "content/public/renderer/render_thread.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#include "third_party/blink/public/web/modules/service_worker/web_service_worker_context_proxy.h"
#include "third_party/blink/public/web/web_script_controller.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/renderer/speedreader_render_frame_observer.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#if BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_mobile_subscription/renderer/android/subscription_render_frame_observer.h"
#endif  // BUILDFLAG(IS_ANDROID)
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/renderer/playlist_render_frame_observer.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "media/base/key_system_info.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT) && BUILDFLAG(IS_ANDROID)
#include "brave/components/ai_chat/core/common/features.h"
#endif

namespace {
void MaybeRemoveWidevineSupport(media::GetSupportedKeySystemsCB cb,
                                media::KeySystemInfos key_systems) {
#if BUILDFLAG(ENABLE_WIDEVINE)
  auto dynamic_params = BraveRenderThreadObserver::GetDynamicParams();
  if (!dynamic_params.widevine_enabled) {
    key_systems.erase(
        base::ranges::remove(
            key_systems, kWidevineKeySystem,
            [](const std::unique_ptr<media::KeySystemInfo>& key_system) {
              return key_system->GetBaseKeySystemName();
            }),
        key_systems.cend());
  }
#endif
  cb.Run(std::move(key_systems));
}

}  // namespace

BraveContentRendererClient::BraveContentRendererClient() = default;

void BraveContentRendererClient::
    SetRuntimeFeaturesDefaultsBeforeBlinkInitialization() {
  ChromeContentRendererClient::
      SetRuntimeFeaturesDefaultsBeforeBlinkInitialization();

  blink::WebRuntimeFeatures::EnableFledge(false);
  // Disable topics APIs because kBrowsingTopics feature is disabled
  blink::WebRuntimeFeatures::EnableTopicsAPI(false);
  blink::WebRuntimeFeatures::EnableTopicsDocumentAPI(false);
  blink::WebRuntimeFeatures::EnableWebGPUExperimentalFeatures(false);
  blink::WebRuntimeFeatures::EnableWebNFC(false);

  // These features don't have dedicated WebRuntimeFeatures wrappers.
  blink::WebRuntimeFeatures::EnableFeatureFromString("AdTagging", false);
  blink::WebRuntimeFeatures::EnableFeatureFromString("DigitalGoods", false);
  if (!base::FeatureList::IsEnabled(blink::features::kFileSystemAccessAPI)) {
    blink::WebRuntimeFeatures::EnableFeatureFromString("FileSystemAccessLocal",
                                                       false);
    blink::WebRuntimeFeatures::EnableFeatureFromString(
        "FileSystemAccessAPIExperimental", false);
  }
  blink::WebRuntimeFeatures::EnableFeatureFromString("FledgeMultiBid", false);

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  if (base::FeatureList::IsEnabled(
          blink::features::kMiddleButtonClickAutoscroll)) {
    blink::WebRuntimeFeatures::EnableFeatureFromString("MiddleClickAutoscroll",
                                                       true);
  }
#endif  // BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
}

BraveContentRendererClient::~BraveContentRendererClient() = default;

void BraveContentRendererClient::RenderThreadStarted() {
  ChromeContentRendererClient::RenderThreadStarted();

  brave_observer_ = std::make_unique<BraveRenderThreadObserver>();
  content::RenderThread::Get()->AddObserver(brave_observer_.get());
  brave_search_service_worker_holder_.SetBrowserInterfaceBrokerProxy(
      browser_interface_broker_.get());

  blink::WebScriptController::RegisterExtension(
      brave::SafeBuiltins::CreateV8Extension());
}

void BraveContentRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  ChromeContentRendererClient::RenderFrameCreated(render_frame);

  if (base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockCosmeticFiltering)) {
    auto dynamic_params_closure = base::BindRepeating([]() {
      auto dynamic_params = BraveRenderThreadObserver::GetDynamicParams();
      return dynamic_params.de_amp_enabled;
    });

    new cosmetic_filters::CosmeticFiltersJsRenderFrameObserver(
        render_frame, ISOLATED_WORLD_ID_BRAVE_INTERNAL, dynamic_params_closure);
  }

  if (base::FeatureList::IsEnabled(
          brave_wallet::features::kNativeBraveWalletFeature)) {
    new brave_wallet::BraveWalletRenderFrameObserver(
        render_frame,
        base::BindRepeating(&BraveRenderThreadObserver::GetDynamicParams));
  }

  new script_injector::ScriptInjectorRenderFrameObserver(render_frame);

  if (brave_search::IsDefaultAPIEnabled()) {
    new brave_search::BraveSearchRenderFrameObserver(
        render_frame, content::ISOLATED_WORLD_ID_GLOBAL);
  }

  if (base::FeatureList::IsEnabled(skus::features::kSkusFeature) &&
      !IsIncognitoProcess()) {
    new skus::SkusRenderFrameObserver(render_frame);
  }

#if BUILDFLAG(IS_ANDROID)
  if (brave_vpn::IsBraveVPNFeatureEnabled()
#if BUILDFLAG(ENABLE_AI_CHAT)
      || ai_chat::features::IsAIChatHistoryEnabled()
#endif
  ) {
    new brave_subscription::SubscriptionRenderFrameObserver(
        render_frame, content::ISOLATED_WORLD_ID_GLOBAL);
  }
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
  if (base::FeatureList::IsEnabled(speedreader::kSpeedreaderFeature)) {
    new speedreader::SpeedreaderRenderFrameObserver(
        render_frame, ISOLATED_WORLD_ID_BRAVE_INTERNAL);
  }
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist) &&
      !IsIncognitoProcess()) {
    new playlist::PlaylistRenderFrameObserver(
        render_frame, base::BindRepeating([] {
          return BraveRenderThreadObserver::GetDynamicParams().playlist_enabled;
        }),
        ISOLATED_WORLD_ID_BRAVE_INTERNAL);
  }
#endif
}

std::unique_ptr<media::KeySystemSupportRegistration>
BraveContentRendererClient::GetSupportedKeySystems(
    content::RenderFrame* render_frame,
    media::GetSupportedKeySystemsCB cb) {
  return ChromeContentRendererClient::GetSupportedKeySystems(
      render_frame, base::BindRepeating(&MaybeRemoveWidevineSupport, cb));
}

void BraveContentRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  auto* observer =
      cosmetic_filters::CosmeticFiltersJsRenderFrameObserver::Get(render_frame);
  // Run this before any extensions
  if (observer) {
    observer->RunScriptsAtDocumentStart();
  }

#if BUILDFLAG(ENABLE_PLAYLIST)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    if (auto* playlist_observer =
            playlist::PlaylistRenderFrameObserver::Get(render_frame)) {
      playlist_observer->RunScriptsAtDocumentStart();
    }
  }
#endif

  ChromeContentRendererClient::RunScriptsAtDocumentStart(render_frame);
}

void BraveContentRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
#if BUILDFLAG(ENABLE_PLAYLIST)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    if (auto* playlist_observer =
            playlist::PlaylistRenderFrameObserver::Get(render_frame)) {
      playlist_observer->RunScriptsAtDocumentEnd();
    }
  }
#endif

  ChromeContentRendererClient::RunScriptsAtDocumentEnd(render_frame);
}

void BraveContentRendererClient::WillEvaluateServiceWorkerOnWorkerThread(
    blink::WebServiceWorkerContextProxy* context_proxy,
    v8::Local<v8::Context> v8_context,
    int64_t service_worker_version_id,
    const GURL& service_worker_scope,
    const GURL& script_url,
    const blink::ServiceWorkerToken& service_worker_token) {
  brave_search_service_worker_holder_.WillEvaluateServiceWorkerOnWorkerThread(
      context_proxy, v8_context, service_worker_version_id,
      service_worker_scope, script_url);
  ChromeContentRendererClient::WillEvaluateServiceWorkerOnWorkerThread(
      context_proxy, v8_context, service_worker_version_id,
      service_worker_scope, script_url, service_worker_token);
}

void BraveContentRendererClient::WillDestroyServiceWorkerContextOnWorkerThread(
    v8::Local<v8::Context> v8_context,
    int64_t service_worker_version_id,
    const GURL& service_worker_scope,
    const GURL& script_url) {
  brave_search_service_worker_holder_
      .WillDestroyServiceWorkerContextOnWorkerThread(
          v8_context, service_worker_version_id, service_worker_scope,
          script_url);
  ChromeContentRendererClient::WillDestroyServiceWorkerContextOnWorkerThread(
      v8_context, service_worker_version_id, service_worker_scope, script_url);
}

std::unique_ptr<blink::URLLoaderThrottleProvider>
BraveContentRendererClient::CreateURLLoaderThrottleProvider(
    blink::URLLoaderThrottleProviderType provider_type) {
  return URLLoaderThrottleProviderImpl::Create(provider_type, this,
                                               browser_interface_broker_.get());
}

bool BraveContentRendererClient::IsOnionAllowed() const {
  return brave_observer_->IsOnionAllowed();
}
