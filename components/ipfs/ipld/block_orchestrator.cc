/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/block_orchestrator.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

#include "absl/types/optional.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/ipld/block_reader.h"
#include "net/http/http_status_code.h"

namespace {

constexpr char kDefaultMimeType[] = "text/plain";

void EnumerateBlocksFromCid(
    const std::string& cid_to_start,
    const std::unordered_map<std::string,
                             std::unique_ptr<ipfs::ipld::Block>,
                             ipfs::ipld::StringHash,
                             std::equal_to<>>& dag_nodes,
    base::RepeatingCallback<void(ipfs::ipld::Block const*, bool is_completed)>
        for_each_block_callback) {
  std::deque<ipfs::ipld::Block*> blocks_deque;
  auto current_iter = dag_nodes.find(cid_to_start);
  DCHECK(current_iter != dag_nodes.end());
  if (current_iter == dag_nodes.end()) {
    return;
  }

  auto* current = current_iter->second.get();
  while (current != nullptr || !blocks_deque.empty()) {
    if (current) {
      DCHECK(current != nullptr);
      blocks_deque.push_back(current);
    }

    while (current != nullptr) {
      if (!current->GetLinks()) {
        current = nullptr;
        continue;
      }

      base::ranges::for_each(*current->GetLinks(), [&](const auto& item) {
        auto current_link_iter = dag_nodes.find(item.hash);
        //        LOG(INFO) << "[IPFS] try to find hash:" << item.hash;
        DCHECK(current_link_iter != dag_nodes.end());
        if (current_link_iter != dag_nodes.end()) {
          current = current_link_iter->second.get();
          DCHECK(current != nullptr);
          blocks_deque.push_back(current);
        }
      });
    }

    current = blocks_deque.front();
    blocks_deque.pop_front();

    if (for_each_block_callback) {
      LOG(INFO) << "[IPFS] for_each_block_callback cid:"
                << (current ? current->Cid() : "n\a")
                << " blocks_deque.empty():" << blocks_deque.empty();
      for_each_block_callback.Run(current, !current || blocks_deque.empty());
    }
    current = nullptr;
  }
}

absl::optional<std::string> GetRootCid(
    const ipfs::ipld::BlockOrchestrator::BlockCollectorMap& dag_nodes) {
  DCHECK(!dag_nodes.empty());

  auto root_block_iter = dag_nodes.find("");
  if (root_block_iter == dag_nodes.end()) {
    return absl::nullopt;
  }
  const auto* root_cids_list =
      root_block_iter->second->Meta().FindList("roots");

  if (!root_cids_list || root_cids_list->empty()) {
    return absl::nullopt;
  }

  return root_cids_list->front().GetString();
}

}  // namespace

