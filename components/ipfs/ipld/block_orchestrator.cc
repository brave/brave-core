/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/block_orchestrator.h"
#include <memory>
#include <utility>
#include "base/functional/bind.h"

namespace ipfs::ipld {

BlockOrchestrator::BlockOrchestrator(PrefService* pref_service)
    : pref_service_(pref_service) {}

BlockOrchestrator::~BlockOrchestrator() = default;

void BlockOrchestrator::BuildResponse(std::unique_ptr<IpfsRequest> request,
                                      IpfsRequestCallback callback) {
  request_callback_ = std::move(callback);
  DCHECK(request);
  if (!request) {
    return;
  }

  auto car_reader = block_reader_factory_->CreateCarBlockReader(
      request->url, request->url_loader_factory, pref_service_);
  DCHECK(car_reader);
  if (!car_reader) {
    return;
  }

  car_reader->Read(base::BindRepeating(&BlockOrchestrator::OnBlockRead,
                                       weak_ptr_factory_.GetWeakPtr()));
}

void BlockOrchestrator::OnBlockRead(std::unique_ptr<Block> block,
                                    bool is_completed) {
  if (is_completed && !block && request_callback_) {
    // Here is where we finished to collect all blocks
    // TODO: Here we have to prepare the response and return it
    std::move(request_callback_)
        .Run(std::make_unique<IpfsResponse>("", 200, "", ""));
    return;
  }

  dag_nodes_.try_emplace(block->Cid(), std::move(block));
}

}  // namespace ipfs::ipld
