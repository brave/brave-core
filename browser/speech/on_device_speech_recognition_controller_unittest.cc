/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_recognition_controller.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speech {

namespace {

// Renderer-side worker stand-in. Records how often the browser loaded the
// model (Init) and manufactured a stream (CreateAsrStream), and lets a test
// force a load failure or sever the pipe.
class FakeSpeechRecognitionFactory
    : public local_ai::mojom::SpeechRecognitionFactory {
 public:
  FakeSpeechRecognitionFactory() = default;
  ~FakeSpeechRecognitionFactory() override = default;

  // local_ai::mojom::SpeechRecognitionFactory:
  void Init(local_ai::mojom::OrtModelFilesPtr files,
            InitCallback callback) override {
    ++init_count_;
    std::move(callback).Run(init_success_);
  }
  void CreateAsrStream(
      on_device_model::mojom::AsrStreamOptionsPtr options,
      mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
      mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder)
      override {
    ++create_count_;
  }

  mojo::PendingRemote<local_ai::mojom::SpeechRecognitionFactory> BindRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }
  void ResetReceiver() { receiver_.reset(); }

  void set_init_success(bool success) { init_success_ = success; }
  int init_count() const { return init_count_; }
  int create_count() const { return create_count_; }

 private:
  bool init_success_ = true;
  int init_count_ = 0;
  int create_count_ = 0;
  mojo::Receiver<local_ai::mojom::SpeechRecognitionFactory> receiver_{this};
};

// Stand-in for the worker's BackgroundWebContents. Needs no renderer; runs an
// on-destroyed closure so the fixture can observe teardown, and can simulate an
// unexpected renderer loss.
class FakeBackgroundWebContents : public local_ai::BackgroundWebContents {
 public:
  FakeBackgroundWebContents(Delegate* delegate, base::OnceClosure on_destroyed)
      : delegate_(delegate), on_destroyed_(std::move(on_destroyed)) {}
  ~FakeBackgroundWebContents() override { std::move(on_destroyed_).Run(); }

  void SimulateDestroyed() {
    delegate_->OnBackgroundContentsDestroyed(DestroyReason::kRendererGone);
  }

 private:
  raw_ptr<Delegate> delegate_;
  base::OnceClosure on_destroyed_;
};

}  // namespace

class OnDeviceSpeechRecognitionControllerTest : public testing::Test {
 protected:
  OnDeviceSpeechRecognitionControllerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  // One recognition's worth of pipes, kept alive for the test's duration.
  struct Session {
    Session() = default;
    Session(Session&&) = default;
    Session& operator=(Session&&) = default;
    ~Session() = default;

