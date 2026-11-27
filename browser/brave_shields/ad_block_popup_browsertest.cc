// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <set>
#include <string>
#include <utility>

#include "base/callback_list.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/synchronization/lock.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/simple_test_clock.h"
#include "base/thread_annotations.h"
#include "base/time/time.h"
#include "brave/browser/brave_shields/ad_block_service_browsertest.h"
#include "brave/browser/brave_shields/brave_shields_settings_service_factory.h"
#include "brave/components/brave_shields/content/browser/ad_block_popup_navigation_throttle.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/blocked_content/popup_blocker_tab_helper.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/hit_test_region_observer.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "third_party/blink/public/common/switches.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/point_f.h"
#include "url/gurl.h"
#include "url/url_constants.h"

using blocked_content::PopupBlockerTabHelper;
using brave_shields::AdBlockPopupNavigationThrottle;
using brave_shields::features::kBraveAdblockPopupBlocking;
using brave_shields::mojom::AdBlockMode;
using brave_shields::prefs::kAdsBlocked;

namespace {

class CommitObserver : public content::WebContentsObserver {
 public:
  explicit CommitObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}

  bool committed() const { return committed_; }

 private:
  // content::WebContentsObserver:
  void DidFinishNavigation(content::NavigationHandle* handle) override {
    if (handle->IsInPrimaryMainFrame() && handle->HasCommitted() &&
        handle->GetURL().SchemeIsHTTPOrHTTPS()) {
      committed_ = true;
    }
  }

  bool committed_ = false;
};

// Waits until `popup` either commits an http(s) page or is closed, and returns
// true if it was closed.
bool WaitForBlocked(
    content::WebContents* popup,
    const base::Location& location = base::Location::Current()) {
  SCOPED_TRACE(location.ToString());
  content::WebContentsDestroyedWatcher destroyed(popup);
  CommitObserver commit(popup);
  EXPECT_TRUE(base::test::RunUntil(
      [&] { return destroyed.IsDestroyed() || commit.committed(); }));
  return destroyed.IsDestroyed();
}

// The modifier that opens a link in a background tab.
constexpr int kNewTabModifier =
#if BUILDFLAG(IS_MAC)
    blink::WebInputEvent::kMetaKey;
#else
    blink::WebInputEvent::kControlKey;
#endif

}  // namespace

