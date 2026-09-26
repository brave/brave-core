/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "brave/components/local_ai/core/test/fake_asr_session.h"
#include "chrome/browser/browser_process.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/update_client_errors.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "media/base/media_switches.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// What a page is told by Web Speech's `available()`, `install()` and
// `start()`, for each quality and with or without Brave's model on disk. Every
// page is on an origin that has installed nothing.

namespace speech {

namespace {

constexpr char kUnavailable[] = "unavailable";
constexpr char kDownloadable[] = "downloadable";
constexpr char kAvailable[] = "available";
constexpr char kTranscript[] = "hello";
constexpr char kError[] = "error: ";

// What a page is told by `available()`, `install()`, `available()` again and
// then `start()`. `start` is the transcript, or the start of an error.
struct Expected {
  std::string_view available;
  bool installed;
  std::string_view available_again;
  std::string_view start;
};

// Hands out the fake worker session in place of the real controller, which
// would boot a renderer and read a 600 MB model off disk.
class TestContentBrowserClient : public BraveContentBrowserClient {
 public:
  explicit TestContentBrowserClient(local_ai::FakeAsrSession& session)
      : session_(&session) {}

  mojo::PendingRemote<local_ai::mojom::AsrSession> GetAsrSession() override {
    requested.SetValue();
    return session_->BindRemote();
  }

  // Ready once the engine has asked the embedder for a session.
  base::test::TestFuture<void> requested;

 private:
  raw_ptr<local_ai::FakeAsrSession> session_;
};

}  // namespace

class BraveOnDeviceSpeechBrowserTest : public InProcessBrowserTest {
 public:
  BraveOnDeviceSpeechBrowserTest() {
    feature_list_.InitWithFeatures({local_ai::kBraveOnDeviceSpeechRecognition},
                                   {media::kOnDeviceWebSpeechSmallExpertModel,
                                    media::kOnDeviceWebSpeechGeminiNano});
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    // The recognitions below are fed by an AudioContext, which stays suspended
    // under the default policy and would render no audio.
    command_line->AppendSwitchASCII(
        switches::kAutoplayPolicy,
        switches::autoplay::kNoUserGestureRequiredPolicy);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_https_test_server().ServeFilesFromSourceDirectory(
        GetChromeTestDataDir());
    ASSERT_TRUE(embedded_https_test_server().Start());

    // Replaces the on-demand updater browser startup installed, so it cannot
    // be built with the fixture.
    on_demand_updater_ =
        std::make_unique<brave_component_updater::MockOnDemandUpdater>();

    // Browser tests run with `--disable-component-update`
    // (`chrome/test/base/test_launcher_utils.cc`), so startup registers no
    // components and the registrar has no update service. Start it here
    // instead. Nothing reaches the network, since the updater is mocked.
    local_ai::ManageOnDeviceSpeechModelsComponentRegistration(
        g_browser_process->component_updater(),
        g_browser_process->local_state());
    DrainRegistrationRequest();

    original_client_ = content::SetBrowserClientForTesting(&client_);
    // Only a recognition run by Brave's engine can read this back.
    fake_session_.RespondOnNextAudioChunk(kTranscript);
  }

  void TearDownOnMainThread() override {
    content::SetBrowserClientForTesting(original_client_);
    // Drops the registrar's pointers to this browser, so the next `Manage...`
    // call can start it again.
    local_ai::ShutdownOnDeviceSpeechModelsComponentRegistration();
    // Process-wide, so leave it as a fresh browser would have it.
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        base::FilePath());
    on_demand_updater_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

 protected:
  content::RenderFrameHost* main_frame() {
    return chrome_test_utils::GetActiveWebContents(this)->GetPrimaryMainFrame();
  }

  // What the component installer publishes once a model is on disk. Nothing
  // reads the files, because the worker that would is replaced by the test
  // client.
  void PublishModel() {
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        base::FilePath(FILE_PATH_LITERAL("/brave/speech/models")));
  }

  // Answers the download `BraveSodaInstaller::InstallLanguage` asks for, and
  // expects to be asked. `publish_model` stands in for the component
  // arriving, which a real download does before the request it answers is
  // settled.
  void AnswerDownloadWith(update_client::Error error, bool publish_model) {
    EXPECT_CALL(
        *on_demand_updater_,
        EnsureInstalled(local_ai::kOnDeviceSpeechModelsComponentId, testing::_))
        .Times(testing::AtLeast(1))
        .WillRepeatedly(
            [this, error, publish_model](const std::string& id,
                                         component_updater::Callback callback) {
              if (publish_model) {
                PublishModel();
              }
              std::move(callback).Run(error);
            });
  }

