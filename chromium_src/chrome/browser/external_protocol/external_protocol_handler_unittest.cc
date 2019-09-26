#include "chrome/browser/external_protocol/external_protocol_handler.h"

#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class ExternalProtocolHandlerTest : public testing::Test {
 protected:
  void SetUp() override {
    profile_.reset(new TestingProfile());
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
  }

  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(ExternalProtocolHandlerTest, TestGetBlockStateMailto) {
  ExternalProtocolHandler::BlockState block_state =
      ExternalProtocolHandler::GetBlockState("mailto", profile_.get());
  EXPECT_EQ(ExternalProtocolHandler::UNKNOWN, block_state);
}
