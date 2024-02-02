/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPLD_CONTENT_REQUESTER_H_
#define BRAVE_COMPONENTS_IPFS_IPLD_CONTENT_REQUESTER_H_

#include <cstdint>
#include <memory>
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "url/gurl.h"

namespace network {
struct ResourceRequest;
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace ipfs::ipld {

using ContentRequestBufferCallback =
    base::RepeatingCallback<void(std::unique_ptr<std::vector<uint8_t>>, const bool)>;

class IContentRequester {
 public:
  virtual ~IContentRequester() = default;

  virtual void Request(ContentRequestBufferCallback callback) = 0;
  virtual bool IsStarted() const = 0;
};

class ContentRequester : public IContentRequester,
                         network::SimpleURLLoaderStreamConsumer {
 public:
  void Request(ContentRequestBufferCallback callback) override;
  bool IsStarted() const override;

 protected:
  explicit ContentRequester(
      const GURL& url,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs);
  ~ContentRequester() override;

  virtual const GURL GetGatewayRequestUrl() const;
  virtual std::unique_ptr<network::SimpleURLLoader> CreateLoader() const = 0;

 private:
  friend class CarContentRequesterUnitTest;

  // network::SimpleURLLoaderStreamConsumer implementations.
  void OnDataReceived(base::StringPiece string_piece,
                      base::OnceClosure resume) override;
  void OnRetry(base::OnceClosure start_retry) override;
  void OnComplete(bool success) override;

  GURL url_;
  std::unique_ptr<std::vector<uint8_t>> data_;
  ContentRequestBufferCallback buffer_ready_callback_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  raw_ptr<PrefService> prefs_;
  bool is_started_{false};
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<ContentRequester> weak_ptr_factory_{this};
};

class ContentReaderFactory {
 public:
  ContentReaderFactory() = default;
  ~ContentReaderFactory() = default;
  ContentReaderFactory(const ContentReaderFactory&) = delete;
  ContentReaderFactory(ContentReaderFactory&&) = delete;
  ContentReaderFactory& operator=(const ContentReaderFactory&) = delete;
  ContentReaderFactory& operator=(ContentReaderFactory&&) = delete;

  std::unique_ptr<IContentRequester> CreateCarContentRequester(
      const GURL& url,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs,
      const bool only_structure = true);
};

}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_CONTENT_REQUESTER_H_