  void NavigateToUrl(const std::string& host) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_https_test_server().GetURL(host, "/empty.html")));
  }

  // Sets `processLocally`, because without it the renderer resolves
  // `available` itself, assuming cloud recognition, and never asks the
  // browser.
  content::EvalJsResult Available(std::string_view quality) {
    return content::EvalJs(
        main_frame(),
        content::JsReplace("SpeechRecognition.available({langs: ['en-US'], "
                           "processLocally: true, quality: $1})",
                           quality));
  }

  // Sets `processLocally`, because without it the renderer resolves `false`
  // itself. `EvalJs` runs with a user gesture, which `install()` requires
  // while the answer is downloadable.
  content::EvalJsResult Install(std::string_view quality) {
    return content::EvalJs(
        main_frame(),
        content::JsReplace("SpeechRecognition.install({langs: ['en-US'], "
                           "processLocally: true, quality: $1})",
                           quality));
  }

  // Runs a recognition over a MediaStreamTrack and resolves with the first
  // transcript, or with `error: <name>` if it fails instead. The track comes
  // from an oscillator rather than a microphone, so no capture device or
  // permission is needed. Upstream's web platform test builds its track the
  // same way.
  content::EvalJsResult StartRecognition(std::string_view quality,
                                         bool process_locally) {
    return content::EvalJs(main_frame(),
                           content::JsReplace(R"JS(
      (async () => {
        const context = new AudioContext();
        const destination = context.createMediaStreamDestination();
        const oscillator = context.createOscillator();
        oscillator.connect(destination);
        oscillator.start();

        const recognition = new SpeechRecognition();
        recognition.lang = 'en-US';
        recognition.quality = $1;
        recognition.processLocally = $2;
        return await new Promise(resolve => {
          recognition.onresult = e => resolve(e.results[0][0].transcript);
          recognition.onerror = e => resolve('error: ' + e.error);
          recognition.start(destination.stream.getAudioTracks()[0]);
        });
      })()
  )JS",
                                              quality, process_locally));
  }

  // Makes one page's calls in order and checks each answer. A transcript can
  // only come from Brave's engine, so it also says whether the engine ran.
  void ExpectPageIsTold(std::string_view quality,
                        bool process_locally,
                        const Expected& expected) {
    EXPECT_EQ(expected.available, Available(quality));
    EXPECT_EQ(expected.installed, Install(quality));
    EXPECT_EQ(expected.available_again, Available(quality));
    EXPECT_THAT(StartRecognition(quality, process_locally).ExtractString(),
                testing::StartsWith(expected.start));

    const bool ran_on_brave_engine = expected.start == kTranscript;
    EXPECT_EQ(ran_on_brave_engine, client_.requested.IsReady());
    EXPECT_EQ(ran_on_brave_engine, fake_session_.started().IsReady());
  }

  local_ai::FakeAsrSession fake_session_;
  TestContentBrowserClient client_{fake_session_};

 private:
  // Starting the registrar asks for a download of its own while the feature
  // is on. Draining it leaves a test body's request to start a fresh one
  // rather than join this.
  void DrainRegistrationRequest() {
    EXPECT_CALL(
        *on_demand_updater_,
        EnsureInstalled(local_ai::kOnDeviceSpeechModelsComponentId, testing::_))
        .WillRepeatedly(
            [](const std::string& id, component_updater::Callback callback) {
              std::move(callback).Run(update_client::Error::NONE);
            });
    base::test::TestFuture<bool> answered;
    local_ai::MaybeRegisterOnDeviceSpeechModelsComponent(
        answered.GetCallback());
    ASSERT_TRUE(answered.Wait());
    testing::Mock::VerifyAndClearExpectations(on_demand_updater_.get());
  }

  std::unique_ptr<brave_component_updater::MockOnDemandUpdater>
      on_demand_updater_;
  raw_ptr<content::ContentBrowserClient> original_client_ = nullptr;
  base::test::ScopedFeatureList feature_list_;
};

