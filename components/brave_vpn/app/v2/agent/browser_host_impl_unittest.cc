/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_host_impl.h"

#include <memory>
#include <utility>

#include "base/functional/callback.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser.h"
#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser_endpoint.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {

// Bounded barrier for asserting that something did not happen: a round trip on
// a throwaway pipe bound to this sequence completes only after the work already
// queued on the sequence has run. Needed when neither session pipe survives to
// be flushed directly.
void FlushSequence() {
  FakeBrowserEndpoint endpoint_impl;
  mojo::Remote<mojom::BrowserEndpoint> remote;
  mojo::Receiver<mojom::BrowserEndpoint> receiver(
      &endpoint_impl, remote.BindNewPipeAndPassReceiver());
  remote.FlushForTesting();
}

}  // namespace

class BrowserHostImplTest : public testing::Test {
 protected:
  void CreateHost() { CreateHost(disconnected_.GetCallback()); }
  void CreateHost(base::OnceClosure disconnect_handler) {
    host_ = std::make_unique<BrowserHostImpl>(browser_.BindEndpoint(),
                                              browser_.BindHost(),
                                              std::move(disconnect_handler));
    ASSERT_TRUE(host_);
  }

  base::test::TaskEnvironment task_environment_;
  FakeBrowser browser_;
  std::unique_ptr<BrowserHostImpl> host_;
  base::test::TestFuture<void> disconnected_;
};

TEST_F(BrowserHostImplTest, StaysAliveWhileBothPipesAreOpen) {
  CreateHost();
  browser_.FlushHost();
  EXPECT_FALSE(disconnected_.IsReady());
}

TEST_F(BrowserHostImplTest, DroppingHostReportsDisconnect) {
  CreateHost();
  browser_.DropHost();
  EXPECT_TRUE(disconnected_.Wait());
}

// A browser that drops only its endpoint can no longer be called back, so the
// session must end even though the BrowserHost pipe is still open.
TEST_F(BrowserHostImplTest, DroppingEndpointReportsDisconnect) {
  CreateHost();
  browser_.DropEndpoint();
  EXPECT_TRUE(disconnected_.Wait());
}

// Both pipes closing must still report exactly once, since the report is what
// tears the session down. The handler here deliberately keeps the host alive,
// so the surviving pipe's handler really does run and find the report consumed.
TEST_F(BrowserHostImplTest, ReportsDisconnectOnceWhenBothPipesClose) {
  int report_count = 0;
  base::RunLoop first_report;
  CreateHost(base::BindLambdaForTesting([&] {
    ++report_count;
    first_report.Quit();
  }));

  browser_.DropEndpoint();
  first_report.Run();
  ASSERT_EQ(1, report_count);

  browser_.DropHost();
  FlushSequence();

  EXPECT_EQ(1, report_count);
}

// The registry destroys sessions itself when a connection goes away, and must
// not get a report back for a host it has already deleted.
TEST_F(BrowserHostImplTest, DestructionDoesNotReportDisconnect) {
  CreateHost();
  browser_.WatchHost();

  host_.reset();

  // The browser observing its own end close proves the teardown has been
  // processed, so the report either happened by now or never will.
  EXPECT_TRUE(browser_.WaitForHostClosed());
  EXPECT_FALSE(disconnected_.IsReady());
}

}  // namespace brave_vpn::v2
