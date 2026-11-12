/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/peek/peek_features.h"
#include "brave/browser/ui/views/peek/peek_view.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "ui/base/interaction/element_identifier.h"
#include "ui/gfx/geometry/point.h"
#include "url/gurl.h"

namespace {

DEFINE_LOCAL_ELEMENT_IDENTIFIER_VALUE(kSourceTabId);
DEFINE_LOCAL_ELEMENT_IDENTIFIER_VALUE(kPeekTabId);

constexpr char kAddLinkScript[] = R"(() => {
  const a = document.createElement('a');
  a.href = $1;
  a.style = 'position:absolute;left:0;top:0;width:100px;height:100px;' +
            'display:block';
  document.body.appendChild(a);
})";

}  // namespace

class PeekInteractiveBrowserTest : public InteractiveBrowserTest {
 public:
  PeekInteractiveBrowserTest() {
    scoped_features_.InitAndEnableFeature(peek::features::kBravePeek);
  }

  void SetUpOnMainThread() override {
    InteractiveBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 protected:
  // Adds a link to `url` over the top-left of the page and Shift+clicks it.
  auto ShiftClickLink(const GURL& url) {
    return Steps(
        ExecuteJs(kSourceTabId, content::JsReplace(kAddLinkScript, url)),
        WaitForWebContentsPainted(kSourceTabId),
        WithElement(kSourceTabId, [](ui::TrackedElement* el) {
          content::SimulateMouseClickAt(
              AsInstrumentedWebContents(el)->web_contents(),
              blink::WebInputEvent::kShiftKey,
              blink::WebMouseEvent::Button::kLeft, gfx::Point(50, 50));
        }));
  }

  int active_index() { return browser()->GetTabStripModel()->active_index(); }

  int tab_count() { return browser()->GetTabStripModel()->count(); }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(PeekInteractiveBrowserTest, OpenAsTab) {
  const GURL source_url =
      embedded_test_server()->GetURL("a.com", "/title1.html");
  const GURL peek_url = embedded_test_server()->GetURL("b.com", "/title2.html");

  RunTestSequence(
      InstrumentTab(kSourceTabId),
      NavigateWebContents(kSourceTabId, source_url), ShiftClickLink(peek_url),
      // The Peek card is in its own widget, so it has its own context.
      InAnyContext(WaitForShow(PeekView::kCardElementId)),
      InstrumentNextTab(kPeekTabId),
      InAnyContext(PressButton(PeekView::kOpenAsTabButtonElementId)),
      InAnyContext(WaitForHide(PeekView::kCardElementId)),
      WaitForWebContentsReady(kPeekTabId, peek_url),
      CheckResult([this] { return tab_count(); }, 2),
      CheckResult([this] { return active_index(); }, 1));
}

IN_PROC_BROWSER_TEST_F(PeekInteractiveBrowserTest, Close) {
  const GURL source_url =
      embedded_test_server()->GetURL("a.com", "/title1.html");
  const GURL peek_url = embedded_test_server()->GetURL("b.com", "/title2.html");

  RunTestSequence(InstrumentTab(kSourceTabId),
                  NavigateWebContents(kSourceTabId, source_url),
                  ShiftClickLink(peek_url),
                  InAnyContext(WaitForShow(PeekView::kCardElementId)),
                  InAnyContext(PressButton(PeekView::kCloseButtonElementId)),
                  InAnyContext(WaitForHide(PeekView::kCardElementId)),
                  CheckResult([this] { return tab_count(); }, 1),
                  CheckResult([this] { return active_index(); }, 0));
}
