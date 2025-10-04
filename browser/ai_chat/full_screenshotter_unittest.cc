// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/full_screenshotter.h"

#include <memory>

#include "base/notreached.h"
#include "base/test/test_future.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/paint_preview/common/mock_paint_preview_recorder.h"
#include "components/paint_preview/common/mojom/paint_preview_recorder.mojom.h"
#include "components/services/paint_preview_compositor/public/mojom/paint_preview_compositor.mojom.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "content/test/test_render_view_host.h"
#include "content/test/test_web_contents.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace ai_chat {

namespace {

class TestView : public content::TestRenderWidgetHostView {
 public:
  explicit TestView(content::RenderWidgetHost* widget)
      : TestRenderWidgetHostView(widget) {}

  gfx::Rect GetViewBounds() override { return view_bounds_; }
  void SetViewBounds(const gfx::Rect& bounds) { view_bounds_ = bounds; }

 private:
  gfx::Rect view_bounds_;
};

class LaxMockPaintPreviewRecorder
    : public paint_preview::MockPaintPreviewRecorder {
 public:
  LaxMockPaintPreviewRecorder() = default;
  ~LaxMockPaintPreviewRecorder() override = default;
  void CheckParams(const paint_preview::mojom::PaintPreviewCaptureParamsPtr&
                       params) override {}
};

class MockPaintPreviewCompositorClient
    : public paint_preview::PaintPreviewCompositorClient {
 public:
  explicit MockPaintPreviewCompositorClient(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : response_status_(paint_preview::mojom::PaintPreviewCompositor::
                             BeginCompositeStatus::kSuccess),
        bitmap_status_(paint_preview::mojom::PaintPreviewCompositor::
                           BitmapStatus::kSuccess),
        is_empty_bitmap_(false),
        token_(base::UnguessableToken::Create()),
        task_runner_(task_runner) {}
  ~MockPaintPreviewCompositorClient() override = default;

  MockPaintPreviewCompositorClient(const MockPaintPreviewCompositorClient&) =
      delete;
  MockPaintPreviewCompositorClient& operator=(
      const MockPaintPreviewCompositorClient&) = delete;

  const std::optional<base::UnguessableToken>& Token() const override {
    return token_;
  }

  void SetDisconnectHandler(base::OnceClosure closure) override {
    disconnect_handler_ = std::move(closure);
  }

  void BeginSeparatedFrameComposite(
      paint_preview::mojom::PaintPreviewBeginCompositeRequestPtr request,
      paint_preview::mojom::PaintPreviewCompositor::
          BeginSeparatedFrameCompositeCallback callback) override {
    NOTREACHED();
  }

  void BitmapForSeparatedFrame(
      const base::UnguessableToken& frame_guid,
      const gfx::Rect& clip_rect,
      float scale_factor,
      paint_preview::mojom::PaintPreviewCompositor::
          BitmapForSeparatedFrameCallback callback,
      bool run_task_on_default_task_runner = true) override {
    NOTREACHED();
  }

  void BeginMainFrameComposite(
      paint_preview::mojom::PaintPreviewBeginCompositeRequestPtr request,
      paint_preview::mojom::PaintPreviewCompositor::
          BeginMainFrameCompositeCallback callback) override {
    auto response =
        paint_preview::mojom::PaintPreviewBeginCompositeResponse::New();
    response->root_frame_guid = root_frame_guid_;
    if (!frames_.empty()) {
      response->frames = std::move(frames_);
    }
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(std::move(callback), response_status_,
                                          std::move(response)));
  }

  void BitmapForMainFrame(
      const gfx::Rect& clip_rect,
      float scale_factor,
      paint_preview::mojom::PaintPreviewCompositor::BitmapForMainFrameCallback
          callback,
      bool run_task_on_default_task_runner = true) override {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), bitmap_status_,
                       is_empty_bitmap_
                           ? SkBitmap()
                           : gfx::test::CreateBitmap(clip_rect.width(),
                                                     clip_rect.height())),
        base::Seconds(1));
  }

  void SetRootFrameUrl(const GURL& url) override {
    // no-op.
  }

  void SetBeginMainFrameResponseStatus(
      paint_preview::mojom::PaintPreviewCompositor::BeginCompositeStatus
          status) {
    response_status_ = status;
  }

  void SetBitmapStatus(
      paint_preview::mojom::PaintPreviewCompositor::BitmapStatus status) {
    bitmap_status_ = status;
  }

  void SetIsEmptyBitmap(bool is_empty) { is_empty_bitmap_ = is_empty; }

  void Disconnect() {
    if (disconnect_handler_) {
      std::move(disconnect_handler_).Run();
    }
  }

  void SetCompositeResponse(
      base::flat_map<base::UnguessableToken,
                     mojo::StructPtr<paint_preview::mojom::FrameData>> frames,
      const base::UnguessableToken& root_guid) {
    frames_ = std::move(frames);
    root_frame_guid_ = root_guid;
  }

 private:
  paint_preview::mojom::PaintPreviewCompositor::BeginCompositeStatus
      response_status_;
  paint_preview::mojom::PaintPreviewCompositor::BitmapStatus bitmap_status_;
  bool is_empty_bitmap_;
  std::optional<base::UnguessableToken> token_;
  base::OnceClosure disconnect_handler_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::flat_map<base::UnguessableToken,
                 mojo::StructPtr<paint_preview::mojom::FrameData>>
      frames_;
  base::UnguessableToken root_frame_guid_;
};

