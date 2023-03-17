/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/logging.h"
#include "brave/mojo_examples/1/mojom/logger.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace logger {

class LoggerImpl : public mojom::Logger {
 public:
  // NOTE: A common pattern for interface implementations which have one
  // instance per client is to take a PendingReceiver in the constructor.

  explicit LoggerImpl(mojo::PendingReceiver<mojom::Logger> pending_receiver);

  LoggerImpl(const LoggerImpl&) = delete;
  LoggerImpl& operator=(const LoggerImpl&) = delete;

  ~LoggerImpl() override;

  void Log(const std::string& message) override;

  void GetTail(GetTailCallback callback) override;

 private:
  void OnError();

  mojo::Receiver<mojom::Logger> receiver_;
  std::vector<std::string> lines_;
};

}  // namespace logger
