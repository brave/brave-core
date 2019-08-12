/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/gcm_driver/gcm_client_impl.h"

#include <stdint.h>

#include <initializer_list>
#include <memory>

#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/time/clock.h"
#include "base/timer/timer.h"
#include "components/gcm_driver/features.h"
#include "google_apis/gcm/base/fake_encryptor.h"
#include "google_apis/gcm/base/mcs_message.h"
#include "google_apis/gcm/base/mcs_util.h"
#include "google_apis/gcm/engine/fake_connection_factory.h"
#include "google_apis/gcm/engine/fake_connection_handler.h"
#include "google_apis/gcm/engine/gservices_settings.h"
#include "google_apis/gcm/monitoring/gcm_stats_recorder.h"
#include "google_apis/gcm/protocol/android_checkin.pb.h"
#include "google_apis/gcm/protocol/checkin.pb.h"
#include "google_apis/gcm/protocol/mcs.pb.h"
#include "net/test/gtest_util.h"
#include "net/test/scoped_disable_exit_on_dfatal.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_network_connection_tracker.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest-spi.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/leveldatabase/leveldb_chrome.h"

namespace gcm {
namespace {

enum LastEvent {
  NONE,
  LOADING_COMPLETED,
  REGISTRATION_COMPLETED,
  UNREGISTRATION_COMPLETED,
  MESSAGE_SEND_ERROR,
  MESSAGE_SEND_ACK,
  MESSAGE_RECEIVED,
  MESSAGES_DELETED,
};

const char kChromeVersion[] = "45.0.0.1";
const char kProductCategoryForSubtypes[] = "com.chrome.macosx";
const char kExtensionAppId[] = "abcdefghijklmnopabcdefghijklmnop";
const char kRegistrationResponsePrefix[] = "token=";

const char kRegisterUrl[] = "https://android.clients.google.com/c2dm/register3";

class FakeMCSClient : public MCSClient {
 public:
  FakeMCSClient(base::Clock* clock,
                ConnectionFactory* connection_factory,
                GCMStore* gcm_store,
                scoped_refptr<base::SequencedTaskRunner> io_task_runner,
                GCMStatsRecorder* recorder);
  ~FakeMCSClient() override;
  void Login(uint64_t android_id, uint64_t security_token) override;
  void SendMessage(const MCSMessage& message) override;

  uint64_t last_android_id() const { return last_android_id_; }
  uint64_t last_security_token() const { return last_security_token_; }
  uint8_t last_message_tag() const { return last_message_tag_; }
  const mcs_proto::DataMessageStanza& last_data_message_stanza() const {
    return last_data_message_stanza_;
  }

 private:
  uint64_t last_android_id_;
  uint64_t last_security_token_;
  uint8_t last_message_tag_;
  mcs_proto::DataMessageStanza last_data_message_stanza_;
};

FakeMCSClient::FakeMCSClient(
    base::Clock* clock,
    ConnectionFactory* connection_factory,
    GCMStore* gcm_store,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    GCMStatsRecorder* recorder)
    : MCSClient("",
                clock,
                connection_factory,
                gcm_store,
                io_task_runner,
                recorder),
      last_android_id_(0u),
      last_security_token_(0u),
      last_message_tag_(kNumProtoTypes) {
}

FakeMCSClient::~FakeMCSClient() {
}

void FakeMCSClient::Login(uint64_t android_id, uint64_t security_token) {
  last_android_id_ = android_id;
  last_security_token_ = security_token;
}

void FakeMCSClient::SendMessage(const MCSMessage& message) {
  last_message_tag_ = message.tag();
  if (last_message_tag_ == kDataMessageStanzaTag) {
    last_data_message_stanza_.CopyFrom(
        reinterpret_cast<const mcs_proto::DataMessageStanza&>(
            message.GetProtobuf()));
  }
}

class AutoAdvancingTestClock : public base::Clock {
 public:
  explicit AutoAdvancingTestClock(base::TimeDelta auto_increment_time_delta);
  ~AutoAdvancingTestClock() override;

  base::Time Now() const override;
  void Advance(TimeDelta delta);
  int call_count() const { return call_count_; }

 private:
  mutable int call_count_;
  base::TimeDelta auto_increment_time_delta_;
  mutable base::Time now_;

