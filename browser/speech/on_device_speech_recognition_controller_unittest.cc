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
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
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
    if (on_call_) {
      on_call_.Run();
    }
    std::move(callback).Run(init_success_);
  }
  void CreateAsrStream(
      on_device_model::mojom::AsrStreamOptionsPtr options,
      mojo::PendingReceiver<on_device_model::mojom::AsrStreamInput> stream,
      mojo::PendingRemote<on_device_model::mojom::AsrStreamResponder> responder)
      override {
    ++created_asr_stream_count_;
    if (on_call_) {
      on_call_.Run();
    }
  }

  mojo::PendingRemote<local_ai::mojom::SpeechRecognitionFactory> BindRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }
  void ResetReceiver() { receiver_.reset(); }

  void set_init_success(bool success) { init_success_ = success; }
  int init_count() const { return init_count_; }
  int created_asr_stream_count() const { return created_asr_stream_count_; }

  // Runs on every Init/CreateAsrStream so a test can quit a RunLoop once the
  // counts reach what it is waiting for.
  void set_on_call(base::RepeatingClosure on_call) {
    on_call_ = std::move(on_call);
  }

 private:
  bool init_success_ = true;
  int init_count_ = 0;
  int created_asr_stream_count_ = 0;
  base::RepeatingClosure on_call_;
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

  // Injected BackgroundWebContents factory. Three modes drive the states the
  // production async pipeline produces: synchronous success, an async creation
  // failure, and a captured (deferred) reply for stale-reply tests.
  void CreateFakeBwc(
      base::WeakPtr<local_ai::BackgroundWebContents::Delegate> delegate,
      local_ai::BackgroundWebContentsCreatedCallback created) {
    ++bwc_created_count_;
    if (capture_created_) {
      // Hold the reply plus delegate so a test can deliver a stale reply after
      // a teardown or restart, without completing this cycle's creation.
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
    std::move(created).Run(MakeFakeBwc(delegate, &last_bwc_), &otr_profile_);
  }

  // Builds a fake worker BackgroundWebContents that nulls `*tracker` when the
  // controller drops it, so a test can observe teardown.
  std::unique_ptr<FakeBackgroundWebContents> MakeFakeBwc(
      base::WeakPtr<local_ai::BackgroundWebContents::Delegate> delegate,
      raw_ptr<FakeBackgroundWebContents>* tracker) {
    auto bwc = std::make_unique<FakeBackgroundWebContents>(
        delegate.get(),
        base::BindOnce(
            [](raw_ptr<FakeBackgroundWebContents>* ref) { *ref = nullptr; },
            tracker));
    *tracker = bwc.get();
    return bwc;
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
    // ReadNemotronOrtFiles only needs these to exist and be non-empty; the
    // contents are ignored by the fake factory's Init.
    for (const char* name :
         {"encoder.onnx", "encoder.onnx.data", "decoder_joint.onnx",
          "decoder_joint.onnx.data", "filterbank.bin", "tokens.txt"}) {
      ASSERT_TRUE(base::WriteFile(model_dir.AppendASCII(name), "x"));
    }
  }

  // Acts as the worker WebUI: registers the given factory so the browser can
  // load the model. Does not wait for the load to finish.
  void RegisterFactoryWith(FakeSpeechRecognitionFactory& factory) {
    // Only one worker page ever holds the FactoryHost interface, so keep a
    // single host remote and supersede any previous one, mirroring the
    // controller's own single receiver that reset()s on each rebind.
    factory_host_.reset();
    controller_->BindFactoryHost(factory_host_.BindNewPipeAndPassReceiver());
    factory_host_->RegisterFactory(factory.BindRemote());
    factory_host_.FlushForTesting();
  }

  void RegisterFactory() { RegisterFactoryWith(fake_factory_); }

  // Runs the loop until `factory` has loaded the model, quitting from the
  // factory's own Init callback.
  void WaitForInit(FakeSpeechRecognitionFactory& factory) {
    if (factory.init_count() >= 1) {
      return;
    }
    base::RunLoop loop;
    factory.set_on_call(base::BindLambdaForTesting([&] {
      if (factory.init_count() >= 1) {
        loop.Quit();
      }
    }));
    loop.Run();
    factory.set_on_call(base::RepeatingClosure());
  }

  // Runs the loop until `factory` has created at least `count` ASR streams,
  // quitting from its own CreateAsrStream callback.
  void WaitForCreatedAsrStreams(FakeSpeechRecognitionFactory& factory,
                                int count) {
    if (factory.created_asr_stream_count() >= count) {
      return;
    }
    base::RunLoop loop;
    factory.set_on_call(base::BindLambdaForTesting([&] {
      if (factory.created_asr_stream_count() >= count) {
        loop.Quit();
      }
    }));
    loop.Run();
    factory.set_on_call(base::RepeatingClosure());
  }

  // Starts one session and drives the worker all the way to Ready, returning
  // the (now-forwarded) session.
  Session DriveToReady() {
    SetInstalled(true);
    Session s = StartSession();
    RegisterFactory();
    WaitForCreatedAsrStreams(fake_factory_, 1);
    return s;
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

  // Runs the loop until `remote`'s peer closes, driven by the disconnect
  // handler rather than by polling.
  template <typename Interface>
  void WaitForDisconnect(mojo::Remote<Interface>& remote) {
    if (!remote.is_connected()) {
      return;
    }
    base::test::TestFuture<void> disconnected;
    remote.set_disconnect_handler(disconnected.GetCallback());
    EXPECT_TRUE(disconnected.Wait());
  }

  content::BrowserTaskEnvironment task_environment_;
  // The guest OTR profile the fake reports as the worker's home. A non-null
  // profile lets the boot flow proceed.
  TestingProfile otr_profile_;
  // A different profile, for asserting the observer ignores others.
  TestingProfile other_profile_;
  base::ScopedTempDir temp_dir_;
  FakeSpeechRecognitionFactory fake_factory_;
  mojo::Remote<local_ai::mojom::SpeechRecognitionFactoryHost> factory_host_;
  raw_ptr<FakeBackgroundWebContents> last_bwc_ = nullptr;
  // When set, the injected factory reports an async creation failure.
  bool bwc_creation_fails_ = false;
  // When set, CreateFakeBwc captures the reply plus delegate instead of
  // completing creation, so a test can deliver a stale reply later.
  bool capture_created_ = false;
  base::WeakPtr<local_ai::BackgroundWebContents::Delegate> captured_delegate_;
  local_ai::BackgroundWebContentsCreatedCallback captured_created_;
  int bwc_created_count_ = 0;
  std::unique_ptr<OnDeviceSpeechRecognitionController> controller_;
};

// ---------- Working behavior ----------

// The full working path: sessions that arrive before the worker is ready boot
// one shared worker and queue, the model loads once, every queued session is
// forwarded when ready, and a later session forwards immediately to the same
// worker.
TEST_F(OnDeviceSpeechRecognitionControllerTest,
       ForwardsSessionsOnceWorkerReady) {
  SetInstalled(true);
  Session s1 = StartSession();
  Session s2 = StartSession();
  EXPECT_EQ(1, bwc_created_count_);
  EXPECT_EQ(0, fake_factory_.created_asr_stream_count());

  RegisterFactory();
  WaitForCreatedAsrStreams(fake_factory_, 2);
  EXPECT_EQ(1, fake_factory_.init_count());
  EXPECT_EQ(2, fake_factory_.created_asr_stream_count());

  // A session started on the now-ready worker is forwarded immediately, with
  // no new worker booted.
  Session s3 = StartSession();
  WaitForCreatedAsrStreams(fake_factory_, 3);
  EXPECT_EQ(1, bwc_created_count_);
}

// ---------- Error and edge handling ----------

TEST_F(OnDeviceSpeechRecognitionControllerTest,
       StartWithNoModelDropsSessionNoBoot) {
  SetInstalled(false);
  Session s = StartSession();
  // No worker is spun up just to discover the files are missing.
  EXPECT_EQ(nullptr, last_bwc_.get());
  EXPECT_EQ(0, bwc_created_count_);
  // The session's audio pipe is dropped, surfacing as a failure to the engine.
  WaitForDisconnect(s.input);
  EXPECT_FALSE(s.input.is_connected());
}

// A worker whose environment fails to come up tears down, drops the queued
// session, and returns to idle so a later Start() reboots cleanly.
TEST_F(OnDeviceSpeechRecognitionControllerTest, BwcCreationFailureTearsDown) {
  SetInstalled(true);
  bwc_creation_fails_ = true;

  Session s = StartSession();
  WaitForDisconnect(s.input);
  EXPECT_EQ(nullptr, last_bwc_.get());

  // Back to idle: a later attempt succeeds once creation works again.
  bwc_creation_fails_ = false;
  Session s2 = StartSession();
  EXPECT_NE(nullptr, last_bwc_.get());
  EXPECT_EQ(2, bwc_created_count_);
}

// Renderer loss is reported through the delegate. TearDown is deferred off the
// observer callback (resetting the BackgroundWebContents synchronously would
// destroy it from inside its own destroy notification), so the worker is still
// present until the loop is pumped.
TEST_F(OnDeviceSpeechRecognitionControllerTest,
       BackgroundContentsDestroyedTearsDownDeferred) {
  SetInstalled(true);
  Session s = StartSession();
  ASSERT_NE(nullptr, last_bwc_.get());

  last_bwc_->SimulateDestroyed();
  // The teardown is posted, not run inline, so the worker is still present.
  EXPECT_NE(nullptr, last_bwc_.get());

  // The deferred teardown disconnects the engine-held session and drops the
  // worker.
  WaitForDisconnect(s.session);
  EXPECT_EQ(nullptr, last_bwc_.get());
}

// The controller observes the worker's OTR profile so it tears the worker down
// before that profile is destroyed, and ignores any other profile.
TEST_F(OnDeviceSpeechRecognitionControllerTest,
       ProfileDestructionTearsDownObservedOnly) {
  SetInstalled(true);
  Session s = StartSession();
  ASSERT_NE(nullptr, last_bwc_.get());

  // A different profile's destruction is ignored.
  controller_->OnProfileWillBeDestroyed(&other_profile_);
  EXPECT_NE(nullptr, last_bwc_.get());

  // The observed OTR profile's destruction tears the worker down. Invoking the
  // callback directly exercises the controller's response without dragging in
  // real profile-destruction machinery.
  controller_->OnProfileWillBeDestroyed(&otr_profile_);
  EXPECT_EQ(nullptr, last_bwc_.get());
}

// The worker environment comes up but the worker never registers its factory.
// The 30s startup timer tears it down and drops the queued session so the
// engine sees a failure instead of hanging.
TEST_F(OnDeviceSpeechRecognitionControllerTest, StartupTimeoutTearsDown) {
  SetInstalled(true);
  Session s = StartSession();
  ASSERT_NE(nullptr, last_bwc_.get());

  task_environment_.FastForwardBy(base::Seconds(31));
  EXPECT_EQ(nullptr, last_bwc_.get());
  // The queued session's audio pipe is dropped, surfacing as a failure.
  WaitForDisconnect(s.input);
  EXPECT_FALSE(s.input.is_connected());
}

// A creation reply from a torn-down cycle must not land in a later cycle and
// adopt a stale worker. TearDown()'s InvalidateWeakPtrs() drops the reply; the
// state check alone is insufficient because the later cycle is also
// kRendererStarting.
TEST_F(OnDeviceSpeechRecognitionControllerTest, StaleBwcCreationReplyDropped) {
  SetInstalled(true);
  capture_created_ = true;

  // Cycle 1: capture its weak-bound creation reply, then take it aside before
  // the next cycle overwrites the captured slot.
  Session s1 = StartSession();
  auto stale_created = std::move(captured_created_);
  auto stale_delegate = captured_delegate_;

  // The startup timer tears cycle 1 down, invalidating the captured reply.
  task_environment_.FastForwardBy(base::Seconds(31));

  // Cycle 2: back in kRendererStarting (its reply is captured, not run).
  Session s2 = StartSession();

  // Deliver the stale cycle-1 reply. It must be ignored (its weak pointer was
  // invalidated), so the worker it carries is dropped, not adopted into cycle
  // 2. Were it adopted, the controller would hold the fake and keep it alive.
  raw_ptr<FakeBackgroundWebContents> stale_bwc = nullptr;
  std::move(stale_created)
      .Run(MakeFakeBwc(stale_delegate, &stale_bwc), &otr_profile_);
  EXPECT_EQ(nullptr, stale_bwc);
}

// RegisterFactory is a no-op unless the controller is in kRendererStarting. A
// register in kIdle is ignored, and a second register after the worker has
// advanced past kRendererStarting is ignored too (state has moved on), so the
// live worker is untouched.
TEST_F(OnDeviceSpeechRecognitionControllerTest,
       RegisterFactoryIgnoredOutsideRendererStarting) {
  SetInstalled(true);

  // In kIdle (no Start yet): registering a factory does nothing.
  {
    FakeSpeechRecognitionFactory early;
    RegisterFactoryWith(early);
    EXPECT_EQ(0, early.init_count());
    EXPECT_EQ(nullptr, last_bwc_.get());
  }

  // Boot the worker and load the model, then a second register is ignored.
  Session s = StartSession();
  RegisterFactory();
  WaitForInit(fake_factory_);
  ASSERT_EQ(1, fake_factory_.init_count());
  {
    FakeSpeechRecognitionFactory late;
    RegisterFactoryWith(late);
    EXPECT_EQ(0, late.init_count());
    // The original worker was not reloaded.
    EXPECT_EQ(1, fake_factory_.init_count());
    EXPECT_NE(nullptr, last_bwc_.get());
  }
}

// A misbehaving renderer may request the FactoryHost interface again while
// already bound. The second bind supersedes the first rather than tripping a
// DCHECK, so the first host remote disconnects.
TEST_F(OnDeviceSpeechRecognitionControllerTest,
       BindFactoryHostSupersedesPrevious) {
  mojo::Remote<local_ai::mojom::SpeechRecognitionFactoryHost> host1;
  controller_->BindFactoryHost(host1.BindNewPipeAndPassReceiver());
  host1.FlushForTesting();
  ASSERT_TRUE(host1.is_connected());

  mojo::Remote<local_ai::mojom::SpeechRecognitionFactoryHost> host2;
  controller_->BindFactoryHost(host2.BindNewPipeAndPassReceiver());
  host2.FlushForTesting();

  WaitForDisconnect(host1);
  EXPECT_FALSE(host1.is_connected());
  EXPECT_TRUE(host2.is_connected());
}

TEST_F(OnDeviceSpeechRecognitionControllerTest, ModelFileReadFailureTearsDown) {
  // Install dir is set (so Start proceeds) but the model files are absent, so
  // ReadNemotronOrtFiles returns null and the worker tears down without Init.
  local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
      temp_dir_.GetPath());
  Session s = StartSession();
  ASSERT_NE(nullptr, last_bwc_.get());
  RegisterFactory();

  // TearDown disconnects the engine-held session; the worker is gone by then.
  WaitForDisconnect(s.session);
  EXPECT_EQ(nullptr, last_bwc_.get());
  EXPECT_EQ(0, fake_factory_.init_count());
}

