/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import SnapKit
import UIKit
import WebKit
import BraveShared

let DefaultTimeoutTimeInterval = 10.0  // Seconds.  We'll want some telemetry on load times in the wild.

/**
 * A controller that manages a single web view and provides a way for
 * the user to navigate back to Settings.
 */
class SettingsContentViewController: UIViewController, WKNavigationDelegate {
  let interstitialBackgroundColor: UIColor
  var settingsTitle: NSAttributedString?
  var url: URL!
  var timer: Timer?

  var isLoaded: Bool = false {
    didSet {
      if isLoaded {
        UIView.transition(
          from: interstitialView, to: webView,
          duration: 0.5,
          options: .transitionCrossDissolve,
          completion: { finished in
            self.interstitialView.removeFromSuperview()
            self.interstitialSpinnerView.stopAnimating()
          })
      }
    }
  }

  fileprivate var isError: Bool = false {
    didSet {
      if isError {
        interstitialErrorView.isHidden = false
        UIView.transition(
          from: interstitialSpinnerView, to: interstitialErrorView,
          duration: 0.5,
          options: .transitionCrossDissolve,
          completion: { finished in
            self.interstitialSpinnerView.removeFromSuperview()
            self.interstitialSpinnerView.stopAnimating()
          })
      }
    }
  }

  // The view shown while the content is loading in the background web view.
  fileprivate var interstitialView: UIView!
  fileprivate var interstitialSpinnerView: UIActivityIndicatorView!
  fileprivate var interstitialErrorView: UILabel!

  // The web view that displays content.
  var webView: BraveWebView!

  fileprivate func startLoading(_ timeout: Double = DefaultTimeoutTimeInterval) {
    if self.isLoaded {
      return
    }
    if timeout > 0 {
      self.timer = Timer.scheduledTimer(timeInterval: timeout, target: self, selector: #selector(didTimeOut), userInfo: nil, repeats: false)
    } else {
      self.timer = nil
    }
    self.webView.load(PrivilegedRequest(url: url) as URLRequest)
    self.interstitialSpinnerView.startAnimating()
  }

  init(backgroundColor: UIColor = UIColor.white, title: NSAttributedString? = nil) {
    interstitialBackgroundColor = backgroundColor
    settingsTitle = title
    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    // This background agrees with the web page background.
    // Keeping the background constant prevents a pop of mismatched color.
    view.backgroundColor = interstitialBackgroundColor

    self.webView = makeWebView()
    view.addSubview(webView)
    self.webView.snp.remakeConstraints { make in
      make.edges.equalTo(self.view)
    }

    // Destructuring let causes problems.
    let ret = makeInterstitialViews()
    self.interstitialView = ret.0
    self.interstitialSpinnerView = ret.1
    self.interstitialErrorView = ret.2
    view.addSubview(interstitialView)
    self.interstitialView.snp.remakeConstraints { make in
      make.edges.equalTo(self.view)
    }

    startLoading()
  }

  func makeWebView() -> BraveWebView {
    let frame = CGRect(width: 1, height: 1)
    let configuration = WKWebViewConfiguration().then {
      $0.setURLSchemeHandler(InternalSchemeHandler(), forURLScheme: InternalURL.scheme)
    }
    let webView = BraveWebView(frame: frame, configuration: configuration)
    webView.allowsLinkPreview = false
    webView.navigationDelegate = self
    return webView
  }

  fileprivate func makeInterstitialViews() -> (UIView, UIActivityIndicatorView, UILabel) {
    let view = UIView()

    // Keeping the background constant prevents a pop of mismatched color.
    view.backgroundColor = interstitialBackgroundColor

    let spinner = UIActivityIndicatorView(style: .medium)
    view.addSubview(spinner)

    let error = UILabel()
    if let _ = settingsTitle {
      error.text = Strings.settingsContentLoadErrorMessage
      error.textColor = .braveErrorLabel
      error.textAlignment = .center
    }
    error.isHidden = true
    view.addSubview(error)

    spinner.snp.makeConstraints { make in
      make.center.equalTo(view)
      return
    }

    error.snp.makeConstraints { make in
      make.center.equalTo(view)
      make.left.equalTo(view.snp.left).offset(20)
      make.right.equalTo(view.snp.right).offset(-20)
      make.height.equalTo(44)
      return
    }

    return (view, spinner, error)
  }

  @objc func didTimeOut() {
    self.timer = nil
    self.isError = true
  }

  func webView(_ webView: WKWebView, didFailProvisionalNavigation navigation: WKNavigation!, withError error: Error) {
    didTimeOut()
  }

  func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
    didTimeOut()
  }

  func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    self.timer?.invalidate()
    self.timer = nil
    self.isLoaded = true
  }
}
