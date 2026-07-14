// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/local_ai/on_device_speech_recognition_worker_ui.h"

#include <memory>

#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/components/local_ai/resources/grit/on_device_speech_recognition_worker_generated.h"
#include "brave/components/local_ai/resources/grit/on_device_speech_recognition_worker_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/webui_util.h"

namespace local_ai {

UntrustedOnDeviceSpeechRecognitionWorkerUI::
    UntrustedOnDeviceSpeechRecognitionWorkerUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(),
      kOnDeviceSpeechRecognitionWorkerURL);

  webui::SetupWebUIDataSource(source, kOnDeviceSpeechRecognitionWorkerGenerated,
                              IDR_ON_DEVICE_SPEECH_RECOGNITION_WORKER_HTML);

  // Make the page cross-origin isolated so onnxruntime-web can use the
  // multi-threaded WASM backend (SharedArrayBuffer).
  source->OverrideCrossOriginOpenerPolicy("same-origin");
  source->OverrideCrossOriginEmbedderPolicy("require-corp");

  // Tighten the fallback that SetupWebUIDataSource set to 'self' down to
  // 'none', so every directive we do not explicitly grant below (img, media,
  // object, frame, manifest, ...) is denied without a per-directive line.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::DefaultSrc, "default-src 'none';");

  // Explicit grants for what the page actually uses.
  // script-src: the worker script plus WASM compile, which needs
  //   'wasm-unsafe-eval'.
  // worker-src: onnxruntime-web spawns its thread pool as Web Workers from the
  //   same-origin worker glue rather than ORT's default inlined blob: worker,
  //   so 'self' (no blob:) is sufficient and blocks any blob-URL worker.
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
  // (installed in speech_worker.ts, accepting solely our own same-origin
  // worker glue). This line only narrows the broad policy
  // allowlist SetupWebUIDataSource sets down to what this page uses; the
  // `default` policy name it relies on is appended automatically by Brave's
  // chromium_src override of WebUIDataSourceImpl.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types;");
}

UntrustedOnDeviceSpeechRecognitionWorkerUI::
    ~UntrustedOnDeviceSpeechRecognitionWorkerUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedOnDeviceSpeechRecognitionWorkerUI)

///////////////////////////////////////////////////////////////////////////////

UntrustedOnDeviceSpeechRecognitionWorkerUIConfig::
    UntrustedOnDeviceSpeechRecognitionWorkerUIConfig()
    : content::WebUIConfig(content::kChromeUIUntrustedScheme,
                           kOnDeviceSpeechRecognitionWorkerHost) {}

std::unique_ptr<content::WebUIController>
UntrustedOnDeviceSpeechRecognitionWorkerUIConfig::CreateWebUIController(
    content::WebUI* web_ui,
    const GURL& url) {
  return std::make_unique<UntrustedOnDeviceSpeechRecognitionWorkerUI>(web_ui);
}

}  // namespace local_ai