TEST_F(OnDeviceSpeechRecognitionControllerTest, InitFailureTearsDown) {
  SetInstalled(true);
  fake_factory_.set_init_success(false);

  Session s = StartSession();
  RegisterFactory();

  // TearDown disconnects the engine-held session; the worker is gone by then.
  WaitForDisconnect(s.session);
  EXPECT_EQ(nullptr, last_bwc_.get());
}

// The model-file read hops to a ThreadPool and replies weakly. If the cycle is
// torn down while the read is in flight, the reply must not Init a later
// cycle's factory. To prove the weak invalidation (and not the is_bound guard)
// is what protects this, cycle 2 binds a fresh factory before the stale reply
// lands: the guard would pass against that live factory, so only the weak drop
// keeps it from being Init'd twice.
TEST_F(OnDeviceSpeechRecognitionControllerTest,
       StaleModelReadReplyDroppedAfterRestart) {
  SetInstalled(true);

  // Register the factory and tear down through the controller's public methods
  // directly rather than over a mojo pipe. FlushForTesting on a pipe spins a
  // nested loop that would let the tiny fixture read complete inline; a direct
  // call posts the read and tears down with no pump in between, so the reply is
  // genuinely in flight when InvalidateWeakPtrs() drops it.

  // Cycle 1: reach kModelLoading, which posts the ThreadPool read.
  Session s1 = StartSession();
  controller_->RegisterFactory(fake_factory_.BindRemote());

  // Tear cycle 1 down before the read reply is pumped.
  controller_->OnProfileWillBeDestroyed(&otr_profile_);
  ASSERT_EQ(nullptr, last_bwc_.get());

  // Cycle 2: a fresh worker and factory, also reaching kModelLoading.
  FakeSpeechRecognitionFactory fake2;
  Session s2 = StartSession();
  controller_->RegisterFactory(fake2.BindRemote());

  // Drain the pool: both reads complete. Cycle 1's reply is weak-invalidated
  // and dropped, so only cycle 2's reply runs and fake2 is Init'd exactly once.
  // Were the stale reply not dropped, it would Init the now-live fake2 a second
  // time, since the is_bound guard passes against the fresh factory.
  WaitForInit(fake2);
  EXPECT_EQ(1, fake2.init_count());
  // The torn-down cycle-1 factory was never Init'd.
  EXPECT_EQ(0, fake_factory_.init_count());
}

