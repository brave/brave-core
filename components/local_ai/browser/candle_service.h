// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/common/candle.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace local_ai {

class CandleService : mojom::CandleService {
 public:
  CandleService();
  ~CandleService() override;

  void BindBert(mojo::PendingRemote<mojom::BertInterface>) override;

  void RunBertExample();

 private:
  void OnRunBertExample(const std::string& result);

  mojo::Remote<mojom::BertInterface> bert_remote_;

  base::WeakPtrFactory<CandleService> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_
