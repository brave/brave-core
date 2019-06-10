#include <stdint.h>

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/ptr_util.h"
#include "components/invalidation/impl/push_client_channel.h"
#include "jingle/notifier/listener/fake_push_client.h"
#include "jingle/notifier/listener/notification_defines.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {
namespace {

class PushClientChannelTest : public ::testing::Test,
                              public SyncNetworkChannel::Observer {
 protected:
  PushClientChannelTest()
      : fake_push_client_(new notifier::FakePushClient()),
        push_client_channel_(base::WrapUnique(fake_push_client_)) {
    push_client_channel_.AddObserver(this);
    push_client_channel_.SetMessageReceiver(invalidation::NewPermanentCallback(
        this, &PushClientChannelTest::OnIncomingMessage));
    push_client_channel_.SetSystemResources(nullptr);
  }

  ~PushClientChannelTest() override {
    push_client_channel_.RemoveObserver(this);
  }

  void OnNetworkChannelStateChanged(
      InvalidatorState invalidator_state) override {
    NOTREACHED();
  }

  void OnIncomingMessage(std::string incoming_message) { NOTREACHED(); }

  notifier::FakePushClient* fake_push_client_;
  PushClientChannel push_client_channel_;
};

const char kMessage[] = "message";
const char kServiceContext[] = "service context";
const int64_t kSchedulingHash = 100;

// Simulate an incoming notification. Nothing should happen because
// the channel should not be listening.
TEST_F(PushClientChannelTest, OnIncomingMessage) {
  notifier::Notification notification;
  notification.data = PushClientChannel::EncodeMessageForTest(
      kMessage, kServiceContext, kSchedulingHash);
  fake_push_client_->SimulateIncomingNotification(notification);
}

}  // namespace
}  // namespace syncer