class MockPaintPreviewCompositorService
    : public paint_preview::PaintPreviewCompositorService {
 public:
  explicit MockPaintPreviewCompositorService(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : task_runner_(task_runner) {}
  ~MockPaintPreviewCompositorService() override = default;

  MockPaintPreviewCompositorService(const MockPaintPreviewCompositorService&) =
      delete;
  MockPaintPreviewCompositorService& operator=(
      const MockPaintPreviewCompositorService&) = delete;

  std::unique_ptr<paint_preview::PaintPreviewCompositorClient,
                  base::OnTaskRunnerDeleter>
  CreateCompositor(base::OnceClosure connected_closure) override {
    task_runner_->PostTask(FROM_HERE, std::move(connected_closure));
    return std::unique_ptr<MockPaintPreviewCompositorClient,
                           base::OnTaskRunnerDeleter>(
        new MockPaintPreviewCompositorClient(task_runner_),
        base::OnTaskRunnerDeleter(task_runner_));
  }

  void OnMemoryPressure(base::MemoryPressureListener::MemoryPressureLevel
                            memory_pressure_level) override {
    // no-op.
  }

  bool HasActiveClients() const override { NOTREACHED(); }

  void SetDisconnectHandler(base::OnceClosure disconnect_handler) override {
    disconnect_handler_ = std::move(disconnect_handler);
  }

  void Disconnect() {
    if (disconnect_handler_) {
      std::move(disconnect_handler_).Run();
    }
  }

 private:
  base::OnceClosure disconnect_handler_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

MockPaintPreviewCompositorClient* AsMockClient(
    paint_preview::PaintPreviewCompositorClient* client) {
  return static_cast<MockPaintPreviewCompositorClient*>(client);
}

}  // namespace

class FullScreenshotterTest : public ChromeRenderViewHostTestHarness {
 public:
  FullScreenshotterTest() = default;
  ~FullScreenshotterTest() override = default;

  FullScreenshotterTest(const FullScreenshotterTest&) = delete;
  FullScreenshotterTest& operator=(const FullScreenshotterTest&) = delete;

 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    NavigateAndCommit(GURL("https://brave.com/"),
                      ui::PageTransition::PAGE_TRANSITION_FIRST);
    // Store the original RenderWidgetHost, allowing it to be injected back from
    // the destructor.
    original_rwhv_ = GetRenderWidgetHostImpl()->GetView();

    // Set a new RenderWidgetHost that allows us control over its size.
    rwhv_ = std::make_unique<TestView>(GetRenderWidgetHostImpl());
    SetView(rwhv_.get());
    SetSize(gfx::Size(320, 240));
    full_screenshotter_ = std::make_unique<FullScreenshotter>();
  }
  void TearDown() override {
    SetView(original_rwhv_);
    original_rwhv_ = nullptr;
    full_screenshotter_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  base::expected<std::vector<std::vector<uint8_t>>, std::string>
  CaptureScreenshots(content::WebContents* web_contents) {
    base::test::TestFuture<
        base::expected<std::vector<std::vector<uint8_t>>, std::string>>
        future;
    full_screenshotter()->CaptureScreenshots(web_contents,
                                             future.GetCallback());
    return future.Take();
  }

  FullScreenshotter* full_screenshotter() { return full_screenshotter_.get(); }

  content::RenderWidgetHostImpl* GetRenderWidgetHostImpl() const {
    return content::RenderWidgetHostImpl::From(
        web_contents()->GetRenderWidgetHostView()->GetRenderWidgetHost());
  }

  void SetView(content::RenderWidgetHostViewBase* rwhv) {
    GetRenderWidgetHostImpl()->SetView(rwhv);
  }

  void SetSize(const gfx::Size& size) { rwhv_->SetViewBounds(gfx::Rect(size)); }

  void OverrideInterface(LaxMockPaintPreviewRecorder* recorder) {
    blink::AssociatedInterfaceProvider* remote_interfaces =
        web_contents()->GetPrimaryMainFrame()->GetRemoteAssociatedInterfaces();
    remote_interfaces->OverrideBinderForTesting(
        paint_preview::mojom::PaintPreviewRecorder::Name_,
        base::BindRepeating(&LaxMockPaintPreviewRecorder::BindRequest,
                            base::Unretained(recorder)));
  }

  std::unique_ptr<paint_preview::PaintPreviewCompositorService,
                  base::OnTaskRunnerDeleter>
  CreateCompositorService() {
    auto task_runner = base::SingleThreadTaskRunner::GetCurrentDefault();
    return std::unique_ptr<MockPaintPreviewCompositorService,
                           base::OnTaskRunnerDeleter>(
        new MockPaintPreviewCompositorService(task_runner),
        base::OnTaskRunnerDeleter(task_runner));
  }

 private:
  std::unique_ptr<FullScreenshotter> full_screenshotter_;
  std::unique_ptr<TestView> rwhv_;
  raw_ptr<content::RenderWidgetHostViewBase> original_rwhv_ = nullptr;
};

TEST_F(FullScreenshotterTest, InvalidWebContentsAndView) {
  auto result = CaptureScreenshots(nullptr);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "The given web contents is no longer valid");

  for (auto size : {gfx::Size(320, 0), gfx::Size(0, 240)}) {
    SetSize(size);
    auto result2 = CaptureScreenshots(web_contents());
    ASSERT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error(), "No visible render widget host view available");
  }
}