namespace {

struct ServedCase {
  bool model_on_disk;
  bool process_locally;
  // How the download `install()` asks for is answered, when it asks for one.
  std::optional<update_client::Error> download;
  Expected expected;
};

// What a fresh origin is told for a quality Brave's model serves, in the order
// `available()`, `install()`, `available()` again, then `start()`. The first
// `available()` is downloadable in every row, because upstream masks the
// answer for an origin that has not installed.
constexpr ServedCase kServedCases[] = {
    // The model is already on disk, so `install()` succeeds without a download
    // and unmasks the origin. The page then reads available, which the
    // renderer needs before it starts a `processLocally` recognition.
    {.model_on_disk = true,
     .process_locally = true,
     .expected = {kDownloadable, true, kAvailable, kTranscript}},
    // Upstream masks only what `OnDeviceSpeechRecognitionImpl` answers, which
    // `start()` without `processLocally` never asks, so it runs on Brave's
    // engine. Upstream would hand a recognition on a MediaStreamTrack to its
    // speech recognition service process, and Brave keeps it on the engine
    // instead.
    {.model_on_disk = true,
     .process_locally = false,
     .expected = {kDownloadable, true, kAvailable, kTranscript}},
    // `install()` downloads the model and unmasks the origin, so the
    // `processLocally` recognition starts.
    {.model_on_disk = false,
     .process_locally = true,
     .download = update_client::Error::NONE,
     .expected = {kDownloadable, true, kAvailable, kTranscript}},
    // The download puts the model on disk, so the recognition runs on Brave's
    // engine.
    {.model_on_disk = false,
     .process_locally = false,
     .download = update_client::Error::NONE,
     .expected = {kDownloadable, true, kAvailable, kTranscript}},
    // The download fails, and the installer's error report settles `install()`
    // as false rather than leaving it waiting. The page stays at downloadable,
    // so the renderer refuses the `processLocally` recognition.
    {.model_on_disk = false,
     .process_locally = true,
     .download = update_client::Error::INVALID_ARGUMENT,
     .expected = {kDownloadable, false, kDownloadable, kError}},
    // The download fails, so with no model the recognition falls back to the
    // cloud engine, which fails.
    {.model_on_disk = false,
     .process_locally = false,
     .download = update_client::Error::INVALID_ARGUMENT,
     .expected = {kDownloadable, false, kDownloadable, kError}},
};

std::string ServedCaseName(
    const testing::TestParamInfo<std::tuple<std::string_view, ServedCase>>&
        info) {
  const auto& [quality, served_case] = info.param;
  return std::string(quality == "command" ? "Command" : "Dictation") +
         (served_case.model_on_disk ? "WithModel" : "WithoutModel") +
         (served_case.download &&
                  *served_case.download != update_client::Error::NONE
              ? "DownloadFails"
              : "") +
         (served_case.process_locally ? "ProcessLocally"
                                      : "WithoutProcessLocally");
}

}  // namespace

// Runs every served case for each quality Brave's model serves.
class BraveOnDeviceSpeechServedBrowserTest
    : public BraveOnDeviceSpeechBrowserTest,
      public testing::WithParamInterface<
          std::tuple<std::string_view, ServedCase>> {};

IN_PROC_BROWSER_TEST_P(BraveOnDeviceSpeechServedBrowserTest,
                       AvailableInstallStart) {
  const auto& [quality, served_case] = GetParam();
  if (served_case.model_on_disk) {
    PublishModel();
  }
  if (served_case.download) {
    AnswerDownloadWith(
        *served_case.download,
        /*publish_model=*/*served_case.download == update_client::Error::NONE);
  }
  NavigateToUrl("foo.com");

  ExpectPageIsTold(quality, served_case.process_locally, served_case.expected);
}

INSTANTIATE_TEST_SUITE_P(All,
                         BraveOnDeviceSpeechServedBrowserTest,
                         testing::Combine(testing::Values("command",
                                                          "dictation"),
                                          testing::ValuesIn(kServedCases)),
                         ServedCaseName);

// Runs once per quality Brave's model serves.
class BraveOnDeviceSpeechQualityBrowserTest
    : public BraveOnDeviceSpeechBrowserTest,
      public testing::WithParamInterface<std::string_view> {};