namespace ipfs::ipld {

BlockOrchestrator::BlockOrchestrator(PrefService* pref_service)
    : pref_service_(pref_service) {}

BlockOrchestrator::~BlockOrchestrator() = default;

void BlockOrchestrator::BuildResponse(
    std::unique_ptr<IpfsTrustlessRequest> request,
    IpfsRequestCallback callback) {
  DCHECK(request);
  DCHECK(!IsActive());
  if (!request || IsActive()) {
    return;
  }

  request_callback_ = std::move(callback);
  request_ = std::move(request);
  SendRequest(request_->url);
}

void BlockOrchestrator::SendRequest(const GURL& url) {
  LOG(INFO) << "[IPFS] SendRequest #10";
  block_reader_ = BlockReaderFactory().CreateCarBlockReader(
      url, request_->url_loader_factory, pref_service_,
      request_->only_structure);
  DCHECK(block_reader_);
  if (!block_reader_) {
    return;
  }

  LOG(INFO) << "[IPFS] SendRequest #20";
  block_reader_->Read(base::BindRepeating(&BlockOrchestrator::OnBlockRead,
                                          weak_ptr_factory_.GetWeakPtr()));
  LOG(INFO) << "[IPFS] SendRequest #30";
}

void BlockOrchestrator::OnBlockRead(std::unique_ptr<Block> block,
                                    const bool is_completed,
                                    const int& error_code) {
  LOG(INFO) << "[IPFS] OnBlockRead is_completed:"
            << is_completed << " Cid:" << (block ? block->Cid() : "n/a")
            << " error_code:" << error_code;

  if (!is_completed) {
    LOG(INFO) << "[IPFS] OnBlockRead Not Completed, Collected block cid:" << (block ? block->Cid() : "n/a");
    dag_nodes_.try_emplace(block->Cid(), std::move(block));
    return;
  }

  DCHECK(request_callback_);

  if (error_code != net::HTTP_OK) {
    LOG(INFO) << "[IPFS] OnBlockRead is_completed && "
                 "error_code != net::HTTP_OK";
    request_callback_.Run(
        std::move(request_),
        std::make_unique<IpfsTrustlessResponse>(kDefaultMimeType, error_code,
                                                nullptr, "", 0, true));
    return;
  }

  if (!block) {
    LOG(INFO) << "[IPFS] OnBlockRead Block collecting "
                 "finished Root CID:"
              << (dag_nodes_.contains("") ? dag_nodes_[""]->Meta().DebugString() : "N/A");
    // Here is where we finished to collect all blocks
    auto root_cid = GetRootCid(dag_nodes_);

    DCHECK(root_cid);
    if (!root_cid || root_cid->empty()) {
      return;
    }

    ProcessBlock(*root_cid);
    return;
  }

  LOG(INFO) << "[IPFS] OnBlockRead collected block cid:" << (block ? block->Cid() : "n/a");
  dag_nodes_.try_emplace(block->Cid(), std::move(block));
}

void BlockOrchestrator::ProcessBlock(const std::string& cid) {
  LOG(INFO) << "[IPFS] BlockOrchestrator::ProcessBlock cid:" << cid;
  DCHECK(!cid.empty());
  auto block_iter = dag_nodes_.find(cid);
  if (block_iter == dag_nodes_.end()) {
    GURL sub_request_url(base::StringPrintf(
        "ipfs://%s",
        cid.c_str()));  // TODO change it to manage with replace URL parts
    SendRequest(sub_request_url);
    return;
  }

  const auto* block = block_iter->second.get();
  DCHECK(block);
  LOG(INFO) << "[IPFS] BlockOrchestrator::ProcessBlock cid:"
            << block->Cid();
  if (block->IsContent()) {
    LOG(INFO) << "[IPFS] BlockOrchestrator::ProcessBlock Content";
    std::string_view vontent_view((const char*)block->GetContentData()->data(),
                                  block->GetContentData()->size());
    auto mime_type{mime_sniffer_->GetMime("", vontent_view, request_->url)};
    LOG(INFO) << "[IPFS] MIME type:" << mime_type.value_or("N/A");
    request_callback_.Run(std::move(request_),
                          std::make_unique<IpfsTrustlessResponse>(
                              mime_type.value_or(kDefaultMimeType),
                              net::HTTP_OK, block->GetContentData(), "",
                              block->GetContentData()->size(), true));
  } else if (block->IsMultiblockFile()) {
    uint64_t size{0};
    base::ranges::for_each(*block->GetLinks(), [&size](const auto& item) {
      LOG(INFO) << "[IPFS] Size Calculation: " << item.size;
      size += item.size;
    });

    LOG(INFO) << "[IPFS] BlockOrchestrator::ProcessBlock MultiblockFile ";

    EnumerateBlocksFromCid(
        block->Cid(), dag_nodes_,
        base::BindRepeating(&BlockOrchestrator::BlockChainForCid,
                            weak_ptr_factory_.GetWeakPtr(), size));
  } else if (block->IsFolder()) {
    LOG(INFO) << "[IPFS] BlockOrchestrator::ProcessBlock Folder";
    auto index_item =
        base::ranges::find_if(*block->GetLinks(), [](const auto& item) {
          return base::EqualsCaseInsensitiveASCII(item.name, "index.html");
        });
    if (index_item != block->GetLinks()->end()) {
      LOG(INFO) << "[IPFS] BlockOrchestrator::ProcessBlock Found index name:"
                << index_item->name << " hash:" << index_item->hash;
      ProcessBlock(index_item->hash);
    }
  } else {
    LOG(INFO) << "[IPFS] BlockOrchestrator::ProcessBlock NOT Found in the "
                 "CAR, must be requested";
    GURL sub_request_url(base::StringPrintf(
        "ipfs://%s",
        cid.c_str()));  // TODO change it to manage with replace URL parts
    SendRequest(sub_request_url);
  }

  //  if(target->IsPathTarget()) {
  // // TODO
  //  }
}

// void BlockOrchestrator::CreateDagPathIndex() {
//   ipfs::ipld::Block const* header_block = dag_nodes_[""].get();

//   DCHECK(header_block);
//   if (!header_block) {
//     return;
//   }

//   auto* root_cids = header_block->Meta().Find("roots");
//   DCHECK(root_cids);
//   if (!root_cids) {
//     return;
//   }
// }

bool BlockOrchestrator::IsActive() const {
  return !request_callback_.is_null() || block_reader_ || !dag_nodes_.empty();
}

void BlockOrchestrator::Reset() {
  request_callback_.Reset();
  block_reader_ = nullptr;  // TODO Can not reset from itself
  dag_nodes_.clear();
}

void BlockOrchestrator::BlockChainForCid(const uint64_t& size,
                                         Block const* block,
                                         bool last_chunk) const {
  // LOG(INFO) << "[IPFS] BlockOrchestrator::BlockChainForCid #10"
  //           << "\r\ncid:" << block->Cid()
  //           << "\r\nsize:" << size
  //           ;
  if (request_callback_ && block->IsContent()) {
    std::string_view vontent_view((const char*)block->GetContentData()->data(),
                                  block->GetContentData()->size());
    auto mime_type{mime_sniffer_->GetMime("", vontent_view, request_->url)};
    LOG(INFO) << "[IPFS] MIME type:" << mime_type.value_or("N/A");
    request_callback_.Run(nullptr,
                          std::make_unique<IpfsTrustlessResponse>(
                              mime_type.value_or(kDefaultMimeType), 200,
                              block->GetContentData(), "", size, last_chunk));
  }
}

}  // namespace ipfs::ipld
