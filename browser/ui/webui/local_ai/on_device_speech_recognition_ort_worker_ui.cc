// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/local_ai/on_device_speech_recognition_ort_worker_ui.h"

#include <memory>

#include "brave/browser/speech/on_device_speech_recognition_controller.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/components/local_ai/resources/grit/on_device_speech_recognition_ort_worker_generated.h"
#include "brave/components/local_ai/resources/grit/on_device_speech_recognition_ort_worker_generated_map.h"
#include "brave/components/local_ai/resources/grit/ort_dist_generated.h"
#include "brave/components/local_ai/resources/grit/ort_dist_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/webui_util.h"

namespace local_ai {

UntrustedOnDeviceSpeechRecognitionOrtWorkerUI::
    UntrustedOnDeviceSpeechRecognitionOrtWorkerUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(),
      kOnDeviceSpeechRecognitionOrtWorkerURL);

  webui::SetupWebUIDataSource(source,
                              kOnDeviceSpeechRecognitionOrtWorkerGenerated,
                              IDR_ON_DEVICE_SPEECH_RECOGNITION_ORT_WORKER_HTML);

  // Serve the bundled onnxruntime-web distribution (loader, pthread worker
  // glue, threaded WASM) under /ort-dist/ from the pak built by
  // components/local_ai/resources/speech_worker_ort:ort_dist_generated.
  source->AddResourcePaths(kOrtDistGenerated);

  // Make the page cross-origin isolated so onnxruntime-web can use the
  // multi-threaded WASM backend (SharedArrayBuffer).
  source->OverrideCrossOriginOpenerPolicy("same-origin");
  source->OverrideCrossOriginEmbedderPolicy("require-corp");

  // Tighten the fallback that SetupWebUIDataSource set to 'self' down to
  // 'none', so every directive we do not explicitly grant below (img, media,
  // object, frame, manifest, ...) is denied without a per-directive line.
  // This re-Override replaces the earlier default-src value. This is
  // renderer-side defense in depth only: a compromised renderer that ignores
  // CSP is independently denied a network URLLoaderFactory by Chromium (WebUI
  // bindings, see RenderFrameHostImpl::CommitNavigation), and navigation to
  // web URLs is blocked by
  // Navigator::CheckWebUIRendererDoesNotDisplayNormalURL.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::DefaultSrc, "default-src 'none';");

  // Explicit grants for what the page actually uses.
  // script-src: dynamic import() of the ORT bundle plus WASM compile, which
  //   needs 'wasm-unsafe-eval'.
  // worker-src: onnxruntime-web spawns its thread pool as Web Workers; we pin
  //   those to a same-origin served script (ort.env.wasm.wasmPaths.mjs in
  //   speech_worker_ort.ts) rather than ORT's default inlined blob: worker, so
  //   'self' (no blob:) is sufficient and blocks any blob-URL worker.
  // connect-src: ORT fetches the runtime .wasm from this origin. Model weights
  //   arrive as mojo BigBuffers, never fetched, so same-origin is the only
  //   connection the page needs.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome-untrusted://resources 'self' 'wasm-unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc, "worker-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc, "style-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc, "connect-src 'self';");

  // These directives have no default-src fallback, so default-src 'none' above
  // does NOT cover them. They must be denied explicitly. base-uri blocks a
  // <base> tag from repointing relative URLs, form-action blocks form-based
  // egress, and frame-ancestors stops any page from embedding this worker.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::BaseURI, "base-uri 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FormAction, "form-action 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      "frame-ancestors 'none';");

  // Trusted Types enforcement (`require-trusted-types-for 'script'`) is set
  // unconditionally by SetupWebUIDataSource above and is not disabled here.
  // onnxruntime-web spawns its thread pool via `new Worker(<url string>)`,
  // which that enforcement routes through the `default` policy
  // (installed in speech_worker_ort.ts, accepting solely our own same-origin
  // ort-dist worker script). This line only narrows the broad policy
  // allowlist SetupWebUIDataSource sets down to what this page uses; the
  // `default` policy name it relies on is appended automatically by Brave's
  // chromium_src override of WebUIDataSourceImpl.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types;");
}

UntrustedOnDeviceSpeechRecognitionOrtWorkerUI::
    ~UntrustedOnDeviceSpeechRecognitionOrtWorkerUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedOnDeviceSpeechRecognitionOrtWorkerUI)

void UntrustedOnDeviceSpeechRecognitionOrtWorkerUI::BindInterface(
    mojo::PendingReceiver<mojom::SpeechRecognitionFactoryHost> receiver) {
  speech::OnDeviceSpeechRecognitionController::Get()->BindFactoryHost(
      std::move(receiver));
}

///////////////////////////////////////////////////////////////////////////////

UntrustedOnDeviceSpeechRecognitionOrtWorkerUIConfig::
    UntrustedOnDeviceSpeechRecognitionOrtWorkerUIConfig()
    : content::WebUIConfig(content::kChromeUIUntrustedScheme,
                           kOnDeviceSpeechRecognitionOrtWorkerHost) {}

std::unique_ptr<content::WebUIController>
UntrustedOnDeviceSpeechRecognitionOrtWorkerUIConfig::CreateWebUIController(
    content::WebUI* web_ui,
    const GURL& url) {
  return std::make_unique<UntrustedOnDeviceSpeechRecognitionOrtWorkerUI>(
      web_ui);
}

}  // namespace local_ai