TEST_F(FullScreenshotterTest, NotSupportPdf) {
  static_cast<content::TestWebContents*>(web_contents())
      ->SetMainFrameMimeType("application/pdf");
  auto result = CaptureScreenshots(web_contents());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Do not support pdf capturing");
}

TEST_F(FullScreenshotterTest, CaptureFailedAllErrorStates) {
  const paint_preview::mojom::PaintPreviewStatus kErrorStatuses[] = {
      paint_preview::mojom::PaintPreviewStatus::kAlreadyCapturing,
      paint_preview::mojom::PaintPreviewStatus::kCaptureFailed,
      paint_preview::mojom::PaintPreviewStatus::kGuidCollision,
      paint_preview::mojom::PaintPreviewStatus::kFileCreationError,
      // Covers !paint_preview::CaptureResult.capture_success
      paint_preview::mojom::PaintPreviewStatus::kPartialSuccess,
      paint_preview::mojom::PaintPreviewStatus::kFailed,
  };

  for (auto status : kErrorStatuses) {
    LaxMockPaintPreviewRecorder recorder;
    recorder.SetResponse(base::unexpected(status));
    OverrideInterface(&recorder);

    auto result = CaptureScreenshots(web_contents());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(),
              absl::StrFormat(
                  "Failed to capture a screenshot (CaptureStatus=%d)",
                  static_cast<int>(paint_preview::PaintPreviewBaseService::
                                       CaptureStatus::kCaptureFailed)));
  }
  // We won't get CaptureStatus::kClientCreationFailed since we check
  // WebContents before calling CapturePaintPreview and no
  // CaptureStatus::kContentUnsupported because we don't prodvide policy.
}

TEST_F(FullScreenshotterTest, BeginMainFrameCompositeFailed) {
  auto compositor_service = CreateCompositorService();
  full_screenshotter()->InitCompositorServiceForTest(
      std::move(compositor_service));
  for (auto status : {paint_preview::mojom::PaintPreviewCompositor::
                          BeginCompositeStatus::kCompositingFailure,
                      paint_preview::mojom::PaintPreviewCompositor::
                          BeginCompositeStatus::kDeserializingFailure,
                      paint_preview::mojom::PaintPreviewCompositor::
                          BeginCompositeStatus::kSuccess}) {
    AsMockClient(full_screenshotter()->GetCompositorClientForTest())
        ->SetBeginMainFrameResponseStatus(status);

    LaxMockPaintPreviewRecorder recorder;
    auto response = paint_preview::mojom::PaintPreviewCaptureResponse::New();
    response->geometry_metadata =
        paint_preview::mojom::GeometryMetadataResponse::New();
    response->skp.emplace(mojo_base::BigBuffer(true));
    recorder.SetResponse(std::move(response));
    OverrideInterface(&recorder);

    auto result = CaptureScreenshots(web_contents());
    ASSERT_FALSE(result.has_value());
    if (status == paint_preview::mojom::PaintPreviewCompositor::
                      BeginCompositeStatus::kSuccess) {
      EXPECT_EQ(result.error(), "Root frame data not found");
    } else {
      EXPECT_EQ(result.error(), "BeginMainFrameComposite failed");
    }
  }
}

