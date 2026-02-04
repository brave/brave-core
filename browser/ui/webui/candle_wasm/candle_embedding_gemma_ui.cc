// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/candle_wasm/candle_embedding_gemma_ui.h"

#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "brave/browser/local_ai/candle_service_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/local_ai/browser/candle_service.h"
#include "brave/components/local_ai/browser/local_models_updater.h"
#include "brave/components/local_ai/resources/grit/candle_embedding_gemma_bridge_generated.h"
#include "brave/components/local_ai/resources/grit/candle_embedding_gemma_bridge_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_paths.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/webui/webui_util.h"

namespace local_ai {

namespace {

// Request filter for serving model files from the component directory.
// This allows the Service Worker to fetch model files via URLs like:
// chrome-untrusted://candle-embedding-gemma-wasm/model/weights.bin
void HandleModelFileRequest(
    const std::string& path,
    content::WebUIDataSource::GotDataCallback callback) {
  // Get the model directory from the updater state
  const base::FilePath& model_dir =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaModelDir();
  if (model_dir.empty()) {
    std::move(callback).Run(nullptr);
    return;
  }

  // Map URL paths to file paths
  base::FilePath file_path;
  std::string file_name = path.substr(6);  // Remove "model/" prefix

  if (file_name == "weights.gguf") {
    file_path =
        LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaModel();
  } else if (file_name == "dense1.safetensors") {
    file_path =
        LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaDense1();
  } else if (file_name == "dense2.safetensors") {
    file_path =
        LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaDense2();
  } else if (file_name == "tokenizer.json") {
    file_path =
        LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaTokenizer();
  } else if (file_name == "config.json") {
    file_path =
        LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaConfig();
  } else {
    std::move(callback).Run(nullptr);
    return;
  }

  // Read file on a background thread
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(
          [](const base::FilePath& path)
              -> scoped_refptr<base::RefCountedMemory> {
            auto bytes = base::ReadFileToBytes(path);
            if (!bytes) {
              return nullptr;
            }
            return base::MakeRefCounted<base::RefCountedBytes>(
                std::move(*bytes));
          },
          file_path),
      std::move(callback));
}

// Request filter for serving web-target WASM files and Service Worker
// from grit resources. The web-target generates an explicit init() function
// instead of auto-initialization with top-level await, making it compatible
// with Service Workers.
void HandleWasmFileRequest(const std::string& path,
                           content::WebUIDataSource::GotDataCallback callback) {
  std::string file_name = path.substr(5);  // Remove "wasm/" prefix

  int resource_id = -1;
  if (file_name == "index.js") {
    resource_id = IDR_CANDLE_EMBEDDING_GEMMA_WEB_JS;
  } else if (file_name == "index_bg.wasm") {
    resource_id = IDR_CANDLE_EMBEDDING_GEMMA_WEB_WASM;
  }

  if (resource_id == -1) {
    std::move(callback).Run(nullptr);
    return;
  }

  scoped_refptr<base::RefCountedMemory> bytes =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
          resource_id);
  std::move(callback).Run(bytes);
}

// Request filter for serving the Service Worker script.
void HandleServiceWorkerRequest(
    content::WebUIDataSource::GotDataCallback callback) {
  scoped_refptr<base::RefCountedMemory> bytes =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
          IDR_CANDLE_EMBEDDING_GEMMA_SW_JS);
  std::move(callback).Run(bytes);
}

// Helper function to create and configure the WebUI data source.
// This is called both from the WebUIController constructor and from
// RegisterURLDataSource (for Service Worker registration).
void CreateDataSource(content::BrowserContext* browser_context) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      browser_context, kUntrustedCandleEmbeddingGemmaWasmURL);

  // Set default resource and add generated paths
  source->SetDefaultResource(IDR_CANDLE_EMBEDDING_GEMMA_BRIDGE_HTML);
  source->AddResourcePaths(kCandleEmbeddingGemmaBridgeGenerated);

  // Setup WebUI data source with generated resources
  webui::SetupWebUIDataSource(source, kCandleEmbeddingGemmaBridgeGenerated,
                              IDR_CANDLE_EMBEDDING_GEMMA_BRIDGE_HTML);

  // Add request filter for model files, WASM files, and Service Worker
  source->SetRequestFilter(
      base::BindRepeating([](const std::string& path) {
        return base::StartsWith(path, "model/") ||
               base::StartsWith(path, "wasm/") ||
               path == "candle_embedding_gemma_sw.js";
      }),
      base::BindRepeating([](const std::string& path,
                             content::WebUIDataSource::GotDataCallback cb) {
        if (base::StartsWith(path, "model/")) {
          HandleModelFileRequest(path, std::move(cb));
        } else if (base::StartsWith(path, "wasm/")) {
          HandleWasmFileRequest(path, std::move(cb));
        } else if (path == "candle_embedding_gemma_sw.js") {
          HandleServiceWorkerRequest(std::move(cb));
        } else {
          std::move(cb).Run(nullptr);
        }
      }));

  // Set up CSP to allow WASM execution and Service Worker
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src chrome://resources chrome-untrusted://resources "
                  "'self' 'wasm-unsafe-eval';"));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'self';"));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc,
      "connect-src chrome://resources chrome-untrusted://resources 'self' "
      "'wasm-unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      std::string("font-src 'self' data:;"));
  // Allow Service Worker registration
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc,
      std::string("worker-src 'self';"));
}

}  // namespace

UntrustedCandleEmbeddingGemmaUI::UntrustedCandleEmbeddingGemmaUI(
    content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  CreateDataSource(web_ui->GetWebContents()->GetBrowserContext());
}

UntrustedCandleEmbeddingGemmaUI::~UntrustedCandleEmbeddingGemmaUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedCandleEmbeddingGemmaUI)

void UntrustedCandleEmbeddingGemmaUI::BindInterface(
    mojo::PendingReceiver<mojom::CandleService> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  auto* service = CandleServiceFactory::GetForProfile(profile);
  if (service) {
    service->BindReceiver(std::move(receiver));
  }
}

///////////////////////////////////////////////////////////////////////////////

UntrustedCandleEmbeddingGemmaUIConfig::UntrustedCandleEmbeddingGemmaUIConfig()
    : content::WebUIConfig(content::kChromeUIUntrustedScheme,
                           kUntrustedCandleEmbeddingGemmaWasmHost) {}

std::unique_ptr<content::WebUIController>
UntrustedCandleEmbeddingGemmaUIConfig::CreateWebUIController(
    content::WebUI* web_ui,
    const GURL& url) {
  return std::make_unique<UntrustedCandleEmbeddingGemmaUI>(web_ui);
}

void UntrustedCandleEmbeddingGemmaUIConfig::RegisterURLDataSource(
    content::BrowserContext* browser_context) {
  CreateDataSource(browser_context);
}

}  // namespace local_ai