class AdBlockPopupTestBase : public AdBlockServiceTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    AdBlockServiceTest::SetUpCommandLine(command_line);
    // Otherwise clicks sent while a page is still loading are dropped.
    command_line->AppendSwitch(blink::switches::kAllowPreCommitInput);
  }

  GURL Url(const std::string& host) {
    return embedded_test_server()->GetURL(host, "/simple.html");
  }

  // A URL on `host` that redirects to `target`.
  GURL RedirectUrl(const std::string& host, const std::string& target) {
    return embedded_test_server()->GetURL(
        host, "/cross-site/" + target + "/simple.html");
  }

  brave_shields::BraveShieldsSettingsService* settings() {
    return BraveShieldsSettingsServiceFactory::GetForProfile(profile());
  }

  // Runs `open`, which must make `opener` open a window, and returns true if
  // that window was closed before it committed a page. Otherwise closes it. A
  // closed window must be listed in the blocked-popup UI.
  bool OpensBlockedPopup(
      content::WebContents* opener,
      base::OnceClosure open,
      const base::Location& location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    PopupBlockerTabHelper* popup_blocker =
        PopupBlockerTabHelper::FromWebContents(opener);
    const size_t blocked_count = popup_blocker->GetBlockedPopupsCount();
    TabStripModel* tabs = browser()->tab_strip_model();
    const int tab_count = tabs->count();

    content::WebContents* popup = WaitForPopup(opener, std::move(open));
    const bool blocked = WaitForBlocked(popup);

    if (blocked) {
      EXPECT_EQ(blocked_count + 1, popup_blocker->GetBlockedPopupsCount());
    } else {
      EXPECT_EQ(blocked_count, popup_blocker->GetBlockedPopupsCount());
      // A popup window isn't in the tab strip.
      content::WebContentsDestroyedWatcher destroyed(popup);
      popup->Close();
      destroyed.Wait();
    }

    EXPECT_EQ(tab_count, tabs->count());
    return blocked;
  }

  bool OpensBlockedPopup(
      const content::ToRenderFrameHost& opener,
      const std::string& script,
      const base::Location& location = base::Location::Current()) {
    return OpensBlockedPopup(
        content::WebContents::FromRenderFrameHost(opener.render_frame_host()),
        base::BindLambdaForTesting(
            [&] { content::ExecuteScriptAsync(opener, script); }),
        location);
  }

  bool OpensBlockedPopup(
      const std::string& script,
      const base::Location& location = base::Location::Current()) {
    return OpensBlockedPopup(web_contents(), script, location);
  }

  bool WindowOpenIsBlocked(
      const content::ToRenderFrameHost& opener,
      const GURL& url,
      const base::Location& location = base::Location::Current()) {
    return OpensBlockedPopup(opener, content::JsReplace("window.open($1)", url),
                             location);
  }

  bool WindowOpenIsBlocked(
      const GURL& url,
      const base::Location& location = base::Location::Current()) {
    return WindowOpenIsBlocked(web_contents(), url, location);
  }

  // Makes `opener` open a window for `url`, and returns it once it has loaded.
  content::WebContents* OpenPopup(const content::ToRenderFrameHost& opener,
                                  const GURL& url) {
    content::WebContents* popup = WaitForPopup(
        content::WebContents::FromRenderFrameHost(opener.render_frame_host()),
        base::BindLambdaForTesting([&] {
          content::ExecuteScriptAsync(
              opener, content::JsReplace("window.open($1)", url));
        }));
    EXPECT_FALSE(WaitForBlocked(popup));
    return popup;
  }

  // Makes `popup` navigate itself to `url`, and returns true if it was closed.
  bool NavigatesItselfIsBlocked(
      content::WebContents* popup,
      const GURL& url,
      const base::Location& location = base::Location::Current()) {
    content::ExecuteScriptAsync(popup,
                                content::JsReplace("location = $1", url));
    return WaitForBlocked(popup, location);
  }

  // Loads a page with a link to `url` with the id `link`.
  void LoadPageWithLink(const GURL& url,
                        const std::string& rel = "",
                        const std::string& target = "_blank") {
    NavigateToURL(Url("a.com"));
    ASSERT_TRUE(
        content::ExecJs(web_contents(), content::JsReplace(R"(
          const a = document.createElement('a');
          a.id = 'link';
          a.href = $1;
          a.rel = $2;
          a.target = $3;
          a.textContent = 'link';
          document.body.appendChild(a);
        )",
                                                           url, rel, target)));
  }

  // Sends a real click to the center of the element with the id `id`.
  void Click(
      const std::string& id,
      blink::WebMouseEvent::Button button = blink::WebMouseEvent::Button::kLeft,
      int modifiers = 0) {
    const content::EvalJsResult result =
        content::EvalJs(web_contents(), content::JsReplace(R"(
          const r = document.getElementById($1).getBoundingClientRect();
          [r.left + r.width / 2, r.top + r.height / 2]
        )",
                                                           id));
    const auto& xy = result.ExtractList();
    content::WaitForHitTestData(web_contents()->GetPrimaryMainFrame());
    content::SimulateMouseClickAt(
        web_contents(), modifiers, button,
        gfx::ToFlooredPoint(gfx::PointF(xy[0].GetDouble(), xy[1].GetDouble())));
  }

  // Clicks the element with the id `id` and returns whether the window it
  // opened was blocked.
  bool ClickIsBlocked(
      const std::string& id,
      const base::Location& location = base::Location::Current()) {
    return OpensBlockedPopup(web_contents(),
                             base::BindLambdaForTesting([&] { Click(id); }),
                             location);
  }

  // Adds a frame to the active tab and returns it once it has loaded.
  content::RenderFrameHost* AddFrame(const GURL& url,
                                     const std::string& sandbox) {
    const std::string script = R"(
      new Promise(resolve => {
        const e = document.createElement('iframe');
        if ($2) {
          e.sandbox = $2;
        }
        e.src = $1;
        e.onload = () => resolve(document.querySelectorAll('iframe').length);
        document.body.appendChild(e);
      })
    )";
    const int count = content::EvalJs(web_contents(),
                                      content::JsReplace(script, url, sandbox))
                          .ExtractInt();
    return content::ChildFrameAt(web_contents()->GetPrimaryMainFrame(),
                                 count - 1);
  }

  // The URL of the last popup listed in `opener`'s blocked-popup UI.
  GURL LastBlockedPopupUrl(content::WebContents* opener) {
    auto requests = PopupBlockerTabHelper::FromWebContents(opener)
                        ->GetBlockedPopupRequests();
    return requests.empty() ? GURL() : requests.rbegin()->second;
  }

 protected:
  // Runs `open` and returns the window it makes `opener` open.
  content::WebContents* WaitForPopup(content::WebContents* opener,
                                     base::OnceClosure open) {
    content::WebContents* popup = nullptr;
    base::RunLoop run_loop;
    // Not WebContentsAddedObserver, which takes a WebContents the browser
    // creates for something else.
    base::CallbackListSubscription subscription =
        content::RegisterWebContentsCreationCallback(
            base::BindLambdaForTesting([&](content::WebContents* contents) {
              if (!popup &&
                  contents->GetFirstWebContentsInLiveOriginalOpenerChain() ==
                      opener) {
                popup = contents;
                run_loop.Quit();
              }
            }));
    std::move(open).Run();
    run_loop.Run();
    return popup;
  }
};