TEST_F(FullScreenshotterTest, CompositionSucceeded) {
  struct {
    gfx::Size viewport_size;
    gfx::Size page_size;
    size_t num_of_screenshots;
  } test_cases[]{
      {gfx::Size(800, 600), gfx::Size(1024, 768), 2u},
      {gfx::Size(1024, 768), gfx::Size(800, 600), 1u},
      {gfx::Size(1024, 768), gfx::Size(1024, 1536), 2u},
      {gfx::Size(1024, 768), gfx::Size(1024, 3072), 4u},
      {gfx::Size(1024, 768), gfx::Size(2048, 768), 1u},
      {gfx::Size(1024, 768), gfx::Size(2048, 1536), 2u},
      {gfx::Size(2560, 1440), gfx::Size(1024, 768), 1u},
      {gfx::Size(2560, 1440), gfx::Size(2560, 7200), 5u},
  };
  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(testing::Message()
                 << "viewport size: " << test_case.viewport_size.ToString()
                 << "; page size: " << test_case.page_size.ToString()
                 << "; screenshots number: " << test_case.num_of_screenshots);
    SetSize(test_case.viewport_size);
    auto compositor_service = CreateCompositorService();
    full_screenshotter()->InitCompositorServiceForTest(
        std::move(compositor_service));

    auto* client =
        AsMockClient(full_screenshotter()->GetCompositorClientForTest());
    client->SetBeginMainFrameResponseStatus(
        paint_preview::mojom::PaintPreviewCompositor::BeginCompositeStatus::
            kSuccess);

    // Set up frames with root frame data
    base::flat_map<base::UnguessableToken,
                   mojo::StructPtr<paint_preview::mojom::FrameData>>
        frames;
    auto root_frame = paint_preview::mojom::FrameData::New();
    root_frame->scroll_extents = test_case.page_size;
    auto token = base::UnguessableToken::Create();
    frames.insert({token, std::move(root_frame)});
    client->SetCompositeResponse(std::move(frames), token);

    // Setup successful capture response
    LaxMockPaintPreviewRecorder recorder;
    auto response = paint_preview::mojom::PaintPreviewCaptureResponse::New();
    response->geometry_metadata =
        paint_preview::mojom::GeometryMetadataResponse::New();
    response->skp.emplace(mojo_base::BigBuffer(true));
    recorder.SetResponse(std::move(response));
    OverrideInterface(&recorder);

    auto result = CaptureScreenshots(web_contents());
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), test_case.num_of_screenshots);
    EXPECT_TRUE(std::ranges::any_of(
        result.value(), [](const auto& entry) { return !entry.empty(); }));
  }
}

TEST_F(FullScreenshotterTest, BitmapForMainFrameFailed) {
  SetSize(gfx::Size(1024, 768));
  auto compositor_service = CreateCompositorService();
  full_screenshotter()->InitCompositorServiceForTest(
      std::move(compositor_service));
  for (auto status :
       {paint_preview::mojom::PaintPreviewCompositor::BitmapStatus::
            kAllocFailed,
        paint_preview::mojom::PaintPreviewCompositor::BitmapStatus::
            kMissingFrame,
        paint_preview::mojom::PaintPreviewCompositor::BitmapStatus::kSuccess}) {
    auto* client =
        AsMockClient(full_screenshotter()->GetCompositorClientForTest());
    client->SetBeginMainFrameResponseStatus(
        paint_preview::mojom::PaintPreviewCompositor::BeginCompositeStatus::
            kSuccess);
    client->SetBitmapStatus(status);
    if (status ==
        paint_preview::mojom::PaintPreviewCompositor::BitmapStatus::kSuccess) {
      client->SetIsEmptyBitmap(true);
    }

    // Set up frames with root frame data
    base::flat_map<base::UnguessableToken,
                   mojo::StructPtr<paint_preview::mojom::FrameData>>
        frames;
    auto root_frame = paint_preview::mojom::FrameData::New();
    root_frame->scroll_extents = gfx::Size(800, 600);
    auto token = base::UnguessableToken::Create();
    frames.insert({token, std::move(root_frame)});
    client->SetCompositeResponse(std::move(frames), token);

    LaxMockPaintPreviewRecorder recorder;
    auto response = paint_preview::mojom::PaintPreviewCaptureResponse::New();
    response->geometry_metadata =
        paint_preview::mojom::GeometryMetadataResponse::New();
    response->skp.emplace(mojo_base::BigBuffer(true));
    recorder.SetResponse(std::move(response));
    OverrideInterface(&recorder);

    auto result = CaptureScreenshots(web_contents());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(),
              absl::StrFormat("Failed to get bitmap (BitmapStatus=%d)",
                              static_cast<int>(status)));
  }
}

}  // namespace ai_chat
