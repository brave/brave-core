// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/browser/candle_service.h"

#include "base/logging.h"

namespace local_ai {

CandleService::CandleService() = default;
CandleService::~CandleService() = default;

void CandleService::BindBert(
    mojo::PendingRemote<mojom::BertInterface> pending_remote) {
  // Reset if already bound (e.g., on page reload)
  if (bert_remote_.is_bound()) {
    bert_remote_.reset();
  }
  bert_remote_.Bind(std::move(pending_remote));
}

void CandleService::RunBertExample() {
  bert_remote_->RunExample(base::BindOnce(&CandleService::OnRunBertExample,
                                          weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::OnRunBertExample(const std::string& result) {
  LOG(ERROR) << __PRETTY_FUNCTION__ << "\n" << result;
}

}  // namespace local_ai
