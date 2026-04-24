/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_recognition_controller.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/task/thread_pool.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/local_ai/content/background_web_contents_impl.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "url/gurl.h"

namespace speech {

namespace {

constexpr int64_t kChunkSizeBytes = 8 * 1024 * 1024;  // 8 MB

constexpr char kWorkerUrl[] =
    "chrome-untrusted://on-device-speech-recognition-worker/";

}  // namespace

OnDeviceSpeechRecognitionController::InitReadResult::InitReadResult() = default;
OnDeviceSpeechRecognitionController::InitReadResult::InitReadResult(
    InitReadResult&&) = default;
OnDeviceSpeechRecognitionController::InitReadResult&
OnDeviceSpeechRecognitionController::InitReadResult::operator=(
    InitReadResult&&) = default;
OnDeviceSpeechRecognitionController::InitReadResult::~InitReadResult() =
    default;

namespace {

// Runs on a ThreadPool blocking sequence. Reads the three small
// companion files plus the GGUF header (bytes 0..tensor_data_offset)
// and reports the total size of model.gguf so the caller knows how
// many chunks to stream.
OnDeviceSpeechRecognitionController::InitReadResult ReadInitFiles(
    const base::FilePath& config_path,
    const base::FilePath& tokenizer_path,
    const base::FilePath& mel_filters_path,
    const base::FilePath& model_path) {
  OnDeviceSpeechRecognitionController::InitReadResult result;

  std::string config_str;
  std::string tokenizer_str;
  if (!base::ReadFileToString(config_path, &config_str) ||
      !base::ReadFileToString(tokenizer_path, &tokenizer_str)) {
    return result;
  }

  std::optional<std::vector<uint8_t>> mel_filters =
      base::ReadFileToBytes(mel_filters_path);
  if (!mel_filters) {
    return result;
  }

  std::optional<base::Value> parsed =
      base::JSONReader::Read(config_str, base::JSON_PARSE_RFC);
  if (!parsed || !parsed->is_dict()) {
    return result;
  }
  const std::optional<int> offset_opt =
      parsed->GetDict().FindInt("tensor_data_offset");
  if (!offset_opt || *offset_opt <= 0) {
    return result;
  }
  const int64_t tensor_data_offset = *offset_opt;

  base::File model_file(model_path,
                        base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!model_file.IsValid()) {
    return result;
  }
  const int64_t total_bytes = model_file.GetLength();
  if (total_bytes <= tensor_data_offset) {
    return result;
  }

  std::vector<uint8_t> gguf_header(tensor_data_offset);
  const std::optional<size_t> read = model_file.Read(0, gguf_header);
  if (!read || static_cast<int64_t>(*read) != tensor_data_offset) {
    return result;
  }

  result.files = local_ai::mojom::SpeechRecognitionInitFiles::New();
  result.files->gguf_header = mojo_base::BigBuffer(std::move(gguf_header));
  result.files->config = mojo_base::BigBuffer(base::as_byte_span(config_str));
  result.files->tokenizer =
      mojo_base::BigBuffer(base::as_byte_span(tokenizer_str));
  result.files->mel_filters = mojo_base::BigBuffer(std::move(*mel_filters));
  result.model_total_bytes = total_bytes;
  result.tensor_data_offset = tensor_data_offset;
  return result;
}

// Runs on a ThreadPool blocking sequence. Reads a single chunk at
// the given offset. Returns an empty vector on error.
std::vector<uint8_t> ReadModelChunk(const base::FilePath& model_path,
                                    int64_t offset,
                                    int64_t size) {
  base::File file(model_path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    return {};
  }
  std::vector<uint8_t> buf(size);
  const std::optional<size_t> read = file.Read(offset, buf);
  if (!read) {
    return {};
  }
  buf.resize(*read);
  return buf;
}

}  // namespace

// static
OnDeviceSpeechRecognitionController*
OnDeviceSpeechRecognitionController::Get() {
  static base::NoDestructor<OnDeviceSpeechRecognitionController> instance;
  return instance.get();
}

OnDeviceSpeechRecognitionController::OnDeviceSpeechRecognitionController() {
  ApplyCommandLineSwitch();
  local_ai::OnDeviceSpeechModelsState::GetInstance()->AddObserver(this);
}

OnDeviceSpeechRecognitionController::~OnDeviceSpeechRecognitionController() {
  local_ai::OnDeviceSpeechModelsState::GetInstance()->RemoveObserver(this);
}

mojo::PendingRemote<local_ai::mojom::OnDeviceSpeechRecognitionService>
OnDeviceSpeechRecognitionController::MakeRemote() {
  mojo::PendingRemote<local_ai::mojom::OnDeviceSpeechRecognitionService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void OnDeviceSpeechRecognitionController::BindForWebContents(
    mojo::PendingReceiver<local_ai::mojom::OnDeviceSpeechRecognitionService>
        receiver) {
  receivers_.Add(this, std::move(receiver));
}

void OnDeviceSpeechRecognitionController::RegisterSpeechRecognitionFactory(
    mojo::PendingRemote<local_ai::mojom::SpeechRecognitionFactory> factory) {
  if (state_ != State::kBwcStarting) {
    return;
  }
  factory_.Bind(std::move(factory));
  factory_.set_disconnect_handler(base::BindOnce(
      &OnDeviceSpeechRecognitionController::OnFactoryDisconnected,
      weak_factory_.GetWeakPtr()));
  state_ = State::kModelLoading;
  LoadInitFiles();
}

void OnDeviceSpeechRecognitionController::OnBackgroundContentsDestroyed(
    local_ai::BackgroundWebContents::DestroyReason reason) {
  TearDown();
}

void OnDeviceSpeechRecognitionController::OnProfileWillBeDestroyed(
    Profile* profile) {
  if (profile != guest_profile_) {
    return;
  }
  TearDown();
}

void OnDeviceSpeechRecognitionController::OnModelsReady(
    const base::FilePath& install_dir) {
  // Don't pre-warm: the model is several hundred MB and should only
  // be loaded when a session actually needs it. CreateAsrSession
  // calls StartModelLoad() on demand. This observer fires both at
  // controller construction (via AddObserver replay) and when the
  // component updater delivers a new model mid-run, neither of
  // which should trigger an unrequested load.
}

void OnDeviceSpeechRecognitionController::ApplyCommandLineSwitch() {
  const auto* cmd = base::CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(switches::kOnDeviceSpeechRecognitionModelDir)) {
    return;
  }
  const base::FilePath dir =
      cmd->GetSwitchValuePath(switches::kOnDeviceSpeechRecognitionModelDir);
  if (dir.empty()) {
    return;
  }
  local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(dir);
}

void OnDeviceSpeechRecognitionController::StartModelLoad() {
  if (state_ != State::kIdle) {
    return;
  }
  state_ = State::kBwcStarting;
  EnsureBackgroundContents();
}

void OnDeviceSpeechRecognitionController::EnsureBackgroundContents() {
  auto* profile_manager = g_browser_process->profile_manager();
  profile_manager->CreateProfileAsync(
      ProfileManager::GetGuestProfilePath(),
      base::BindOnce(
          &OnDeviceSpeechRecognitionController::OnGuestProfileCreated,
          weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnGuestProfileCreated(
    Profile* guest_profile) {
  if (!guest_profile) {
    TearDown();
    return;
  }
  guest_profile_ = guest_profile;
  profile_observation_.Observe(guest_profile);

  background_web_contents_ =
      std::make_unique<local_ai::BackgroundWebContentsImpl>(
          guest_profile, GURL(kWorkerUrl), this,
          base::BindOnce([](content::WebContents* web_contents) {
            task_manager::WebContentsTags::CreateForToolContents(
                web_contents,
                IDS_ON_DEVICE_SPEECH_RECOGNITION_TASK_MANAGER_TITLE);
          }));
}

void OnDeviceSpeechRecognitionController::LoadInitFiles() {
  auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&ReadInitFiles, state->GetParakeetCtc110mConfig(),
                     state->GetParakeetCtc110mTokenizer(),
                     state->GetParakeetCtc110mMelFilters(),
                     state->GetParakeetCtc110mModel()),
      base::BindOnce(&OnDeviceSpeechRecognitionController::OnInitFilesRead,
                     weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnInitFilesRead(
    InitReadResult result) {
  if (!result.files) {
    LOG(ERROR) << "OnDeviceSpeechRecognition: failed to read model files from "
               << local_ai::OnDeviceSpeechModelsState::GetInstance()
                      ->GetParakeetCtc110mDir();
    TearDown();
    return;
  }
  model_bytes_total_ = result.model_total_bytes;
  model_bytes_sent_ = result.tensor_data_offset;
  factory_->Init(
      std::move(result.files),
      base::BindOnce(&OnDeviceSpeechRecognitionController::OnInitResult,
                     weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnInitResult(bool success) {
  if (!success) {
    TearDown();
    return;
  }
  ReadNextChunk();
}

void OnDeviceSpeechRecognitionController::ReadNextChunk() {
  if (model_bytes_sent_ >= model_bytes_total_) {
    factory_->Finalize(
        base::BindOnce(&OnDeviceSpeechRecognitionController::OnFinalizeResult,
                       weak_factory_.GetWeakPtr()));
    return;
  }
  const int64_t remaining = model_bytes_total_ - model_bytes_sent_;
  const int64_t chunk_size = std::min(kChunkSizeBytes, remaining);
  auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&ReadModelChunk, state->GetParakeetCtc110mModel(),
                     model_bytes_sent_, chunk_size),
      base::BindOnce(&OnDeviceSpeechRecognitionController::OnChunkRead,
                     weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnChunkRead(
    std::vector<uint8_t> chunk) {
  if (chunk.empty()) {
    TearDown();
    return;
  }
  model_bytes_sent_ += static_cast<int64_t>(chunk.size());
  factory_->LoadWeightChunk(
      mojo_base::BigBuffer(std::move(chunk)),
      base::BindOnce(&OnDeviceSpeechRecognitionController::OnChunkAck,
                     weak_factory_.GetWeakPtr()));
}

void OnDeviceSpeechRecognitionController::OnChunkAck(bool success) {
  if (!success) {
    TearDown();
    return;
  }
  ReadNextChunk();
}

void OnDeviceSpeechRecognitionController::OnFinalizeResult(bool success) {
  if (!success) {
    TearDown();
    return;
  }
  state_ = State::kReady;
}

void OnDeviceSpeechRecognitionController::OnFactoryDisconnected() {
  TearDown();
}

void OnDeviceSpeechRecognitionController::TearDown() {
  factory_.reset();
  background_web_contents_.reset();
  profile_observation_.Reset();
  guest_profile_ = nullptr;
  model_bytes_total_ = 0;
  model_bytes_sent_ = 0;
  state_ = State::kIdle;
}

}  // namespace speech