class AdBlockPopupTest : public AdBlockPopupTestBase {
 public:
  AdBlockPopupTest() {
    feature_list_.InitAndEnableFeature(kBraveAdblockPopupBlocking);
  }

  void SetUpOnMainThread() override {
    embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
        &AdBlockPopupTest::OnRequest, base::Unretained(this)));
    AdBlockPopupTestBase::SetUpOnMainThread();
  }

  // Whether the test server got a request for `host`.
  bool WasRequested(const std::string& host) {
    base::AutoLock lock(lock_);
    return requested_hosts_.contains(host);
  }

  // Middle-clicks or modifier-clicks the element with the id `id`, and returns
  // the tab it opened once it has loaded.
  content::WebContents* OpenInNewTab(const std::string& id,
                                     blink::WebMouseEvent::Button button,
                                     int modifiers) {
    ui_test_utils::TabAddedWaiter added(browser());
    Click(id, button, modifiers);
    content::WebContents* tab = added.Wait();
    EXPECT_TRUE(content::WaitForLoadStop(tab));
    return tab;
  }

 private:
  void OnRequest(const net::test_server::HttpRequest& request) {
    auto host = request.headers.find("Host");

    if (host == request.headers.end()) {
      return;
    }

    base::AutoLock lock(lock_);
    requested_hosts_.emplace(
        GURL(base::StrCat({url::kHttpScheme, url::kStandardSchemeSeparator,
                           host->second}))
            .host());
  }

  base::test::ScopedFeatureList feature_list_;
  base::Lock lock_;
  std::set<std::string> requested_hosts_ GUARDED_BY(lock_);
};

class AdBlockPopupDisabledTest : public AdBlockPopupTestBase {
 public:
  AdBlockPopupDisabledTest() {
    feature_list_.InitAndDisableFeature(kBraveAdblockPopupBlocking);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, WindowOpen) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));

  const uint64_t ads_blocked_count =
      profile()->GetPrefs()->GetUint64(kAdsBlocked);
  EXPECT_TRUE(WindowOpenIsBlocked(Url("b.com")));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked),
            ads_blocked_count + 1);
  EXPECT_EQ(Url("b.com"), LastBlockedPopupUrl(web_contents()));
  EXPECT_FALSE(WasRequested("b.com"));

  EXPECT_FALSE(WindowOpenIsBlocked(Url("c.com")));
  EXPECT_TRUE(WasRequested("c.com"));
}

IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, WindowOpenNoopener) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));
  EXPECT_TRUE(OpensBlockedPopup(content::JsReplace(
      "window.open($1, '_blank', 'noopener')", Url("b.com"))));
  EXPECT_FALSE(WasRequested("b.com"));
}

IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, WindowOpenPopupWindow) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));
  EXPECT_TRUE(OpensBlockedPopup(
      content::JsReplace("window.open($1, '_blank', 'popup')", Url("b.com"))));
  EXPECT_FALSE(WasRequested("b.com"));
}

// The page gets a window that closes, as it does when uBO closes a popup.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, ReturnsClosedWindow) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));

  EXPECT_TRUE(OpensBlockedPopup(content::JsReplace(R"(
    window.w = window.open($1);
    window.ok = w !== null && !navigator.userActivation.isActive;
  )",
                                                   Url("b.com"))));
  EXPECT_EQ(true, content::EvalJs(web_contents(), "ok"));

  EXPECT_EQ(true, content::EvalJs(web_contents(), R"(
    new Promise(resolve => {
      const id = setInterval(() => {
        if (w.closed) {
          clearInterval(id);
          resolve(true);
        }
      }, 10);
    })
  )"));
}

// The user can open a blocked popup from the blocked-popup UI.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, OpenFromBlockedPopupUI) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));
  ASSERT_TRUE(WindowOpenIsBlocked(Url("b.com")));

  PopupBlockerTabHelper* popup_blocker =
      PopupBlockerTabHelper::FromWebContents(web_contents());
  auto requests = popup_blocker->GetBlockedPopupRequests();
  ASSERT_EQ(1u, requests.size());

  ui_test_utils::TabAddedWaiter added(browser());
  popup_blocker->ShowBlockedPopup(requests.begin()->first,
                                  WindowOpenDisposition::kNewForegroundTab);
  content::WebContents* popup = added.Wait();
  EXPECT_FALSE(WaitForBlocked(popup));
  EXPECT_EQ(Url("b.com"), popup->GetLastCommittedURL());
  EXPECT_TRUE(WasRequested("b.com"));
}

IN_PROC_BROWSER_TEST_F(AdBlockPopupDisabledTest, WindowOpen) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));
  EXPECT_FALSE(WindowOpenIsBlocked(Url("b.com")));
}

// A popup rule doesn't block loading the same URL in the current tab.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, NotAPopup) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("b.com"));
  EXPECT_EQ(Url("b.com"), web_contents()->GetLastCommittedURL());

  NavigateToURL(Url("a.com"));
  EXPECT_TRUE(content::NavigateToURLFromRenderer(web_contents(), Url("b.com")));
  EXPECT_EQ(Url("b.com"), web_contents()->GetLastCommittedURL());
}

// `third-party` and `domain=` are checked against the page that opened the
// popup.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, OpenerIsSource) {
  UpdateAdBlockInstanceWithRules(
      "||b.com^$popup,third-party\n||c.com^$popup,domain=a.com");

  NavigateToURL(Url("a.com"));
  EXPECT_TRUE(WindowOpenIsBlocked(Url("b.com")));
  EXPECT_TRUE(WindowOpenIsBlocked(Url("c.com")));

  NavigateToURL(Url("b.com"));
  EXPECT_FALSE(WindowOpenIsBlocked(Url("b.com")));
  EXPECT_FALSE(WindowOpenIsBlocked(Url("c.com")));
}

// A popup opened from a cross-site frame is checked against the frame.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, OpenedFromFrame) {
  UpdateAdBlockInstanceWithRules("||c.com^$popup,domain=b.com");
  NavigateToURL(Url("a.com"));
  EXPECT_FALSE(WindowOpenIsBlocked(Url("c.com")));
  EXPECT_TRUE(WindowOpenIsBlocked(AddFrame(Url("b.com"), ""), Url("c.com")));
  EXPECT_EQ(Url("c.com"), LastBlockedPopupUrl(web_contents()));
}

