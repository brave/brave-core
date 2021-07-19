/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

#include "third_party/blink/public/common/page/launching_process_state.h"
#include "third_party/blink/public/web/web_script_execution_callback.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/renderer/core/frame/frame_test_helpers.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"

const char kStorageEstimateTest[] = "storage_estimate.html";

/*class NavigatorStorageEstimateQuotaTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
  }
};

IN_PROC_BROWSER_TEST_F(NavigatorStorageEstimateQuotaTest, Is2Gb) {
  GURL url = embedded_test_server()->GetURL(kStorageEstimateTest);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  EXPECT_EQ(true, content::EvalJs(contents->GetMainFrame(),
                                  "getStorageEstimateIs2Gb()"));
}*/

class ScriptExecutionCallbackHelper : public blink::WebScriptExecutionCallback {
 public:
  explicit ScriptExecutionCallbackHelper(v8::Local<v8::Context> context)
      : did_complete_(false), bool_value_(false), context_(context) {}
  ~ScriptExecutionCallbackHelper() override = default;

  bool DidComplete() const { return did_complete_; }
  const String& StringValue() const { return string_value_; }
  bool BoolValue() { return bool_value_; }

 private:
  // blink::WebScriptExecutionCallback:
  void Completed(const blink::WebVector<v8::Local<v8::Value>>& values) override {
    did_complete_ = true;
    if (!values.empty()) {
      if (values[0]->IsString()) {
        string_value_ =
            blink::ToCoreString(values[0]->ToString(context_).ToLocalChecked());
      } else if (values[0]->IsBoolean()) {
        bool_value_ = values[0].As<v8::Boolean>()->Value();
      }
    }
  }

  bool did_complete_;
  String string_value_;
  bool bool_value_;
  v8::Local<v8::Context> context_;
};

class NavigatorStorageEstimateQuotaTest : public testing::Test {
 protected:
  NavigatorStorageEstimateQuotaTest()
      : base_url_("http://internal.test/")/*,
        not_base_url_("http://external.test/"),
        chrome_url_("chrome://")*/ {}

  /*~NavigatorStorageEstimateQuotaTest() override {
    url_test_helpers::UnregisterAllURLsAndClearMemoryCache();
  }*/

  void DisableRendererSchedulerThrottling() {
    // Make sure that the RendererScheduler is foregrounded to avoid getting
    // throttled.
    if (blink::kLaunchingProcessIsBackgrounded) {
      blink::ThreadScheduler::Current()
          ->GetWebMainThreadSchedulerForTest()
          ->SetRendererBackgrounded(false);
    }
  }

  void RegisterMockedHttpURLLoad(const std::string& file_name) {
    // TODO(crbug.com/751425): We should use the mock functionality
    // via the WebViewHelper instance in each test case.
    RegisterMockedURLLoadFromBase(base_url_, file_name);
  }

  /*void RegisterMockedChromeURLLoad(const std::string& file_name) {
    // TODO(crbug.com/751425): We should use the mock functionality
    // via the WebViewHelper instance in each test case.
    RegisterMockedURLLoadFromBase(chrome_url_, file_name);
  }*/

  void RegisterMockedURLLoadFromBase(const std::string& base_url,
                                     const std::string& file_name) {
    // TODO(crbug.com/751425): We should use the mock functionality
    // via the WebViewHelper instance in each test case.
    //base::FilePath test_data_dir;
    //base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
      blink::url_test_helpers::RegisterMockedURLLoadFromBase(
        blink::WebString::FromUTF8(base_url), blink::WebString::FromUTF8(blink::test::CoreTestDataPath().Utf8()),
        blink::WebString::FromUTF8(file_name));
  }

  /*void RegisterMockedURLLoadWithCustomResponse(const WebURL& full_url,
                                               const WebString& file_path,
                                               WebURLResponse response) {
    url_test_helpers::RegisterMockedURLLoadWithCustomResponse(
        full_url, file_path, response);
  }

  void RegisterMockedHttpURLLoadWithCSP(const std::string& file_name,
                                        const std::string& csp,
                                        bool report_only = false) {
    WebURLResponse response;
    response.SetMimeType("text/html");
    response.AddHttpHeaderField(
        report_only ? WebString("Content-Security-Policy-Report-Only")
                    : WebString("Content-Security-Policy"),
        WebString::FromUTF8(csp));
    std::string full_string = base_url_ + file_name;
    RegisterMockedURLLoadWithCustomResponse(
        ToKURL(full_string),
        test::CoreTestDataPath(WebString::FromUTF8(file_name)), response);
  }*/

  /*void RegisterMockedHttpURLLoadWithMimeType(const std::string& file_name,
                                             const std::string& mime_type) {
    // TODO(crbug.com/751425): We should use the mock functionality
    // via the WebViewHelper instance in each test case.
    url_test_helpers::RegisterMockedURLLoadFromBase(
        WebString::FromUTF8(base_url_), test::CoreTestDataPath(),
        WebString::FromUTF8(file_name), WebString::FromUTF8(mime_type));
  }*/

