// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/candle_service.h"

#include <map>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "components/grit/brave_components_resources.h"
#include "third_party/blink/public/common/messaging/web_message_port.h"
#include "ui/base/resource/resource_bundle.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/local_ai/browser/local_models_updater.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/storage_partition.h"
#include "third_party/blink/public/common/messaging/message_port_channel.h"
#include "third_party/blink/public/common/messaging/transferable_message.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"
#include "third_party/blink/public/mojom/script/script_type.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration_options.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace local_ai {

CandleService::LoadedFiles::LoadedFiles() = default;
CandleService::LoadedFiles::~LoadedFiles() = default;
CandleService::LoadedFiles::LoadedFiles(LoadedFiles&&) = default;
CandleService::LoadedFiles& CandleService::LoadedFiles::operator=(
    LoadedFiles&&) = default;

CandleService::PendingEmbedRequest::PendingEmbedRequest() = default;
CandleService::PendingEmbedRequest::PendingEmbedRequest(std::string text,
                                                        EmbedCallback callback)
    : text(std::move(text)), callback(std::move(callback)) {}
CandleService::PendingEmbedRequest::~PendingEmbedRequest() = default;
CandleService::PendingEmbedRequest::PendingEmbedRequest(PendingEmbedRequest&&) =
    default;
CandleService::PendingEmbedRequest&
CandleService::PendingEmbedRequest::operator=(PendingEmbedRequest&&) = default;

CandleService::CandleService(content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  DVLOG(3) << "CandleService created for browser context";

  if (!browser_context_) {
    DVLOG(0) << "CandleService: No browser context available";
    return;
  }

  // Get ServiceWorkerContext from the storage partition
  GURL sw_scope(kUntrustedCandleEmbeddingGemmaWasmURL);
  content::StoragePartition* partition =
      browser_context_->GetStoragePartitionForUrl(sw_scope);
  service_worker_context_ = partition->GetServiceWorkerContext();

  // Observe the component updater for model readiness
  LocalModelsUpdaterState::GetInstance()->AddObserver(this);
}

CandleService::~CandleService() {
  LocalModelsUpdaterState::GetInstance()->RemoveObserver(this);
}

void CandleService::BindReceiver(
    mojo::PendingReceiver<mojom::CandleService> receiver) {
  receivers_.Add(this, std::move(receiver));
  DVLOG(3) << "BindReceiver called";
}

void CandleService::BindEmbeddingGemma(
    mojo::PendingRemote<mojom::EmbeddingGemmaInterface> pending_remote) {
  // This is no longer used in the Service Worker approach.
  // The Service Worker communicates via MessagePort instead of Mojo.
  DVLOG(1) << "BindEmbeddingGemma called but not used in SW approach";
}

void CandleService::Embed(const std::string& text, EmbedCallback callback) {
  // Ensure Service Worker is running
  EnsureServiceWorker();

  // If model is not initialized yet, queue the request
  if (!model_initialized_) {
    DVLOG(3) << "Model not initialized yet, queuing embed request";
    pending_embed_requests_.emplace_back(text, std::move(callback));
    return;
  }

  // Send embed request to Service Worker
  int message_id = next_message_id_++;
  pending_callbacks_[message_id] = std::move(callback);

  // Create JSON message
  base::Value::Dict message;
  message.Set("type", "embed");
  message.Set("id", message_id);
  message.Set("text", text);

  std::string json_str;
  base::JSONWriter::Write(message, &json_str);
  std::u16string message_str = base::UTF8ToUTF16(json_str);

  message_port_.PostMessage(blink::WebMessagePort::Message(message_str));
}

void CandleService::OnLocalModelsReady(const base::FilePath& install_dir) {
  DVLOG(3) << "CandleService: Local models ready at: " << install_dir;
  component_ready_ = true;

  // Try to load model if Service Worker is ready
  if (port_connected_ && !model_initialized_) {
    LoadModelFiles();
  }
}

void CandleService::Shutdown() {
  DVLOG(3) << "CandleService: Shutting down";

  // Clear any pending requests
  for (auto& request : pending_embed_requests_) {
    std::move(request.callback).Run({});
  }
  pending_embed_requests_.clear();

  // Clear pending callbacks
  for (auto& [id, callback] : pending_callbacks_) {
    std::move(callback).Run({});
  }
  pending_callbacks_.clear();

  message_port_.Close();
}