// A sandboxed frame has an opaque origin, which matches no `domain=`, so only
// rules without one apply.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, OpenedFromSandboxedFrame) {
  UpdateAdBlockInstanceWithRules(
      "||c.com^$popup,domain=b.com\n"
      "||d.com^$popup\n"
      "@@||d.com^$popup,domain=b.com");
  NavigateToURL(Url("a.com"));
  content::RenderFrameHost* frame =
      AddFrame(Url("b.com"), "allow-scripts allow-popups");
  ASSERT_TRUE(frame->GetLastCommittedOrigin().opaque());
  EXPECT_FALSE(WindowOpenIsBlocked(frame, Url("c.com")));
  EXPECT_TRUE(WindowOpenIsBlocked(frame, Url("d.com")));
  EXPECT_FALSE(WasRequested("d.com"));
}

// A popup opened by a blank window uses the site that opened the blank window,
// which the blank window's origin comes from.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, OpenedFromBlankWindow) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup,domain=a.com");
  NavigateToURL(Url("a.com"));
  content::WebContents* blank = WaitForPopup(
      web_contents(), base::BindLambdaForTesting([&] {
        content::ExecuteScriptAsync(web_contents(), "window.open()");
      }));
  EXPECT_TRUE(WindowOpenIsBlocked(blank, Url("b.com")));
}

// In standard mode third-party popups are blocked and first-party popups are
// left alone, like other first-party requests.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, StandardMode) {
  UpdateAdBlockInstanceWithRules("||a.com^$popup\n||b.com^$popup");
  settings()->SetAdBlockMode(AdBlockMode::STANDARD, Url("a.com"));
  NavigateToURL(Url("a.com"));
  EXPECT_TRUE(WindowOpenIsBlocked(Url("b.com")));
  EXPECT_FALSE(WindowOpenIsBlocked(Url("a.com")));

  settings()->SetAdBlockMode(AdBlockMode::AGGRESSIVE, Url("a.com"));
  EXPECT_TRUE(WindowOpenIsBlocked(Url("a.com")));
}

// Only the opener's Shields settings apply, as uBO only checks whether the
// opener is trusted.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, ShieldsSettings) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));

  settings()->SetBraveShieldsEnabled(false, Url("b.com"));
  EXPECT_TRUE(WindowOpenIsBlocked(Url("b.com")));

  settings()->SetBraveShieldsEnabled(false, Url("a.com"));
  EXPECT_FALSE(WindowOpenIsBlocked(Url("b.com")));

  settings()->SetBraveShieldsEnabled(true, Url("a.com"));
  settings()->SetAdBlockMode(AdBlockMode::ALLOW, Url("a.com"));
  EXPECT_FALSE(WindowOpenIsBlocked(Url("b.com")));
}

// A frame navigating the window it opened is checked against the frame, not its
// tab.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, FrameNavigatesPopup) {
  UpdateAdBlockInstanceWithRules(
      "||c.com^$popup,domain=b.com\n||d.com^$popup,domain=a.com");
  NavigateToURL(Url("a.com"));
  content::RenderFrameHost* frame = AddFrame(Url("b.com"), "");
  const std::string script = R"(
    {
      const w = window.open('about:blank');
      setTimeout(() => w.location = $1, 100);
    }
  )";
  EXPECT_TRUE(
      OpensBlockedPopup(frame, content::JsReplace(script, Url("c.com"))));
  EXPECT_FALSE(
      OpensBlockedPopup(frame, content::JsReplace(script, Url("d.com"))));
  EXPECT_FALSE(WasRequested("c.com"));
}

// A popup navigating itself is checked against the frame that opened it.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, NavigatesItselfOpenedFromFrame) {
  UpdateAdBlockInstanceWithRules(
      "||c.com^$popup,domain=b.com\n||d.com^$popup,domain=a.com");
  NavigateToURL(Url("a.com"));
  content::WebContents* tab = web_contents();
  content::RenderFrameHost* frame = AddFrame(Url("b.com"), "");
  EXPECT_TRUE(
      NavigatesItselfIsBlocked(OpenPopup(frame, Url("e.com")), Url("c.com")));
  EXPECT_EQ(Url("c.com"), LastBlockedPopupUrl(tab));
  EXPECT_FALSE(WasRequested("c.com"));
  EXPECT_FALSE(
      NavigatesItselfIsBlocked(OpenPopup(frame, Url("e.com")), Url("d.com")));
}

