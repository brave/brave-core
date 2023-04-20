import WebKit
import Shared
import Preferences

// WKNavigationDelegates must implement NSObjectProtocol
class TabManagerNavDelegate: NSObject, WKNavigationDelegate {
  private var delegates = WeakList<WKNavigationDelegate>()
  weak var tabManager: TabManager?

  func insert(_ delegate: WKNavigationDelegate) {
    delegates.insert(delegate)
  }

  func webView(_ webView: WKWebView, didCommit navigation: WKNavigation!) {
    for delegate in delegates {
      delegate.webView?(webView, didCommit: navigation)
    }
  }

  func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
    for delegate in delegates {
      delegate.webView?(webView, didFail: navigation, withError: error)
    }
  }

  func webView(_ webView: WKWebView, didFailProvisionalNavigation navigation: WKNavigation!, withError error: Error) {
    for delegate in delegates {
      delegate.webView?(webView, didFailProvisionalNavigation: navigation, withError: error)
    }
  }

  func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    for delegate in delegates {
      delegate.webView?(webView, didFinish: navigation)
    }
  }

  func webViewWebContentProcessDidTerminate(_ webView: WKWebView) {
    for delegate in delegates {
      delegate.webViewWebContentProcessDidTerminate?(webView)
    }
  }

  public func webView(_ webView: WKWebView, respondTo challenge: URLAuthenticationChallenge) async -> (URLSession.AuthChallengeDisposition, URLCredential?) {
    let authenticatingDelegates = delegates.filter { wv in
      return wv.responds(to: #selector(WKNavigationDelegate.webView(_:didReceive:completionHandler:)))
    }

    guard let firstAuthenticatingDelegate = authenticatingDelegates.first else {
      return (.performDefaultHandling, nil)
    }

    // Do NOT change to `delegate.webView?(....)` the optional operator makes async-await calls crash the compiler atm!
    // It must be force-unwrapped at the time of writing `January 17th, 2023`.
    return await firstAuthenticatingDelegate.webView!(webView, respondTo: challenge)
  }

  func webView(_ webView: WKWebView, didReceiveServerRedirectForProvisionalNavigation navigation: WKNavigation!) {
    for delegate in delegates {
      delegate.webView?(webView, didReceiveServerRedirectForProvisionalNavigation: navigation)
    }
  }

  func webView(_ webView: WKWebView, didStartProvisionalNavigation navigation: WKNavigation!) {
    for delegate in delegates {
      delegate.webView?(webView, didStartProvisionalNavigation: navigation)
    }
  }

  private func defaultAllowPolicy() -> WKNavigationActionPolicy {
    let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
    if isPrivateBrowsing || !Preferences.General.followUniversalLinks.value {
      // Stop Brave from opening universal links by using the private enum value
      // `_WKNavigationActionPolicyAllowWithoutTryingAppLink` which is defined here:
      // https://github.com/WebKit/WebKit/blob/main/Source/WebKit/UIProcess/API/Cocoa/WKNavigationDelegatePrivate.h#L62
      let allowDecision = WKNavigationActionPolicy(rawValue: WKNavigationActionPolicy.allow.rawValue + 2) ?? .allow
      return allowDecision
    }
    return .allow
  }
  
  @MainActor
  func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, preferences: WKWebpagePreferences) async -> (WKNavigationActionPolicy, WKWebpagePreferences) {
    var res = defaultAllowPolicy()
    var pref = preferences
    
    for delegate in delegates {
      // Needed to resolve ambiguous delegate signatures: https://github.com/apple/swift/issues/45652#issuecomment-1149235081
      typealias WKNavigationActionSignature = (WKNavigationDelegate) -> ((WKWebView, WKNavigationAction, WKWebpagePreferences, @escaping (WKNavigationActionPolicy, WKWebpagePreferences) -> Void) -> Void)?
      
      // Needed to detect if async implementations exist as we cannot detect them directly
      if delegate.responds(to: #selector(WKNavigationDelegate.webView(_:decidePolicyFor:preferences:decisionHandler:) as WKNavigationActionSignature)) {
        // Do NOT change to `delegate.webView?(....)` the optional operator makes async-await calls crash the compiler atm!
        // It must be force-unwrapped at the time of writing `January 10th, 2023`.
        let (policy, preferences) = await delegate.webView!(webView, decidePolicyFor: navigationAction, preferences: preferences)
        if policy == .cancel {
          res = policy
        }
        
        pref = preferences
      }
    }
    
    return (res, pref)
  }
  
  @MainActor
  func webView(_ webView: WKWebView, decidePolicyFor navigationResponse: WKNavigationResponse) async -> WKNavigationResponsePolicy {
    var res = WKNavigationResponsePolicy.allow
    for delegate in delegates {
      // Needed to resolve ambiguous delegate signatures: https://github.com/apple/swift/issues/45652#issuecomment-1149235081
      typealias WKNavigationResponseSignature = (WKNavigationDelegate) -> ((WKWebView, WKNavigationResponse, @escaping (WKNavigationResponsePolicy) -> Void) -> Void)?
      
      // Needed to detect if async implementations exist as we cannot detect them directly
      if delegate.responds(to: #selector(WKNavigationDelegate.webView(_:decidePolicyFor:decisionHandler:) as WKNavigationResponseSignature)) {
        // Do NOT change to `delegate.webView?(....)` the optional operator makes async-await calls crash the compiler atm!
        // It must be force-unwrapped at the time of writing `January 10th, 2023`.
        let policy = await delegate.webView!(webView, decidePolicyFor: navigationResponse)
        if policy == .cancel {
          res = policy
        }
      }
    }

    if res == .allow {
      let tab = tabManager?[webView]
      tab?.mimeType = navigationResponse.response.mimeType
    }
    
    return res
  }
}
