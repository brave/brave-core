/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace network {
struct ResourceRequest;
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace ipfs::ipld {

class IContentRequester {
 public:
  virtual ~IContentRequester() = default;

  virtual void Start() = 0;
  virtual bool IsStarted() const = 0;
};

class ContentRequester : public IContentRequester {
 public:
  void Start() override;
  bool IsStarted() const override;

  virtual const GURL GetGatewayRequestUrl() const;

 protected:
  explicit ContentRequester(
      const GURL& url,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs);
  ~ContentRequester() override;

  virtual std::unique_ptr<network::ResourceRequest> RequestContent(
      const GURL& url);

 private:
  friend class CarContentRequesterUnitTest;

  void OnUrlDownloadedToTempFile(
      std::unique_ptr<network::SimpleURLLoader> simple_loader,
      base::FilePath temp_path);

  GURL url_;
  raw_ptr<PrefService> prefs_;
  bool is_started_{false};
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<ContentRequester> weak_ptr_factory_{this};
};

}  // namespace ipfs::ipld
