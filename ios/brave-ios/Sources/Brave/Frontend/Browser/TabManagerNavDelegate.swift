import BraveCore
import Preferences
import Shared
import WebKit

// WKNavigationDelegates must implement NSObjectProtocol
class TabManagerNavDelegate: NSObject, CWVNavigationDelegate {
  // FIXME: Nav Delegate
  private var delegates = WeakList<CWVNavigationDelegate>()
  weak var tabManager: TabManager?

  func insert(_ delegate: CWVNavigationDelegate) {
    delegates.insert(delegate)
  }

  func webViewDidCommitNavigation(_ webView: CWVWebView) {
    for delegate in delegates {
      delegate.webViewDidCommitNavigation?(webView)
    }
  }
//
//  func webView(_ webView: WKWebView, didFail navigation: WKNavigation, withError error: Error) {
//    for delegate in delegates {
//      delegate.webView?(webView, didFail: navigation, withError: error)
//    }
//  }
//
//  func webView(
//    _ webView: WKWebView,
//    didFailProvisionalNavigation navigation: WKNavigation,
//    withError error: Error
//  ) {
//    for delegate in delegates {
//      delegate.webView?(webView, didFailProvisionalNavigation: navigation, withError: error)
//    }
//  }
//
//  func webView(_ webView: WKWebView, didFinish navigation: WKNavigation) {
//    for delegate in delegates {
//      delegate.webView?(webView, didFinish: navigation)
//    }
//  }
//
//  func webViewWebContentProcessDidTerminate(_ webView: WKWebView) {
//    for delegate in delegates {
//      delegate.webViewWebContentProcessDidTerminate?(webView)
//    }
//  }
//
//  func webView(
//    _ webView: WKWebView,
//    didReceive challenge: URLAuthenticationChallenge,
//    completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void
//  ) {
//    let authenticatingDelegates = delegates.filter { wv in
//      return wv.responds(
//        to: #selector(WKNavigationDelegate.webView(_:didReceive:completionHandler:))
//      )
//    }
//
//    guard let firstAuthenticatingDelegate = authenticatingDelegates.first else {
//      completionHandler(.performDefaultHandling, nil)
//      return
//    }
//
//    firstAuthenticatingDelegate.webView?(
//      webView,
//      didReceive: challenge,
//      completionHandler: completionHandler
//    )
//  }
//
//  func webView(
//    _ webView: WKWebView,
//    didReceiveServerRedirectForProvisionalNavigation navigation: WKNavigation!
//  ) {
//    for delegate in delegates {
//      delegate.webView?(webView, didReceiveServerRedirectForProvisionalNavigation: navigation)
//    }
//  }
//
//  func webView(_ webView: WKWebView, didStartProvisionalNavigation navigation: WKNavigation!) {
//    for delegate in delegates {
//      delegate.webView?(webView, didStartProvisionalNavigation: navigation)
//    }
//  }
//
//  private func defaultAllowPolicy(
//    for navigationAction: WKNavigationAction
//  ) -> WKNavigationActionPolicy {
//    let isPrivateBrowsing = tabManager?.privateBrowsingManager.isPrivateBrowsing == true
//    func isYouTubeLoad() -> Bool {
//      guard let domain = navigationAction.request.mainDocumentURL?.baseDomain else {
//        return false
//      }
//      let domainsWithUniversalLinks: Set<String> = ["youtube.com", "youtu.be"]
//      return domainsWithUniversalLinks.contains(domain)
//    }
//    if isPrivateBrowsing || !Preferences.General.followUniversalLinks.value
//      || (Preferences.General.keepYouTubeInBrave.value && isYouTubeLoad())
//    {
//      // Stop Brave from opening universal links by using the private enum value
//      // `_WKNavigationActionPolicyAllowWithoutTryingAppLink` which is defined here:
//      // https://github.com/WebKit/WebKit/blob/main/Source/WebKit/UIProcess/API/Cocoa/WKNavigationDelegatePrivate.h#L62
//      let allowDecision =
//        WKNavigationActionPolicy(rawValue: WKNavigationActionPolicy.allow.rawValue + 2) ?? .allow
//      return allowDecision
//    }
//    return .allow
//  }
//
//  func webView(
//    _ webView: WKWebView,
//    decidePolicyFor navigationAction: WKNavigationAction,
//    preferences: WKWebpagePreferences,
//    decisionHandler: @escaping (WKNavigationActionPolicy, WKWebpagePreferences) -> Void
//  ) {
//    var res = defaultAllowPolicy(for: navigationAction)
//    var pref = preferences
//
//    let group = DispatchGroup()
//
//    for delegate in delegates {
//      if !delegate.responds(to: #selector(webView(_:decidePolicyFor:preferences:decisionHandler:)))
//      {
//        continue
//      }
//      group.enter()
//      delegate.webView?(
//        webView,
//        decidePolicyFor: navigationAction,
//        preferences: pref,
//        decisionHandler: { policy, preferences in
//          if policy == .cancel {
//            res = policy
//          }
//
//          if policy == .download {
//            res = policy
//          }
//
//          pref = preferences
//
//          group.leave()
//        }
//      )
//    }
//
//    group.notify(queue: .main) {
//      decisionHandler(res, pref)
//    }
//  }
//
//  func webView(
//    _ webView: WKWebView,
//    decidePolicyFor navigationResponse: WKNavigationResponse,
//    decisionHandler: @escaping (WKNavigationResponsePolicy) -> Void
//  ) {
//    var res = WKNavigationResponsePolicy.allow
//    let group = DispatchGroup()
//    for delegate in delegates {
//      if !delegate.responds(to: #selector(webView(_:decidePolicyFor:decisionHandler:))) {
//        continue
//      }
//      group.enter()
//      delegate.webView?(
//        webView,
//        decidePolicyFor: navigationResponse,
//        decisionHandler: { policy in
//          if policy == .cancel {
//            res = policy
//          }
//
//          if policy == .download {
//            res = policy
//          }
//          group.leave()
//        }
//      )
//    }
//
//    if res == .allow {
//      let tab = tabManager?[webView]
//      tab?.mimeType = navigationResponse.response.mimeType
//    }
//
//    group.notify(queue: .main) {
//      decisionHandler(res)
//    }
//  }
//
//  func webView(
//    _ webView: WKWebView,
//    navigationAction: WKNavigationAction,
//    didBecome download: WKDownload
//  ) {
//    for delegate in delegates {
//      delegate.webView?(webView, navigationAction: navigationAction, didBecome: download)
//      if download.delegate != nil {
//        return
//      }
//    }
//  }
//
//  func webView(
//    _ webView: WKWebView,
//    navigationResponse: WKNavigationResponse,
//    didBecome download: WKDownload
//  ) {
//    for delegate in delegates {
//      delegate.webView?(webView, navigationResponse: navigationResponse, didBecome: download)
//      if download.delegate != nil {
//        return
//      }
//    }
//  }
}