// A popup navigating itself is checked against the page that opened it, with
// that site's current settings, after the opener tab has gone to another site.
// The other site doesn't get the blocked-popup UI.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, NavigatesItselfAfterOpenerNavigates) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup,domain=a.com");
  // The popups open as the active tab.
  content::WebContents* tab = web_contents();

  ASSERT_TRUE(content::NavigateToURL(tab, Url("a.com")));
  content::WebContents* popup = OpenPopup(tab, Url("e.com"));
  ASSERT_TRUE(content::NavigateToURL(tab, Url("f.com")));
  EXPECT_TRUE(NavigatesItselfIsBlocked(popup, Url("b.com")));
  EXPECT_FALSE(WasRequested("b.com"));
  EXPECT_EQ(
      0u, PopupBlockerTabHelper::FromWebContents(tab)->GetBlockedPopupsCount());

  ASSERT_TRUE(content::NavigateToURL(tab, Url("a.com")));
  popup = OpenPopup(tab, Url("e.com"));
  ASSERT_TRUE(content::NavigateToURL(tab, Url("f.com")));
  settings()->SetBraveShieldsEnabled(false, Url("a.com"));
  EXPECT_FALSE(NavigatesItselfIsBlocked(popup, Url("b.com")));
}

// A popup navigating itself is still checked after its opener tab is closed.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, NavigatesItselfAfterOpenerClosed) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup,domain=a.com");
  NavigateToURL(Url("a.com"));
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), Url("a.com"), WindowOpenDisposition::kNewForegroundTab,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  content::WebContents* tab = web_contents();
  content::WebContents* popup = OpenPopup(tab, Url("e.com"));
  TabStripModel* tabs = browser()->tab_strip_model();
  content::WebContentsDestroyedWatcher tab_closed(tab);
  tabs->CloseWebContentsAt(tabs->GetIndexOfWebContents(tab), 0);
  tab_closed.Wait();
  EXPECT_TRUE(NavigatesItselfIsBlocked(popup, Url("b.com")));
  EXPECT_FALSE(WasRequested("b.com"));
}

// A blank window that its opener fills with a script, which then navigates it,
// is checked against the frame that opened it.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, BlankWindowNavigatesItself) {
  UpdateAdBlockInstanceWithRules(
      "||c.com^$popup,domain=b.com\n||d.com^$popup,domain=a.com");
  NavigateToURL(Url("a.com"));
  content::RenderFrameHost* frame = AddFrame(Url("b.com"), "");
  const std::string script = R"(
    {
      const w = window.open();
      const script = w.document.createElement('script');
      script.textContent = 'location = ' + JSON.stringify($1);
      w.document.body.appendChild(script);
    }
  )";
  EXPECT_TRUE(
      OpensBlockedPopup(frame, content::JsReplace(script, Url("c.com"))));
  EXPECT_FALSE(WasRequested("c.com"));
  EXPECT_FALSE(
      OpensBlockedPopup(frame, content::JsReplace(script, Url("d.com"))));
  EXPECT_TRUE(OpensBlockedPopup(content::JsReplace(script, Url("d.com"))));
}

IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, BlankThenNavigate) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));
  EXPECT_TRUE(OpensBlockedPopup(content::JsReplace(
      "{ const w = window.open(); w.location = $1; }", Url("b.com"))));
  EXPECT_TRUE(OpensBlockedPopup(content::JsReplace(R"(
    {
      const w = window.open('about:blank');
      setTimeout(() => w.location = $1, 100);
    }
  )",
                                                   Url("b.com"))));
  EXPECT_FALSE(WasRequested("b.com"));
}