bool CandleService::OnMessage(blink::WebMessagePort::Message message) {
  // Parse the JSON message from Service Worker
  std::string json_str = base::UTF16ToUTF8(message.data);
  auto parsed = base::JSONReader::Read(json_str, base::JSON_PARSE_RFC);
  if (!parsed || !parsed->is_dict()) {
    DVLOG(0) << "Failed to parse message from Service Worker: " << json_str;
    return false;
  }

  const base::Value::Dict& dict = parsed->GetDict();
  const std::string* type = dict.FindString("type");
  if (!type) {
    DVLOG(0) << "Message missing type field";
    return false;
  }

  DVLOG(3) << "Received message from Service Worker: " << *type;

  if (*type == "connected") {
    // Port connected, now we can send messages
    DVLOG(3) << "Service Worker port connected";
    port_connected_ = true;

    // If component is ready, load the model
    if (component_ready_ && !model_initialized_) {
      LoadModelFiles();
    }
    return true;
  }

  if (*type == "init-response") {
    std::optional<bool> success = dict.FindBool("success");
    if (success && *success) {
      DVLOG(3) << "Model initialized successfully";
      model_initialized_ = true;
      ProcessPendingEmbedRequests();
    } else {
      DVLOG(0) << "Model initialization failed";
    }
    return true;
  }

  if (*type == "embed-response") {
    std::optional<int> id = dict.FindInt("id");
    if (!id) {
      DVLOG(0) << "embed-response missing id";
      return false;
    }

    auto it = pending_callbacks_.find(*id);
    if (it == pending_callbacks_.end()) {
      DVLOG(0) << "No pending callback for id: " << *id;
      return false;
    }

    std::vector<double> output;
    const base::Value::List* output_list = dict.FindList("output");
    if (output_list) {
      for (const auto& val : *output_list) {
        if (val.is_double()) {
          output.push_back(val.GetDouble());
        } else if (val.is_int()) {
          output.push_back(static_cast<double>(val.GetInt()));
        }
      }
    }

    std::move(it->second).Run(std::move(output));
    pending_callbacks_.erase(it);
    return true;
  }

  DVLOG(0) << "Unknown message type: " << *type;
  return true;
}

void CandleService::OnPipeError() {
  DVLOG(1) << "Service Worker MessagePort error";

  // Clear pending requests on error
  for (auto& request : pending_embed_requests_) {
    std::move(request.callback).Run({});
  }
  pending_embed_requests_.clear();

  for (auto& [id, callback] : pending_callbacks_) {
    std::move(callback).Run({});
  }
  pending_callbacks_.clear();

  // Reset state so next Embed() will reinitialize
  port_connected_ = false;
  model_initialized_ = false;
  service_worker_started_ = false;
}