// Turning Local AI off after a page has asked to install withdraws the model,
// both from `available()` and from `start()` without `processLocally`, which
// upstream does not mask.
IN_PROC_BROWSER_TEST_P(BraveOnDeviceSpeechQualityBrowserTest,
                       LocalAIOffAfterInstall) {
  PublishModel();
  NavigateToUrl("foo.com");

  EXPECT_EQ(kDownloadable, Available(GetParam()));
  EXPECT_EQ(true, Install(GetParam()));
  EXPECT_EQ(kAvailable, Available(GetParam()));

  g_browser_process->local_state()->SetBoolean(
      local_ai::prefs::kBraveLocalAIEnabled, false);

  EXPECT_EQ(kUnavailable, Available(GetParam()));
  EXPECT_THAT(
      StartRecognition(GetParam(), /*process_locally=*/false).ExtractString(),
      testing::StartsWith(kError));
  EXPECT_FALSE(client_.requested.IsReady());
  EXPECT_FALSE(fake_session_.started().IsReady());
}

INSTANTIATE_TEST_SUITE_P(
    All,
    BraveOnDeviceSpeechQualityBrowserTest,
    testing::Values("command", "dictation"),
    [](const testing::TestParamInfo<std::string_view>& info) {
      return info.param == "command" ? "Command" : "Dictation";
    });

// Runs with and without the model on disk, and with and without
// `processLocally`.
class BraveOnDeviceSpeechConversationBrowserTest
    : public BraveOnDeviceSpeechBrowserTest,
      public testing::WithParamInterface<std::tuple<bool, bool>> {
 protected:
  bool model_on_disk() const { return std::get<0>(GetParam()); }
  bool process_locally() const { return std::get<1>(GetParam()); }
};

// Brave's model does not serve `conversation`, so a page is refused it on
// every entry point.
IN_PROC_BROWSER_TEST_P(BraveOnDeviceSpeechConversationBrowserTest, Refused) {
  if (model_on_disk()) {
    PublishModel();
  }
  NavigateToUrl("foo.com");

  ExpectPageIsTold("conversation", process_locally(),
                   {kUnavailable, false, kUnavailable, kError});
}

INSTANTIATE_TEST_SUITE_P(
    All,
    BraveOnDeviceSpeechConversationBrowserTest,
    testing::Combine(testing::Bool(), testing::Bool()),
    [](const testing::TestParamInfo<std::tuple<bool, bool>>& info) {
      return std::string(std::get<0>(info.param) ? "WithModel"
                                                 : "WithoutModel") +
             (std::get<1>(info.param) ? "ProcessLocally"
                                      : "WithoutProcessLocally");
    });

namespace {

std::string FeatureOffCaseName(
    const testing::TestParamInfo<std::tuple<std::string_view, bool, bool>>&
        info) {
  const auto& [quality, model_on_disk, process_locally] = info.param;
  std::string name(quality);
  name[0] = base::ToUpperASCII(name[0]);
  return name + (model_on_disk ? "WithModel" : "WithoutModel") +
         (process_locally ? "ProcessLocally" : "WithoutProcessLocally");
}

}  // namespace

// Runs every quality with and without the model on disk, and with and without
// `processLocally`, with Brave's feature off.
class BraveOnDeviceSpeechFeatureOffBrowserTest
    : public BraveOnDeviceSpeechBrowserTest,
      public testing::WithParamInterface<
          std::tuple<std::string_view, bool, bool>> {
 public:
  BraveOnDeviceSpeechFeatureOffBrowserTest() {
    feature_off_.InitAndDisableFeature(
        local_ai::kBraveOnDeviceSpeechRecognition);
  }

 protected:
  std::string_view quality() const { return std::get<0>(GetParam()); }
  bool model_on_disk() const { return std::get<1>(GetParam()); }
  bool process_locally() const { return std::get<2>(GetParam()); }

 private:
  base::test::ScopedFeatureList feature_off_;
};

// With the feature off Brave offers nothing, even with the model on disk, and
// its engine never runs.
IN_PROC_BROWSER_TEST_P(BraveOnDeviceSpeechFeatureOffBrowserTest, Refused) {
  if (model_on_disk()) {
    PublishModel();
  }
  NavigateToUrl("foo.com");

  ExpectPageIsTold(quality(), process_locally(),
                   {kUnavailable, false, kUnavailable, kError});
}

INSTANTIATE_TEST_SUITE_P(
    All,
    BraveOnDeviceSpeechFeatureOffBrowserTest,
    testing::Combine(testing::Values("command", "dictation", "conversation"),
                     testing::Bool(),
                     testing::Bool()),
    FeatureOffCaseName);

}  // namespace speech