// Only the redirect target matches, so the first hop is requested and the
// redirect isn't followed. uBO also checks after a redirect.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, WindowOpenRedirect) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));
  const GURL redirect = RedirectUrl("c.com", "b.com");
  EXPECT_TRUE(
      OpensBlockedPopup(content::JsReplace("window.open($1)", redirect)));
  EXPECT_TRUE(WasRequested("c.com"));
  EXPECT_FALSE(WasRequested("b.com"));
  EXPECT_EQ(redirect, LastBlockedPopupUrl(web_contents()));
}

IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, ClickedLinkRedirect) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  LoadPageWithLink(RedirectUrl("c.com", "b.com"));
  EXPECT_TRUE(ClickIsBlocked("link"));
  EXPECT_TRUE(WasRequested("c.com"));
  EXPECT_FALSE(WasRequested("b.com"));
}

// A popup that loads a page and then navigates itself is checked again, as
// long as it navigates within 10 s of its last page load.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, NavigatesItself) {
  base::SimpleTestClock clock;
  AdBlockPopupNavigationThrottle::SetClockForTesting(&clock);
  base::ScopedClosureRunner reset_clock(base::BindOnce(
      [] { AdBlockPopupNavigationThrottle::SetClockForTesting(nullptr); }));
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));
  content::WebContents* tab = web_contents();

  for (const base::TimeDelta delay : {base::Seconds(10), base::Seconds(11)}) {
    SCOPED_TRACE(delay);
    content::WebContents* popup = OpenPopup(tab, Url("c.com"));
    clock.SetNow(
        popup->GetController().GetLastCommittedEntry()->GetTimestamp() + delay);
    EXPECT_EQ(delay == base::Seconds(10),
              NavigatesItselfIsBlocked(popup, Url("b.com")));
  }
}

// A window that goes 10 s without a page load is never checked again, even
// after it loads another page, like uBO.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, ExpiredWindowIsNotCheckedAgain) {
  base::SimpleTestClock clock;
  AdBlockPopupNavigationThrottle::SetClockForTesting(&clock);
  base::ScopedClosureRunner reset_clock(base::BindOnce(
      [] { AdBlockPopupNavigationThrottle::SetClockForTesting(nullptr); }));
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));

  content::WebContents* popup = OpenPopup(web_contents(), Url("c.com"));
  clock.SetNow(popup->GetController().GetLastCommittedEntry()->GetTimestamp() +
               base::Seconds(11));
  EXPECT_TRUE(content::NavigateToURLFromRenderer(popup, Url("d.com")));

  clock.SetNow(popup->GetController().GetLastCommittedEntry()->GetTimestamp());
  EXPECT_FALSE(NavigatesItselfIsBlocked(popup, Url("b.com")));
  EXPECT_TRUE(WasRequested("b.com"));
}

// A blank window is checked when its opener navigates it within 10 s of
// opening it, like uBO.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, BlankWindowNavigatedLater) {
  base::SimpleTestClock clock;
  AdBlockPopupNavigationThrottle::SetClockForTesting(&clock);
  base::ScopedClosureRunner reset_clock(base::BindOnce(
      [] { AdBlockPopupNavigationThrottle::SetClockForTesting(nullptr); }));
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));
  content::WebContents* tab = web_contents();

  for (const base::TimeDelta delay : {base::Seconds(10), base::Seconds(11)}) {
    SCOPED_TRACE(delay);
    content::WebContents* popup = WaitForPopup(
        tab, base::BindLambdaForTesting([&] {
          content::ExecuteScriptAsync(tab, "window.w = window.open()");
        }));
    // The blank document's commit.
    content::NavigationEntry* entry = nullptr;
    ASSERT_TRUE(base::test::RunUntil([&] {
      entry = popup->GetController().GetLastCommittedEntry();
      return !entry->GetTimestamp().is_null();
    }));
    clock.SetNow(entry->GetTimestamp() + delay);
    content::ExecuteScriptAsync(
        tab, content::JsReplace("w.location = $1", Url("b.com")));
    EXPECT_EQ(delay == base::Seconds(10), WaitForBlocked(popup));
  }
}