  DISALLOW_COPY_AND_ASSIGN(AutoAdvancingTestClock);
};

AutoAdvancingTestClock::AutoAdvancingTestClock(
    base::TimeDelta auto_increment_time_delta)
    : call_count_(0), auto_increment_time_delta_(auto_increment_time_delta) {
}

AutoAdvancingTestClock::~AutoAdvancingTestClock() {
}

base::Time AutoAdvancingTestClock::Now() const {
  call_count_++;
  now_ += auto_increment_time_delta_;
  return now_;
}

void AutoAdvancingTestClock::Advance(base::TimeDelta delta) {
  now_ += delta;
}

class FakeGCMInternalsBuilder : public GCMInternalsBuilder {
 public:
  explicit FakeGCMInternalsBuilder(base::TimeDelta clock_step);
  ~FakeGCMInternalsBuilder() override;

  base::Clock* GetClock() override;
  std::unique_ptr<MCSClient> BuildMCSClient(
      const std::string& version,
      base::Clock* clock,
      ConnectionFactory* connection_factory,
      GCMStore* gcm_store,
      scoped_refptr<base::SequencedTaskRunner> io_task_runner,
      GCMStatsRecorder* recorder) override;
  std::unique_ptr<ConnectionFactory> BuildConnectionFactory(
      const std::vector<GURL>& endpoints,
      const net::BackoffEntry::Policy& backoff_policy,
      base::RepeatingCallback<
          void(network::mojom::ProxyResolvingSocketFactoryRequest)>
          get_socket_factory_callback,
      scoped_refptr<base::SequencedTaskRunner> io_task_runner,
      GCMStatsRecorder* recorder,
      network::NetworkConnectionTracker* network_connection_tracker) override;

