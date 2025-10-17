// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/candle_service.h"

#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "components/grit/brave_components_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace local_ai {

namespace {

// TODO(darkdh): Use component updater to deliver model files
mojom::ModelFilesPtr LoadModelFilesFromResources() {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();

  scoped_refptr<base::RefCountedMemory> weights_data =
      resource_bundle.LoadDataResourceBytes(
          IDR_LOCAL_AI_BERT_MODEL_SAFETENSORS);
  scoped_refptr<base::RefCountedMemory> tokenizer_data =
      resource_bundle.LoadDataResourceBytes(IDR_LOCAL_AI_BERT_TOKENIZER_JSON);
  scoped_refptr<base::RefCountedMemory> config_data =
      resource_bundle.LoadDataResourceBytes(IDR_LOCAL_AI_BERT_CONFIG_JSON);

  if (!weights_data || !tokenizer_data || !config_data) {
    LOG(ERROR) << "Failed to load BERT model files from resources";
    return nullptr;
  }

  auto model_files = mojom::ModelFiles::New();
  model_files->weights.assign(weights_data->begin(), weights_data->end());
  model_files->tokenizer.assign(tokenizer_data->begin(), tokenizer_data->end());
  model_files->config.assign(config_data->begin(), config_data->end());

  return model_files;
}

}  // namespace

mojom::ModelFilesPtr LoadBertModelFiles() {
  return LoadModelFilesFromResources();
}

CandleService::CandleService() = default;
CandleService::~CandleService() = default;

void CandleService::BindBert(
    mojo::PendingRemote<mojom::BertInterface> pending_remote) {
  embedding_gemma_remote_.Bind(std::move(pending_remote));
}

void CandleService::RunExample() {
  auto model_files = LoadBertModelFiles();
  if (!model_files) {
    LOG(ERROR) << "Failed to load model files";
    return;
  }

  embedding_gemma_remote_->RunExample(
      std::move(model_files), base::BindOnce(&CandleService::OnRunExample,
                                             weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnRunExample(const std::string& result) {
  LOG(ERROR) << __PRETTY_FUNCTION__ << "\n" << result;
}

}  // namespace local_ai
