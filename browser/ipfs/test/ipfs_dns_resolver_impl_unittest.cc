/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_dns_resolver_impl.h"

#include <memory>
#include <utility>

#include "content/public/browser/network_service_instance.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ipfs {

class IpfsDnsResolverImplUnitTest : public testing::Test {
 public:
  IpfsDnsResolverImplUnitTest() = default;
  IpfsDnsResolverImplUnitTest(const IpfsDnsResolverImplUnitTest&) = delete;
  IpfsDnsResolverImplUnitTest& operator=(const IpfsDnsResolverImplUnitTest&) =
      delete;
  ~IpfsDnsResolverImplUnitTest() override = default;

  void SetUp() override {}

  void TearDown() override {}
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(IpfsDnsResolverImplUnitTest, ReconnectOnMojoError) {
  std::unique_ptr<IpfsDnsResolverImpl> ipfs_dns_resolver_impl_ =
      std::make_unique<IpfsDnsResolverImpl>();
  ipfs_dns_resolver_impl_->receiver_.reset();
  ipfs_dns_resolver_impl_->OnDnsConfigChangeManagerConnectionError();
  EXPECT_FALSE(ipfs_dns_resolver_impl_->receiver_.is_bound());
  {
    base::RunLoop loop;
    while (!ipfs_dns_resolver_impl_->receiver_.is_bound()) {
      loop.RunUntilIdle();
    }
  }
}

}  // namespace ipfs