// A worker crash after Ready (its factory remote disconnects) tears the worker
// down and disconnects the AsrSession the engine is holding.
TEST_F(OnDeviceSpeechRecognitionControllerTest, FactoryDisconnectTearsDown) {
  Session s = DriveToReady();
  ASSERT_NE(nullptr, last_bwc_.get());
  ASSERT_TRUE(s.session.is_connected());

  // The controller tears down when it sees the factory remote drop, which
  // disconnects the engine-held AsrSession.
  fake_factory_.ResetReceiver();
  WaitForDisconnect(s.session);
  EXPECT_EQ(nullptr, last_bwc_.get());
}

// A worker that reached Ready must survive past the startup timeout: the timer
// is stopped in RegisterFactory, and OnStartupTimeout's state guard would
// ignore a stale fire regardless. Prove it is still serving by forwarding a new
// session to the same worker.
TEST_F(OnDeviceSpeechRecognitionControllerTest,
       ReadyWorkerSurvivesStartupTimeout) {
  Session s = DriveToReady();
  ASSERT_NE(nullptr, last_bwc_.get());
  ASSERT_EQ(1, fake_factory_.created_asr_stream_count());

  task_environment_.FastForwardBy(base::Seconds(31));
  ASSERT_NE(nullptr, last_bwc_.get());

  Session s2 = StartSession();
  WaitForCreatedAsrStreams(fake_factory_, 2);
  EXPECT_EQ(1, bwc_created_count_);
}