  /*static void ConfigureCompositingWebView(WebSettings* settings) {
    settings->SetPreferCompositingToLCDTextEnabled(true);
  }

  static void ConfigureAndroid(WebSettings* settings) {
    settings->SetViewportMetaEnabled(true);
    settings->SetViewportEnabled(true);
    settings->SetMainFrameResizesAreOrientationChanges(true);
    settings->SetShrinksViewportContentToFit(true);
    settings->SetViewportStyle(web_pref::ViewportStyle::kMobile);
  }

  static void ConfigureLoadsImagesAutomatically(WebSettings* settings) {
    settings->SetLoadsImagesAutomatically(true);
  }

  void InitializeTextSelectionWebView(
      const std::string& url,
      frame_test_helpers::WebViewHelper* web_view_helper) {
    web_view_helper->InitializeAndLoad(url);
    web_view_helper->GetWebView()->GetSettings()->SetDefaultFontSize(12);
    web_view_helper->Resize(WebSize(640, 480));
  }

  std::unique_ptr<DragImage> NodeImageTestSetup(
      frame_test_helpers::WebViewHelper* web_view_helper,
      const std::string& testcase) {
    RegisterMockedHttpURLLoad("nodeimage.html");
    web_view_helper->InitializeAndLoad(base_url_ + "nodeimage.html");
    web_view_helper->Resize(WebSize(640, 480));
    auto* frame =
        To<LocalFrame>(web_view_helper->GetWebView()->GetPage()->MainFrame());
    DCHECK(frame);
    Element* element = frame->GetDocument()->getElementById(testcase.c_str());
    return DataTransfer::NodeImage(*frame, *element);
  }

  void RemoveElementById(WebLocalFrameImpl* frame, const AtomicString& id) {
    Element* element = frame->GetFrame()->GetDocument()->getElementById(id);
    DCHECK(element);
    element->remove();
  }*/

  // Both sets the inner html and runs the document lifecycle.
  /*void InitializeWithHTML(blink::LocalFrame& frame, const String& html_content) {
    frame.GetDocument()->body()->setInnerHTML(html_content);
    frame.GetDocument()->View()->UpdateAllLifecyclePhases(
        DocumentUpdateReason::kTest);
  }*/

  /*void SwapAndVerifyFirstChildConsistency(const char* const message,
                                          WebFrame* parent,
                                          WebFrame* new_child);
  void SwapAndVerifyMiddleChildConsistency(const char* const message,
                                           WebFrame* parent,
                                           WebFrame* new_child);
  void SwapAndVerifyLastChildConsistency(const char* const message,
                                         WebFrame* parent,
                                         WebFrame* new_child);
  void SwapAndVerifySubframeConsistency(const char* const message,
                                        WebFrame* parent,
                                        WebFrame* new_child);

  int NumMarkersInRange(const Document* document,
                        const EphemeralRange& range,
                        DocumentMarker::MarkerTypes marker_types) {
    Node* start_container = range.StartPosition().ComputeContainerNode();
    unsigned start_offset = static_cast<unsigned>(
        range.StartPosition().ComputeOffsetInContainerNode());

    Node* end_container = range.EndPosition().ComputeContainerNode();
    unsigned end_offset = static_cast<unsigned>(
        range.EndPosition().ComputeOffsetInContainerNode());

    int node_count = 0;
    for (Node& node : range.Nodes()) {
      const DocumentMarkerVector& markers_in_node =
          document->Markers().MarkersFor(To<Text>(node), marker_types);
      node_count += std::count_if(
          markers_in_node.begin(), markers_in_node.end(),
          [start_offset, end_offset, &node, &start_container,
           &end_container](const DocumentMarker* marker) {
            if (node == start_container && marker->EndOffset() <= start_offset)
              return false;
            if (node == end_container && marker->StartOffset() >= end_offset)
              return false;
            return true;
          });
    }

    return node_count;
  }*/

  /*void UpdateAllLifecyclePhases(WebViewImpl* web_view) {
    web_view->MainFrameWidget()->UpdateAllLifecyclePhases(
        DocumentUpdateReason::kTest);
  }*/

  /*static void GetElementAndCaretBoundsForFocusedEditableElement(
      frame_test_helpers::WebViewHelper& helper,
      IntRect& element_bounds,
      IntRect& caret_bounds) {
    Element* element = helper.GetWebView()->FocusedElement();
    WebRect caret_in_viewport, unused;
    helper.GetWebView()->MainFrameWidget()->SelectionBounds(caret_in_viewport,
                                                            unused);
    caret_bounds =
        helper.GetWebView()->GetPage()->GetVisualViewport().ViewportToRootFrame(
            caret_in_viewport);
    element_bounds = element->GetDocument().View()->ConvertToRootFrame(
        PixelSnappedIntRect(element->Node::BoundingBox()));
  }*/

  std::string base_url_;
  //std::string not_base_url_;
  //std::string chrome_url_;
};

TEST_F(NavigatorStorageEstimateQuotaTest, Is2Gb) {
  DisableRendererSchedulerThrottling();
  RegisterMockedHttpURLLoad(kStorageEstimateTest);

  blink::frame_test_helpers::WebViewHelper web_view_helper;
  web_view_helper.InitializeAndLoad(base_url_ + kStorageEstimateTest);

  v8::HandleScope scope(v8::Isolate::GetCurrent());

  // Suspend scheduled tasks so the script doesn't run.
  web_view_helper.GetWebView()->GetPage()->SetPaused(true);
  blink::LocalFrame::NotifyUserActivation(
      web_view_helper.LocalMainFrame()->GetFrame(),
      blink::mojom::UserActivationNotificationType::kTest);
  ScriptExecutionCallbackHelper callback_helper(
      web_view_helper.LocalMainFrame()->MainWorldScriptContext());
  blink::WebScriptSource script_source("navigator.userActivation.isActive;");
                                  //"getStorageEstimateIs2Gb()";
  web_view_helper.GetWebView()
      ->MainFrameImpl()
      ->RequestExecuteScriptAndReturnValue(script_source, false,
                                           &callback_helper);
  blink::test::RunPendingTasks();
  EXPECT_FALSE(callback_helper.DidComplete());

  web_view_helper.GetWebView()->GetPage()->SetPaused(false);
  blink::test::RunPendingTasks();
  EXPECT_TRUE(callback_helper.DidComplete());
  EXPECT_EQ(true, callback_helper.BoolValue());
}