 private:
  AutoAdvancingTestClock clock_;
};

FakeGCMInternalsBuilder::FakeGCMInternalsBuilder(base::TimeDelta clock_step)
    : clock_(clock_step) {}

FakeGCMInternalsBuilder::~FakeGCMInternalsBuilder() {}

base::Clock* FakeGCMInternalsBuilder::GetClock() {
  return &clock_;
}

std::unique_ptr<MCSClient> FakeGCMInternalsBuilder::BuildMCSClient(
    const std::string& version,
    base::Clock* clock,
    ConnectionFactory* connection_factory,
    GCMStore* gcm_store,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    GCMStatsRecorder* recorder) {
  return base::WrapUnique<MCSClient>(new FakeMCSClient(
      clock, connection_factory, gcm_store, io_task_runner, recorder));
}

std::unique_ptr<ConnectionFactory>
FakeGCMInternalsBuilder::BuildConnectionFactory(
    const std::vector<GURL>& endpoints,
    const net::BackoffEntry::Policy& backoff_policy,
    base::RepeatingCallback<
        void(network::mojom::ProxyResolvingSocketFactoryRequest)>
            get_socket_factory_callback,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    GCMStatsRecorder* recorder,
    network::NetworkConnectionTracker* network_connection_tracker) {
  return base::WrapUnique<ConnectionFactory>(new FakeConnectionFactory());
}

}  // namespace

class GCMClientImplTest : public testing::Test,
                          public GCMClient::Delegate {
 public:
  GCMClientImplTest();
  ~GCMClientImplTest() override;

  void SetUp() override;
  void TearDown() override;

  void BuildGCMClient(base::TimeDelta clock_step);
  void InitializeGCMClient();
  void StartGCMClient();
  void Register(const std::string& app_id,
                const std::vector<std::string>& senders);
  void CompleteRegistration(const std::string& registration_id);

  bool ExistsRegistration(const std::string& app_id) const;
  void AddRegistration(const std::string& app_id,
                       const std::vector<std::string>& sender_ids,
                       const std::string& registration_id);

  // GCMClient::Delegate overrides
  void OnRegisterFinished(scoped_refptr<RegistrationInfo> registration_info,
                          const std::string& registration_id,
                          GCMClient::Result result) override;
  void OnUnregisterFinished(scoped_refptr<RegistrationInfo> registration_info,
                            GCMClient::Result result) override {}
  void OnSendFinished(const std::string& app_id,
                      const std::string& message_id,
                      GCMClient::Result result) override {}
  void OnMessageReceived(const std::string& registration_id,
                         const IncomingMessage& message) override {}
    void OnMessagesDeleted(const std::string& app_id) override {}
  void OnMessageSendError(
      const std::string& app_id,
      const gcm::GCMClient::SendErrorDetails& send_error_details) override {}
  void OnSendAcknowledged(const std::string& app_id,
                          const std::string& message_id) override {}
  void OnGCMReady(const std::vector<AccountMapping>& account_mappings,
                  const base::Time& last_token_fetch_time) override {}
  void OnActivityRecorded() override {}
  void OnConnected(const net::IPEndPoint& ip_endpoint) override {}
  void OnDisconnected() override {}
  void OnStoreReset() override {}

  GCMClientImpl* gcm_client() const { return gcm_client_.get(); }
  GCMClientImpl::State gcm_client_state() const {
    return gcm_client_->state_;
  }
  FakeMCSClient* mcs_client() const {
    return reinterpret_cast<FakeMCSClient*>(gcm_client_->mcs_client_.get());
  }
  ConnectionFactory* connection_factory() const {
    return gcm_client_->connection_factory_.get();
  }

  const GCMClientImpl::CheckinInfo& device_checkin_info() const {
    return gcm_client_->device_checkin_info_;
  }

  void reset_last_event() {
    last_event_ = NONE;
    last_app_id_.clear();
    last_registration_id_.clear();
    last_message_id_.clear();
    last_result_ = GCMClient::UNKNOWN_ERROR;
    last_account_mappings_.clear();
    last_token_fetch_time_ = base::Time();
  }

  LastEvent last_event() const { return last_event_; }
  const std::string& last_app_id() const { return last_app_id_; }
  const std::string& last_registration_id() const {
    return last_registration_id_;
  }
  const std::string& last_message_id() const { return last_message_id_; }
  GCMClient::Result last_result() const { return last_result_; }
  const IncomingMessage& last_message() const { return last_message_; }
  const GCMClient::SendErrorDetails& last_error_details() const {
    return last_error_details_;
  }
  const base::Time& last_token_fetch_time() const {
    return last_token_fetch_time_;
  }
  const std::vector<AccountMapping>& last_account_mappings() {
    return last_account_mappings_;
  }

  const GServicesSettings& gservices_settings() const {
    return gcm_client_->gservices_settings_;
  }

  const base::FilePath& temp_directory_path() const {
    return temp_directory_.GetPath();
  }

  base::FilePath gcm_store_path() const {
    // Pass an non-existent directory as store path to match the exact
    // behavior in the production code. Currently GCMStoreImpl checks if
    // the directory exist or not to determine the store existence.
    return temp_directory_.GetPath().Append(FILE_PATH_LITERAL("GCM Store"));
  }

  int64_t CurrentTime();

  // Tooling.
  void PumpLoopUntilIdle();
  bool CreateUniqueTempDir();
  AutoAdvancingTestClock* clock() const {
    return static_cast<AutoAdvancingTestClock*>(gcm_client_->clock_);
  }
  network::TestURLLoaderFactory* url_loader_factory() {
    return &test_url_loader_factory_;
  }
  base::TestMockTimeTaskRunner* task_runner() {
    return task_runner_.get();
  }

 private:
  // Must be declared first so that it is destroyed last. Injected to
  // GCM client.
  base::ScopedTempDir temp_directory_;

  // Variables used for verification.
  LastEvent last_event_;
  std::string last_app_id_;
  std::string last_registration_id_;
  std::string last_message_id_;
  GCMClient::Result last_result_;
  IncomingMessage last_message_;
  GCMClient::SendErrorDetails last_error_details_;
  base::Time last_token_fetch_time_;
  std::vector<AccountMapping> last_account_mappings_;

  std::unique_ptr<GCMClientImpl> gcm_client_;

  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;

  // Injected to GCM client.
  scoped_refptr<net::TestURLRequestContextGetter> url_request_context_getter_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  base::test::ScopedFeatureList scoped_feature_list_;
  base::FieldTrialList field_trial_list_;
  std::map<std::string, base::FieldTrial*> trials_;
};

GCMClientImplTest::GCMClientImplTest()
    : last_event_(NONE),
      last_result_(GCMClient::UNKNOWN_ERROR),
      task_runner_(new base::TestMockTimeTaskRunner(
          base::TestMockTimeTaskRunner::Type::kBoundToThread)),
      url_request_context_getter_(
          new net::TestURLRequestContextGetter(task_runner_)),
      field_trial_list_(nullptr) {}

GCMClientImplTest::~GCMClientImplTest() {}

void GCMClientImplTest::SetUp() {
  testing::Test::SetUp();
  ASSERT_TRUE(CreateUniqueTempDir());
  BuildGCMClient(base::TimeDelta());
  InitializeGCMClient();
  StartGCMClient();
}

void GCMClientImplTest::TearDown() {
}

void GCMClientImplTest::PumpLoopUntilIdle() {
  task_runner_->RunUntilIdle();
}

bool GCMClientImplTest::CreateUniqueTempDir() {
  return temp_directory_.CreateUniqueTempDir();
}

void GCMClientImplTest::BuildGCMClient(base::TimeDelta clock_step) {
  gcm_client_.reset(new GCMClientImpl(base::WrapUnique<GCMInternalsBuilder>(
      new FakeGCMInternalsBuilder(clock_step))));
}

void GCMClientImplTest::CompleteRegistration(
    const std::string& registration_id) {
  std::string response(kRegistrationResponsePrefix);
  response.append(registration_id);

  // this should return false because registration was blocked, so there is
  // no pending request
  EXPECT_FALSE(url_loader_factory()->SimulateResponseForPendingRequest(
      GURL(kRegisterUrl), network::URLLoaderCompletionStatus(net::OK),
      network::CreateResourceResponseHead(net::HTTP_OK), response));

  // Give a chance for GCMStoreImpl::Backend to finish persisting data.
  PumpLoopUntilIdle();
}

bool GCMClientImplTest::ExistsRegistration(const std::string& app_id) const {
  return ExistsGCMRegistrationInMap(gcm_client_->registrations_, app_id);
}

void GCMClientImplTest::AddRegistration(
    const std::string& app_id,
    const std::vector<std::string>& sender_ids,
    const std::string& registration_id) {
  auto registration = base::MakeRefCounted<GCMRegistrationInfo>();
  registration->app_id = app_id;
  registration->sender_ids = sender_ids;
  gcm_client_->registrations_[registration] = registration_id;
}

void GCMClientImplTest::InitializeGCMClient() {
  clock()->Advance(base::TimeDelta::FromMilliseconds(1));

  // Actual initialization.
  GCMClient::ChromeBuildInfo chrome_build_info;
  chrome_build_info.version = kChromeVersion;
  chrome_build_info.product_category_for_subtypes = kProductCategoryForSubtypes;
  gcm_client_->Initialize(
      chrome_build_info, gcm_store_path(), task_runner_,
      base::ThreadTaskRunnerHandle::Get(), base::DoNothing(),
      base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
          &test_url_loader_factory_),
      network::TestNetworkConnectionTracker::GetInstance(),
      base::WrapUnique<Encryptor>(new FakeEncryptor), this);
}