    mojo::Remote<local_ai::mojom::AsrSession> session;
    mojo::Remote<on_device_model::mojom::AsrStreamInput> input;
    mojo::PendingReceiver<on_device_model::mojom::AsrStreamResponder>
        responder_receiver;
  };

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    // In production the worker always runs in a non-null guest OTR profile.
    // Provide one by default so the boot flow reaches WorkerStarting; the
    // profile-destruction test reuses it.
    next_otr_profile_ = &profile_;
    controller_ = OnDeviceSpeechRecognitionController::CreateForTesting(
        base::BindRepeating(
            &OnDeviceSpeechRecognitionControllerTest::CreateFakeBwc,
            base::Unretained(this)));
  }

  void TearDown() override {
    controller_.reset();
    // The models state is a process-wide singleton, so clear it to avoid
    // polluting other suites.
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        base::FilePath());
  }

  // Injected BackgroundWebContents factory: builds a fake synchronously and
  // reports it back with `next_otr_profile_`.
  void CreateFakeBwc(
      base::WeakPtr<local_ai::BackgroundWebContents::Delegate> delegate,
      local_ai::BackgroundWebContentsCreatedCallback created) {
    ++bwc_created_count_;
    if (capture_created_) {
      // Hold the reply + delegate so a test can deliver a stale reply after a
      // teardown/restart, without completing this cycle's creation.
      captured_delegate_ = std::move(delegate);
      captured_created_ = std::move(created);
      return;
    }
    if (bwc_creation_fails_) {
      // Production builds the worker asynchronously (guest profile creation),
      // so the failure is reported on a later turn, not inline. Match that so
      // Start() finishes queuing before the teardown happens.
      base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE,
          base::BindOnce(std::move(created),
                         std::unique_ptr<local_ai::BackgroundWebContents>(),
                         nullptr));
      return;
    }
    auto bwc = std::make_unique<FakeBackgroundWebContents>(
        delegate.get(),
        base::BindOnce(
            [](raw_ptr<FakeBackgroundWebContents>* ref) { *ref = nullptr; },
            &last_bwc_));
    last_bwc_ = bwc.get();
    std::move(created).Run(std::move(bwc), next_otr_profile_);
  }

  void SetInstalled(bool installed) {
    auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();
    if (!installed) {
      state->SetInstallDir(base::FilePath());
      return;
    }
    state->SetInstallDir(temp_dir_.GetPath());
    base::FilePath model_dir = state->GetModelDir();
    ASSERT_TRUE(base::CreateDirectory(model_dir));
    // ReadNemotronOrtFiles only needs these to exist; contents are ignored by
    // the fake factory's Init.
    for (const char* name :
         {"encoder.onnx", "encoder.onnx.data", "decoder_joint.onnx",
          "decoder_joint.onnx.data", "filterbank.bin", "tokens.txt"}) {
      ASSERT_TRUE(base::WriteFile(model_dir.AppendASCII(name), "x"));
    }
  }

  // Acts as the engine: takes an AsrSession and starts one recognition.
  Session StartSession() {
    Session s;
    s.session.Bind(controller_->GetAsrSession());
    mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder;
    s.responder_receiver = responder.InitWithNewPipeAndPassReceiver();
    s.session->Start(on_device_model::mojom::AsrStreamOptions::New(),
                     s.input.BindNewPipeAndPassReceiver(),
                     std::move(responder));
    s.session.FlushForTesting();
    return s;
  }

  // Acts as the worker WebUI: registers a factory so the browser can load the
  // model. Does not wait for the load to finish.
  void RegisterFactory() {
    auto host = std::make_unique<
        mojo::Remote<local_ai::mojom::SpeechRecognitionFactoryHost>>();
    controller_->BindFactoryHost(host->BindNewPipeAndPassReceiver());
    (*host)->RegisterFactory(fake_factory_.BindRemote());
    host->FlushForTesting();
    factory_hosts_.push_back(std::move(host));
  }

  // Starts one session and drives the worker all the way to Ready, returning
  // the (now-forwarded) session.
  Session DriveToReady() {
    SetInstalled(true);
    Session s = StartSession();
    RegisterFactory();
    EXPECT_TRUE(base::test::RunUntil(
        [&] { return fake_factory_.create_count() >= 1; }));
    return s;
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  base::ScopedTempDir temp_dir_;
  FakeSpeechRecognitionFactory fake_factory_;
  raw_ptr<FakeBackgroundWebContents> last_bwc_ = nullptr;
  // Profile the injected factory reports as the worker's OTR profile. Set to a
  // real profile in SetUp() so the boot flow reaches WorkerStarting.
  raw_ptr<Profile> next_otr_profile_ = nullptr;
  // When set, the injected factory reports an async creation failure.
  bool bwc_creation_fails_ = false;
  // When set, CreateFakeBwc captures the reply + delegate instead of completing
  // creation, so a test can deliver a stale reply after a teardown/restart.
  bool capture_created_ = false;
  base::WeakPtr<local_ai::BackgroundWebContents::Delegate> captured_delegate_;
  local_ai::BackgroundWebContentsCreatedCallback captured_created_;
  int bwc_created_count_ = 0;
  std::vector<std::unique_ptr<
      mojo::Remote<local_ai::mojom::SpeechRecognitionFactoryHost>>>
      factory_hosts_;
  std::unique_ptr<OnDeviceSpeechRecognitionController> controller_;
};

TEST_F(OnDeviceSpeechRecognitionControllerTest,
       StartCreatesBackgroundContents) {
  SetInstalled(true);
  Session s = StartSession();
  EXPECT_NE(nullptr, last_bwc_.get());
  // Not yet ready: nothing forwarded to the worker.
  EXPECT_EQ(0, fake_factory_.create_count());
}

