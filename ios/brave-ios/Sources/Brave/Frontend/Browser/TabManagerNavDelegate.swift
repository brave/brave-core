import BraveCore
import Preferences
import Shared
import WebKit

// WKNavigationDelegates must implement NSObjectProtocol
class TabManagerNavDelegate: NSObject, CWVNavigationDelegate {
  private var delegates = WeakList<CWVNavigationDelegate>()
  weak var tabManager: TabManager?

  func insert(_ delegate: CWVNavigationDelegate) {
    delegates.insert(delegate)
  }

  func webViewDidStartNavigation(_ webView: CWVWebView) {
    for delegate in delegates {
      delegate.webViewDidStartNavigation?(webView)
    }
  }

  func webViewDidStartProvisionalNavigation(_ webView: CWVWebView) {
    for delegate in delegates {
      delegate.webViewDidStartProvisionalNavigation?(webView)
    }
  }

  func webViewDidCommitNavigation(_ webView: CWVWebView) {
    for delegate in delegates {
      delegate.webViewDidCommitNavigation?(webView)
    }
  }

  func webViewDidFinishNavigation(_ webView: CWVWebView) {
    for delegate in delegates {
      delegate.webViewDidFinishNavigation?(webView)
    }
  }

  func webView(
    _ webView: CWVWebView,
    decidePolicyFor navigationAction: CWVNavigationAction,
    decisionHandler: @escaping (CWVNavigationActionPolicy) -> Void
  ) {
    var res: CWVNavigationActionPolicy = .allow

    // Needed to resolve ambiguous delegate signatures: https://github.com/apple/swift/issues/45652#issuecomment-1149235081
    typealias CWVNavigationActionSignature = (CWVNavigationDelegate) -> (
      (
        CWVWebView, CWVNavigationAction,
        @escaping (CWVNavigationActionPolicy) -> Void
      ) -> Void
    )?

    let group = DispatchGroup()
    for delegate in delegates {
      if !delegate.responds(
        to: #selector(
          CWVNavigationDelegate.webView(_:decidePolicyFor:decisionHandler:)
            as CWVNavigationActionSignature
        )
      ) {
        continue
      }
      group.enter()
      delegate.webView?(
        webView,
        decidePolicyFor: navigationAction,
        decisionHandler: { (policy: CWVNavigationActionPolicy) in
          if policy == .cancel {
            res = policy
          }
          group.leave()
        }
      )
    }

    group.notify(queue: .main) {
      decisionHandler(res)
    }
  }

  func webView(
    _ webView: CWVWebView,
    decidePolicyFor navigationResponse: CWVNavigationResponse,
    decisionHandler: @escaping (CWVNavigationResponsePolicy) -> Void
  ) {
    var res = CWVNavigationResponsePolicy.allow

    // Needed to resolve ambiguous delegate signatures: https://github.com/apple/swift/issues/45652#issuecomment-1149235081
    typealias CWVNavigationResponseSignature = (CWVNavigationDelegate) -> (
      (
        CWVWebView, CWVNavigationResponse,
        @escaping (CWVNavigationResponsePolicy) -> Void
      ) -> Void
    )?

    let group = DispatchGroup()
    for delegate in delegates {
      if !delegate.responds(
        to: #selector(
          CWVNavigationDelegate.webView(_:decidePolicyFor:decisionHandler:)
            as CWVNavigationResponseSignature
        )
      ) {
        continue
      }
      group.enter()
      delegate.webView?(
        webView,
        decidePolicyFor: navigationResponse,
        decisionHandler: { policy in
          if policy == .cancel {
            res = policy
          }
          group.leave()
        }
      )
    }

    if res == .allow {
      let tab = tabManager?[webView]
      tab?.mimeType = navigationResponse.response.mimeType
    }

    group.notify(queue: .main) {
      decisionHandler(res)
    }
  }

  func webView(_ webView: CWVWebView, didFailNavigationWithError error: any Error) {
    for delegate in delegates {
      delegate.webView?(webView, didFailNavigationWithError: error)
    }
  }

  func webViewWebContentProcessDidTerminate(_ webView: CWVWebView) {
    for delegate in delegates {
      delegate.webViewWebContentProcessDidTerminate?(webView)
    }
  }

  func webView(_ webView: CWVWebView, didRequestDownloadWith task: CWVDownloadTask) {
    for delegate in delegates {
      delegate.webView?(webView, didRequestDownloadWith: task)
    }
  }

  func webView(_ webView: CWVWebView, shouldBlockUniversalLinksFor request: URLRequest) -> Bool {
    for delegate in delegates {
      if let shouldBlock = delegate.webView?(webView, shouldBlockUniversalLinksFor: request),
        shouldBlock
      {
        return true
      }
    }
    return false
  }

  func webView(_ webView: CWVWebView, shouldBlockJavaScriptFor request: URLRequest) -> Bool {
    for delegate in delegates {
      if let shouldBlock = delegate.webView?(webView, shouldBlockJavaScriptFor: request),
        shouldBlock
      {
        return true
      }
    }
    return false
  }

  func webView(
    _ webView: CWVWebView,
    didRequestHTTPAuthFor protectionSpace: URLProtectionSpace,
    proposedCredential: URLCredential,
    completionHandler handler: @escaping (String?, String?) -> Void
  ) {
    var handled: Bool = false
    for delegate in delegates {
      delegate.webView?(
        webView,
        didRequestHTTPAuthFor: protectionSpace,
        proposedCredential: proposedCredential,
        completionHandler: { username, password in
          if !handled {
            handled = true
            handler(username, password)
          }
        }
      )
    }
  }
}