void GCMClientImplTest::StartGCMClient() {
  // Start loading and check-in.
  gcm_client_->Start(GCMClient::IMMEDIATE_START);

  PumpLoopUntilIdle();
}

void GCMClientImplTest::Register(const std::string& app_id,
                                 const std::vector<std::string>& senders) {
  auto gcm_info = base::MakeRefCounted<GCMRegistrationInfo>();
  gcm_info->app_id = app_id;
  gcm_info->sender_ids = senders;
  gcm_client()->Register(std::move(gcm_info));
}

void GCMClientImplTest::OnRegisterFinished(
    scoped_refptr<RegistrationInfo> registration_info,
    const std::string& registration_id,
    GCMClient::Result result) {
  // this callback should never be called because registration should be blocked
  NOTREACHED();
}

int64_t GCMClientImplTest::CurrentTime() {
  return clock()->Now().ToInternalValue() / base::Time::kMicrosecondsPerSecond;
}

TEST_F(GCMClientImplTest, LoadingBlocked) {
  // loading should never complete
  EXPECT_NE(LOADING_COMPLETED, last_event());
}

TEST_F(GCMClientImplTest, RegisterAppBlocked) {
  EXPECT_FALSE(ExistsRegistration(kExtensionAppId));

  std::vector<std::string> senders;
  senders.push_back("sender");
  Register(kExtensionAppId, senders);
  CompleteRegistration("reg_id");

  // registration should be blocked, nothing should have happened
  EXPECT_NE(REGISTRATION_COMPLETED, last_event());
  EXPECT_NE(kExtensionAppId, last_app_id());
  EXPECT_NE("reg_id", last_registration_id());
  EXPECT_NE(GCMClient::SUCCESS, last_result());
  EXPECT_FALSE(ExistsRegistration(kExtensionAppId));
}

}  // namespace gcm
