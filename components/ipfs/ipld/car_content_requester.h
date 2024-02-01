/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/content_requester.h"

namespace ipfs::ipld {

class CarContentRequester : public ContentRequester {
  friend class CarContentRequesterUnitTest;

 public:
  explicit CarContentRequester(
      const GURL& url,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs);
  ~CarContentRequester() override;

  const GURL GetGatewayRequestUrl() const override;
};

}  // namespace ipfs::ipld