// uBO lets a popup through when it's for the link the user clicked. Here it's
// blocked and listed in the blocked-popup UI.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, ClickedLink) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  LoadPageWithLink(Url("b.com"));
  EXPECT_TRUE(ClickIsBlocked("link"));
  EXPECT_EQ(Url("b.com"), LastBlockedPopupUrl(web_contents()));
  EXPECT_FALSE(WasRequested("b.com"));
}

IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, ClickedLinkWithOpener) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  LoadPageWithLink(Url("b.com"), "opener");
  EXPECT_TRUE(ClickIsBlocked("link"));
  EXPECT_FALSE(WasRequested("b.com"));
}

IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, ScriptClickedLink) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  LoadPageWithLink(Url("b.com"));
  EXPECT_TRUE(OpensBlockedPopup("link.click()"));
  LoadPageWithLink(Url("b.com"), "opener");
  EXPECT_TRUE(OpensBlockedPopup("link.click()"));
  EXPECT_FALSE(WasRequested("b.com"));
}

// An invisible link over the page, so a click anywhere opens a popup.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, InvisibleOverlayLink) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup");
  NavigateToURL(Url("a.com"));
  ASSERT_TRUE(
      content::ExecJs(web_contents(), content::JsReplace(R"(
        document.body.innerHTML =
            '<div id="box" style="width:200px;height:200px"></div>';
        const a = document.createElement('a');
        a.href = $1;
        a.target = '_blank';
        a.style = 'position:fixed;inset:0;opacity:0';
        document.body.appendChild(a);
      )",
                                                         Url("b.com"))));
  EXPECT_TRUE(ClickIsBlocked("box"));
  EXPECT_FALSE(WasRequested("b.com"));
}

// The page cancels the click on a link and opens a window itself, for the
// link's URL or another one.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, CancelledClickOpensWindow) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup\n||d.com^$popup");
  LoadPageWithLink(Url("b.com"));
  ASSERT_TRUE(content::ExecJs(web_contents(), R"(
    link.addEventListener('click', (e) => {
      e.preventDefault();
      window.open(window.target || link.href);
    });
  )"));
  EXPECT_TRUE(ClickIsBlocked("link"));
  EXPECT_EQ(Url("b.com"), LastBlockedPopupUrl(web_contents()));

  ASSERT_TRUE(content::ExecJs(
      web_contents(), content::JsReplace("window.target = $1", Url("d.com"))));
  EXPECT_TRUE(ClickIsBlocked("link"));
  EXPECT_EQ(Url("d.com"), LastBlockedPopupUrl(web_contents()));
  EXPECT_FALSE(WasRequested("b.com"));
  EXPECT_FALSE(WasRequested("d.com"));
}

// A middle-click or modifier-click opens a tab without an opener, which isn't a
// popup, even when that tab navigates itself afterwards.
IN_PROC_BROWSER_TEST_F(AdBlockPopupTest, LinkOpenedInNewTab) {
  UpdateAdBlockInstanceWithRules("||b.com^$popup\n||d.com^$popup");

  for (const char* target : {"", "_blank"}) {
    for (const auto& [button, modifiers] :
         {std::pair(blink::WebMouseEvent::Button::kMiddle, 0),
          std::pair(blink::WebMouseEvent::Button::kLeft, kNewTabModifier)}) {
      SCOPED_TRACE(testing::Message()
                   << "target=" << target << " modifiers=" << modifiers);
      LoadPageWithLink(Url("b.com"), "", target);
      content::WebContents* tab = OpenInNewTab("link", button, modifiers);
      EXPECT_EQ(Url("b.com"), tab->GetLastCommittedURL());
      EXPECT_EQ(WindowOpenDisposition::kUnknown,
                tab->GetOriginalWindowOpenDisposition());

      EXPECT_TRUE(content::NavigateToURLFromRenderer(tab, Url("d.com")));
      EXPECT_EQ(Url("d.com"), tab->GetLastCommittedURL());
      TabStripModel* tabs = browser()->tab_strip_model();
      tabs->CloseWebContentsAt(tabs->GetIndexOfWebContents(tab), 0);
    }
  }
}