TEST_F(OnDeviceSpeechRecognitionControllerTest,
       ConcurrentStartsQueueSingleWorker) {
  SetInstalled(true);
  Session s1 = StartSession();
  Session s2 = StartSession();
  // Both starts share one worker.
  EXPECT_EQ(1, bwc_created_count_);

  RegisterFactory();
  EXPECT_TRUE(
      base::test::RunUntil([&] { return fake_factory_.create_count() >= 2; }));
  EXPECT_EQ(2, fake_factory_.create_count());
}

TEST_F(OnDeviceSpeechRecognitionControllerTest,
       BackgroundContentsDestroyedTearsDown) {
  Session s = DriveToReady();
  ASSERT_NE(nullptr, last_bwc_.get());

  // Renderer loss is reported through the delegate; TearDown is deferred off
  // the observer callback, so pump the loop.
  last_bwc_->SimulateDestroyed();
  EXPECT_TRUE(base::test::RunUntil([&] { return last_bwc_.get() == nullptr; }));
}

TEST_F(OnDeviceSpeechRecognitionControllerTest, OtrProfileDestroyedTearsDown) {
  // The controller observes the worker's OTR profile so it tears the worker
  // down before that profile is destroyed (otherwise the WebContents would
  // outlive its BrowserContext).
  Session s = DriveToReady();
  ASSERT_NE(nullptr, last_bwc_.get());

  // Fire the observer callback the profile sends just before destruction.
  // Driving real profile destruction would drag in async teardown machinery;
  // invoking the callback directly exercises the controller's response. It
  // tears down only because the controller stored and observed this profile
  // (OnProfileWillBeDestroyed early-returns for any other profile).
  controller_->OnProfileWillBeDestroyed(next_otr_profile_);
  EXPECT_EQ(nullptr, last_bwc_.get());
}

TEST_F(OnDeviceSpeechRecognitionControllerTest,
       BackgroundContentsCreationFailureTearsDown) {
  SetInstalled(true);
  bwc_creation_fails_ = true;

  Session s = StartSession();
  // The async creation failure tears the worker down, dropping the queued
  // session (its audio pipe disconnects).
  EXPECT_TRUE(base::test::RunUntil([&] { return !s.input.is_connected(); }));
  EXPECT_EQ(nullptr, last_bwc_.get());

  // The controller is back to idle: a later attempt succeeds once creation
  // works again.
  bwc_creation_fails_ = false;
  Session s2 = StartSession();
  EXPECT_NE(nullptr, last_bwc_.get());
}

TEST_F(OnDeviceSpeechRecognitionControllerTest,
       ReachesReadyAndForwardsPendingSession) {
  Session s = DriveToReady();
  EXPECT_EQ(1, fake_factory_.init_count());
  EXPECT_EQ(1, fake_factory_.create_count());
}

TEST_F(OnDeviceSpeechRecognitionControllerTest, InitFailureTearsDown) {
  SetInstalled(true);
  fake_factory_.set_init_success(false);

  Session s = StartSession();
  RegisterFactory();

  // A failed load tears the worker down.
  EXPECT_TRUE(base::test::RunUntil([&] { return last_bwc_.get() == nullptr; }));
  EXPECT_EQ(0, fake_factory_.create_count());

  // A later Start recreates the worker.
  Session s2 = StartSession();
  EXPECT_NE(nullptr, last_bwc_.get());
  EXPECT_EQ(2, bwc_created_count_);
}

TEST_F(OnDeviceSpeechRecognitionControllerTest, FactoryDisconnectTearsDown) {
  Session s = DriveToReady();
  ASSERT_NE(nullptr, last_bwc_.get());

  fake_factory_.ResetReceiver();
  EXPECT_TRUE(base::test::RunUntil([&] { return last_bwc_.get() == nullptr; }));

  // The controller is back to idle and a new Start restarts cleanly.
  Session s2 = StartSession();
  EXPECT_NE(nullptr, last_bwc_.get());
}

