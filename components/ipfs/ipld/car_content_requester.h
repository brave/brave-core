/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/content_requester.h"

namespace ipfs::ipld {

class CarContentRequester : public ContentRequester {
 public:
  explicit CarContentRequester(
      const GURL& url,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs,
      const bool only_structure = true);
  ~CarContentRequester() override;

  const GURL GetGatewayRequestUrl() const override;

  std::unique_ptr<network::SimpleURLLoader> CreateLoader() const override;

  private:
    friend class CarContentRequesterUnitTest;
    bool only_structure_;
};

}  // namespace ipfs::ipld