TEST_F(OnDeviceSpeechRecognitionControllerTest,
       IdleTimeoutTearsDownAfterLastSession) {
  Session s = DriveToReady();
  ASSERT_NE(nullptr, last_bwc_.get());

  // Dropping the last AsrSession arms the idle timer. FastForwardBy runs the
  // immediate disconnect task first, arming the timer, then advances past the
  // 60s idle timeout so it fires.
  s.session.reset();
  task_environment_.FastForwardBy(base::Seconds(61));
  EXPECT_EQ(nullptr, last_bwc_.get());
}

TEST_F(OnDeviceSpeechRecognitionControllerTest, NewSessionCancelsIdleTimeout) {
  Session s = DriveToReady();
  ASSERT_NE(nullptr, last_bwc_.get());

  // Drop the session and advance a little so the disconnect is processed and
  // the idle timer arms.
  s.session.reset();
  task_environment_.FastForwardBy(base::Seconds(1));

  // A new session before the timeout stops the armed timer.
  mojo::Remote<local_ai::mojom::AsrSession> reconnect(
      controller_->GetAsrSession());

  task_environment_.FastForwardBy(base::Seconds(61));
  EXPECT_NE(nullptr, last_bwc_.get());
}

// With more than one session, the idle timer arms only when the last session
// disconnects, not on each disconnect.
TEST_F(OnDeviceSpeechRecognitionControllerTest,
       IdleTimerArmedOnlyOnLastSession) {
  Session s1 = DriveToReady();
  Session s2 = StartSession();
  WaitForCreatedAsrStreams(fake_factory_, 2);
  ASSERT_NE(nullptr, last_bwc_.get());

  // Dropping one of two sessions does not arm the timer.
  s1.session.reset();
  task_environment_.FastForwardBy(base::Seconds(61));
  EXPECT_NE(nullptr, last_bwc_.get());

  // Dropping the last session arms it, and the worker tears down.
  s2.session.reset();
  task_environment_.FastForwardBy(base::Seconds(61));
  EXPECT_EQ(nullptr, last_bwc_.get());
}

}  // namespace speech