TEST_F(OnDeviceSpeechRecognitionControllerTest, ModelFileReadFailureTearsDown) {
  // Install dir is set (so Start proceeds) but the model files are absent, so
  // ReadNemotronOrtFiles returns null and OnOrtFilesRead tears the worker down
  // without Init-ing the factory.
  local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
      temp_dir_.GetPath());
  Session s = StartSession();
  ASSERT_NE(nullptr, last_bwc_.get());
  RegisterFactory();

  EXPECT_TRUE(base::test::RunUntil([&] { return last_bwc_.get() == nullptr; }));
  EXPECT_EQ(0, fake_factory_.init_count());
}

// The worker environment comes up but the worker never registers its factory.
// The startup timer (kStartupTimeout, 30s) tears it down and drops the queued
// session so the engine sees a failure instead of hanging.
TEST_F(OnDeviceSpeechRecognitionControllerTest,
       StartupTimeoutTearsDownWhenFactoryNeverRegisters) {
  SetInstalled(true);
  Session s = StartSession();
  ASSERT_NE(nullptr, last_bwc_.get());

  // No RegisterFactory(). Advance past kStartupTimeout so the startup timer
  // fires.
  task_environment_.FastForwardBy(base::Seconds(31));
  EXPECT_EQ(nullptr, last_bwc_.get());
  // The queued session's audio pipe is dropped, surfacing as a failure.
  EXPECT_TRUE(base::test::RunUntil([&] { return !s.input.is_connected(); }));

  // Back to idle: a later attempt starts a fresh worker.
  Session s2 = StartSession();
  EXPECT_NE(nullptr, last_bwc_.get());
  EXPECT_EQ(2, bwc_created_count_);
}

// A worker that reached Ready must survive past kStartupTimeout.
// RegisterFactory stops the startup timer, and OnStartupTimeout's state guard
// would ignore a stale fire regardless; this pins the net behavior. Assert the
// worker is not just alive but still Ready by forwarding a new session to it.
TEST_F(OnDeviceSpeechRecognitionControllerTest,
       ReadyWorkerSurvivesStartupTimeout) {
  Session s = DriveToReady();
  ASSERT_NE(nullptr, last_bwc_.get());
  ASSERT_EQ(1, fake_factory_.create_count());

  task_environment_.FastForwardBy(base::Seconds(31));
  ASSERT_NE(nullptr, last_bwc_.get());

  // Still Ready and serving: a new session forwards to the same live worker,
  // no new worker created.
  Session s2 = StartSession();
  EXPECT_TRUE(
      base::test::RunUntil([&] { return fake_factory_.create_count() >= 2; }));
  EXPECT_EQ(1, bwc_created_count_);
}

// A creation reply from a torn-down cycle must not land in a later cycle and
// adopt a stale worker. TearDown()'s InvalidateWeakPtrs() drops the reply; the
// state check alone is insufficient because the later cycle is also
// kBwcStarting.
TEST_F(OnDeviceSpeechRecognitionControllerTest,
       StaleCreationReplyIgnoredAfterRestart) {
  SetInstalled(true);
  capture_created_ = true;

  // Cycle 1: capture its (weak-bound) creation reply, then take it aside before
  // the next cycle overwrites the captured slot.
  Session s1 = StartSession();
  auto stale_created = std::move(captured_created_);
  auto stale_delegate = captured_delegate_;

  // The startup timer tears cycle 1 down, invalidating the captured reply.
  task_environment_.FastForwardBy(base::Seconds(31));

  // Cycle 2: back in kBwcStarting (its reply is captured, not run).
  Session s2 = StartSession();

  // Deliver the stale cycle-1 reply. It must be ignored (its weak pointer was
  // invalidated), so the worker it carries is dropped, not adopted into cycle
  // 2. Were it adopted, the controller would hold the fake and it would stay
  // alive.
  raw_ptr<FakeBackgroundWebContents> stale_bwc = nullptr;
  auto bwc = std::make_unique<FakeBackgroundWebContents>(
      stale_delegate.get(),
      base::BindOnce(
          [](raw_ptr<FakeBackgroundWebContents>* ref) { *ref = nullptr; },
          &stale_bwc));
  stale_bwc = bwc.get();
  std::move(stale_created).Run(std::move(bwc), next_otr_profile_);
  EXPECT_EQ(nullptr, stale_bwc);
}

}  // namespace speech