void CandleService::EnsureServiceWorker() {
  if (service_worker_started_ || !service_worker_context_) {
    return;
  }

  DVLOG(3) << "CandleService: Ensuring Service Worker is running";

  // Register the Service Worker if not already registered
  if (!service_worker_registered_) {
    // Use the plain JS Service Worker (not webpack-bundled) to avoid
    // top-level await issues. The SW uses static imports which requires
    // module script type.
    GURL script_url(std::string(kUntrustedCandleEmbeddingGemmaWasmURL) +
                    "candle_embedding_gemma_sw.js");
    GURL scope(kUntrustedCandleEmbeddingGemmaWasmURL);
    blink::StorageKey key =
        blink::StorageKey::CreateFirstParty(url::Origin::Create(scope));

    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = scope;
    // Use module script type since the SW uses static imports
    options.type = blink::mojom::ScriptType::kModule;

    service_worker_context_->RegisterServiceWorker(
        script_url, key, options,
        base::BindOnce(&CandleService::OnServiceWorkerRegistered,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // If registered, start the Service Worker
  StartServiceWorkerWithPort();
}

void CandleService::StartServiceWorkerWithPort() {
  if (service_worker_started_ || !service_worker_context_) {
    return;
  }

  DVLOG(3) << "CandleService: Starting Service Worker with MessagePort";

  GURL scope(kUntrustedCandleEmbeddingGemmaWasmURL);
  blink::StorageKey key =
      blink::StorageKey::CreateFirstParty(url::Origin::Create(scope));

  // Create a MessagePort pair for communication
  auto port_pair = blink::WebMessagePort::CreatePair();
  message_port_ = std::move(port_pair.first);
  message_port_.SetReceiver(this,
                            base::SequencedTaskRunner::GetCurrentDefault());

  // Create a TransferableMessage with the other port
  blink::TransferableMessage transferable_message;
  std::vector<blink::MessagePortDescriptor> ports;
  ports.push_back(port_pair.second.PassPort());
  transferable_message.ports =
      blink::MessagePortChannel::CreateFromHandles(std::move(ports));
  // Set the sender agent cluster ID to the embedder's (required for
  // serialization)
  transferable_message.sender_agent_cluster_id =
      blink::WebMessagePort::GetEmbedderAgentClusterID();

  // Dispatch the message to start the Service Worker and send the port
  service_worker_context_->StartServiceWorkerAndDispatchMessage(
      scope, key, std::move(transferable_message),
      base::BindOnce(&CandleService::OnServiceWorkerStartResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnServiceWorkerStartResult(bool success) {
  DVLOG(3) << "StartServiceWorkerAndDispatchMessage result: " << success;

  if (success) {
    service_worker_started_ = true;
  } else {
    // SW might still be installing, retry after a delay
    DVLOG(3) << "Service Worker not ready, retrying in 200ms";
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&CandleService::StartServiceWorkerWithPort,
                       weak_ptr_factory_.GetWeakPtr()),
        base::Milliseconds(200));
  }
}

void CandleService::OnServiceWorkerRegistered(
    blink::ServiceWorkerStatusCode status) {
  if (status != blink::ServiceWorkerStatusCode::kOk) {
    DVLOG(0) << "Failed to register Service Worker: "
             << static_cast<int>(status);
    return;
  }

  DVLOG(3) << "Service Worker registered successfully";
  service_worker_registered_ = true;

  // Wait a moment for the SW to finish installing before starting it.
  // The registration callback fires when registration starts, but the SW
  // may still be installing/activating.
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&CandleService::StartServiceWorkerWithPort,
                     weak_ptr_factory_.GetWeakPtr()),
      base::Milliseconds(100));
}

void CandleService::OnServiceWorkerStarted(int64_t version_id,
                                           int process_id,
                                           int thread_id) {
  DVLOG(3) << "Service Worker started: version_id=" << version_id
           << ", process_id=" << process_id << ", thread_id=" << thread_id;
  service_worker_version_id_ = version_id;
  service_worker_started_ = true;
}

void CandleService::SendMessageToServiceWorker(
    blink::WebMessagePort::Message message) {
  if (!port_connected_) {
    DVLOG(0) << "Cannot send message: port not connected";
    return;
  }
  message_port_.PostMessage(std::move(message));
}

void CandleService::LoadModelFiles() {
  DVLOG(3) << "CandleService: Loading model files to send via MessagePort";

  if (!port_connected_) {
    DVLOG(0) << "Cannot send init message: port not connected";
    return;
  }

  // Check if model directory is available
  const base::FilePath& model_dir =
      LocalModelsUpdaterState::GetInstance()->GetEmbeddingGemmaModelDir();
  if (model_dir.empty()) {
    DVLOG(0) << "CandleService: Model directory not set in updater state";
    return;
  }

  // Load all files on a background thread, then send via MessagePort
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&CandleService::LoadFilesOnBackgroundThread,
                     LocalModelsUpdaterState::GetInstance()
                         ->GetEmbeddingGemmaModel(),
                     LocalModelsUpdaterState::GetInstance()
                         ->GetEmbeddingGemmaDense1(),
                     LocalModelsUpdaterState::GetInstance()
                         ->GetEmbeddingGemmaDense2(),
                     LocalModelsUpdaterState::GetInstance()
                         ->GetEmbeddingGemmaTokenizer(),
                     LocalModelsUpdaterState::GetInstance()
                         ->GetEmbeddingGemmaConfig()),
      base::BindOnce(&CandleService::OnFilesLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

// static
std::optional<CandleService::LoadedFiles>
CandleService::LoadFilesOnBackgroundThread(
    const base::FilePath& weights_path,
    const base::FilePath& dense1_path,
    const base::FilePath& dense2_path,
    const base::FilePath& tokenizer_path,
    const base::FilePath& config_path) {
  LoadedFiles files;

  auto weights = base::ReadFileToBytes(weights_path);
  if (!weights) {
    LOG(ERROR) << "Failed to read weights file";
    return std::nullopt;
  }
  files.weights = std::move(*weights);

  auto dense1 = base::ReadFileToBytes(dense1_path);
  if (!dense1) {
    LOG(ERROR) << "Failed to read dense1 file";
    return std::nullopt;
  }
  files.dense1 = std::move(*dense1);

  auto dense2 = base::ReadFileToBytes(dense2_path);
  if (!dense2) {
    LOG(ERROR) << "Failed to read dense2 file";
    return std::nullopt;
  }
  files.dense2 = std::move(*dense2);

  auto tokenizer = base::ReadFileToBytes(tokenizer_path);
  if (!tokenizer) {
    LOG(ERROR) << "Failed to read tokenizer file";
    return std::nullopt;
  }
  files.tokenizer = std::move(*tokenizer);

  auto config = base::ReadFileToBytes(config_path);
  if (!config) {
    LOG(ERROR) << "Failed to read config file";
    return std::nullopt;
  }
  files.config = std::move(*config);

  return files;
}

void CandleService::OnFilesLoaded(std::optional<LoadedFiles> files) {
  if (!files) {
    DVLOG(0) << "Failed to load model files";
    return;
  }

  DVLOG(3) << "CandleService: Files loaded, sending to Service Worker";
  DVLOG(3) << "  weights: " << files->weights.size() << " bytes";
  DVLOG(3) << "  dense1: " << files->dense1.size() << " bytes";
  DVLOG(3) << "  dense2: " << files->dense2.size() << " bytes";
  DVLOG(3) << "  tokenizer: " << files->tokenizer.size() << " bytes";
  DVLOG(3) << "  config: " << files->config.size() << " bytes";

  // Also load the WASM binary from grit resources
  scoped_refptr<base::RefCountedMemory> wasm_bytes =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
          IDR_CANDLE_EMBEDDING_GEMMA_WEB_WASM);
  if (!wasm_bytes) {
    DVLOG(0) << "Failed to load WASM binary from resources";
    return;
  }
  DVLOG(3) << "  wasm: " << wasm_bytes->size() << " bytes";

  // Store files for sending via MessagePort
  pending_model_files_ = std::move(files);
  pending_wasm_bytes_ = std::move(wasm_bytes);

  // Send init message via MessagePort with base64-encoded data
  SendInitWithDataMessage();
}

void CandleService::SendInitWithDataMessage() {
  if (!pending_model_files_ || !pending_wasm_bytes_) {
    DVLOG(0) << "No pending files to send";
    return;
  }

  if (!port_connected_) {
    DVLOG(0) << "Cannot send init message: port not connected";
    return;
  }

  DVLOG(3) << "Sending init-with-data via MessagePort (base64 encoded)";

  // Create JSON message with base64-encoded file data
  // While this adds ~33% overhead, it works reliably with MessagePort
  base::Value::Dict message;
  message.Set("type", "init-with-data");
  message.Set("id", 0);
  message.Set("weights",
              base::Base64Encode(pending_model_files_->weights));
  message.Set("dense1",
              base::Base64Encode(pending_model_files_->dense1));
  message.Set("dense2",
              base::Base64Encode(pending_model_files_->dense2));
  message.Set("tokenizer",
              base::Base64Encode(pending_model_files_->tokenizer));
  message.Set("config",
              base::Base64Encode(pending_model_files_->config));
  message.Set("wasm", base::Base64Encode(*pending_wasm_bytes_));

  // Clear pending data
  pending_model_files_.reset();
  pending_wasm_bytes_.reset();

  std::string json_str;
  base::JSONWriter::Write(message, &json_str);
  DVLOG(3) << "Init message size: " << json_str.size() << " bytes";

  std::u16string message_str = base::UTF8ToUTF16(json_str);
  message_port_.PostMessage(blink::WebMessagePort::Message(message_str));
}

void CandleService::OnModelFilesLoaded(mojom::ModelFilesPtr model_files) {
  // This method is no longer used in the Service Worker approach.
  // The Service Worker fetches model files directly via URL.
  // Keeping this for potential fallback or future use.
  DVLOG(3) << "CandleService::OnModelFilesLoaded called (unused in SW mode)";
}

void CandleService::ProcessPendingEmbedRequests() {
  if (!model_initialized_) {
    return;
  }

  DVLOG(3) << "Processing " << pending_embed_requests_.size()
           << " pending embed requests";

  // Process all queued requests
  for (auto& request : pending_embed_requests_) {
    int message_id = next_message_id_++;
    pending_callbacks_[message_id] = std::move(request.callback);

    base::Value::Dict message;
    message.Set("type", "embed");
    message.Set("id", message_id);
    message.Set("text", request.text);

    std::string json_str;
    base::JSONWriter::Write(message, &json_str);
    std::u16string message_str = base::UTF8ToUTF16(json_str);

    message_port_.PostMessage(blink::WebMessagePort::Message(message_str));
  }
  pending_embed_requests_.clear();
}

}  // namespace local_ai
