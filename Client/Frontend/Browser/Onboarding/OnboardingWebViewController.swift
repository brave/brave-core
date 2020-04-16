// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared

class OnboardingWebViewController: UIViewController, WKNavigationDelegate {
    
    private let url = URL(string: "https://brave.com/terms-of-use/")
    private var helpers = [String: TabContentScript]()
    private var profile: Profile
    
    private let KVOs: [KVOConstants] = [
        .loading,
        .canGoBack,
        .canGoForward,
        .URL,
        .hasOnlySecureContent,
        .serverTrust
    ]
    
    private let toolbar = Toolbar().then {
        $0.snp.makeConstraints {
            $0.height.greaterThanOrEqualTo(45.0)
        }
    }
    
    private let webView = { () -> WKWebView in
        let configuration: WKWebViewConfiguration = {
            let configuration = WKWebViewConfiguration()
            configuration.processPool = WKProcessPool()
            configuration.preferences.javaScriptCanOpenWindowsAutomatically = false
            configuration.websiteDataStore = WKWebsiteDataStore.nonPersistent()
            return configuration
        }()
        
        return WKWebView(frame: .zero, configuration: configuration)
    }()
    
    deinit {
        KVOs.forEach { webView.removeObserver(self, forKeyPath: $0.rawValue) }
    }
    
    init(profile: Profile) {
        self.profile = profile
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        let stackView = UIStackView().then {
            $0.axis = .vertical
        }
        
        [toolbar, webView].forEach(stackView.addArrangedSubview(_:))
        
        view.addSubview(stackView)
        stackView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        KVOs.forEach { webView.addObserver(self, forKeyPath: $0.rawValue, options: .new, context: nil) }
        
        webView.navigationDelegate = self
        setupScripts()
        webView.load(PrivilegedRequest(url: url!) as URLRequest)
        
        toolbar.exitButton.addTarget(self, action: #selector(onExit), for: .touchUpInside)
        toolbar.backButton.addTarget(self, action: #selector(onBack), for: .touchUpInside)
        toolbar.forwardButton.addTarget(self, action: #selector(onForward), for: .touchUpInside)
    }
    
    private func setupScripts() {
        let errorHelper = ErrorPageHelper()
        addScript(errorHelper, for: ErrorPageHelper.name())
    }
    
    override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
        guard let webView = object as? WKWebView, let kp = keyPath, let path = KVOConstants(rawValue: kp) else {
            return
        }

        switch path {
        case .URL:
            if url?.origin == webView.url?.origin {
                toolbar.urlLabel.text = webView.url?.host
            }
        case .canGoBack:
            updateBackForwardUI()
        case .canGoForward:
            updateBackForwardUI()
        case .hasOnlySecureContent:
            updateWebPageSecurity()
        case .serverTrust:
            updateWebPageSecurity()
        default:
            break
        }
    }
    
    @objc
    private func onExit() {
        self.dismiss(animated: true, completion: nil)
    }
    
    @objc
    private func onBack() {
        if webView.canGoBack {
            webView.goBack()
        }
    }
    
    @objc
    private func onForward() {
        if webView.canGoForward {
            webView.goForward()
        }
    }
    
    private func updateWebPageSecurity() {
        if let trust = webView.serverTrust {
            toolbar.secureIcon.isHidden = false
            
            let policies = [
                SecPolicyCreateBasicX509(),
                SecPolicyCreateSSL(true, webView.url?.host as CFString?)
            ]
            
            SecTrustSetPolicies(trust, policies as CFTypeRef)
            
            var result: SecTrustResultType = .invalid
            SecTrustEvaluate(trust, &result)
            
            if (result == .proceed || result == .unspecified) && webView.hasOnlySecureContent {
                toolbar.secureIcon.tintColor = UX.secureWebPageColor
                toolbar.urlLabel.textColor = UX.secureWebPageColor
            } else {
                toolbar.secureIcon.tintColor = UX.insecureWebPageColor
                toolbar.urlLabel.textColor = UX.insecureWebPageColor
            }
        } else {
            toolbar.secureIcon.isHidden = true
            toolbar.urlLabel.textColor = UX.unknownWebPageColor
        }
    }
    
    private func updateBackForwardUI() {
        toolbar.backButton.isEnabled = webView.canGoBack
        toolbar.backButton.tintColor = webView.canGoBack ? UX.buttonEnabledColor : UX.buttonDisabledColor

        toolbar.forwardButton.isEnabled = webView.canGoForward
        toolbar.forwardButton.tintColor = webView.canGoForward ? UX.buttonEnabledColor : UX.buttonDisabledColor
    }
    
    func webView(_ webView: WKWebView, didFailProvisionalNavigation navigation: WKNavigation!, withError error: Error) {
        
        let error = error as NSError
        if error.domain == "WebKitErrorDomain" && error.code == 102 {
            return
        }

        if error.code == WKError.webContentProcessTerminated.rawValue && error.domain == "WebKitErrorDomain" {
            webView.reload()
            return
        }

        if error.code == Int(CFNetworkErrors.cfurlErrorCancelled.rawValue) {
            return
        }

        if let url = error.userInfo[NSURLErrorFailingURLErrorKey] as? URL {
            ErrorPageHelper().showPage(error, forUrl: url, inWebView: webView)
        }
    }
    
    func webView(_ webView: WKWebView, didReceive challenge: URLAuthenticationChallenge, completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {
        if challenge.protectionSpace.host == "localhost" && challenge.protectionSpace.port == Int(WebServer.sharedInstance.server.port) {
            return completionHandler(.useCredential, WebServer.sharedInstance.credentials)
        }
        
        let origin = "\(challenge.protectionSpace.host):\(challenge.protectionSpace.port)"
        if challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
            let trust = challenge.protectionSpace.serverTrust,
            let cert = SecTrustGetCertificateAtIndex(trust, 0), profile.certStore.containsCertificate(cert, forOrigin: origin) {
            return completionHandler(.useCredential, URLCredential(trust: trust))
        }
        
        completionHandler(.performDefaultHandling, nil)
    }
}

extension OnboardingWebViewController: WKScriptMessageHandler {
    func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage) {
        
        for helper in helpers.values {
            if let scriptMessageHandlerName = helper.scriptMessageHandlerName(),
                scriptMessageHandlerName == message.name {
                return helper.userContentController(userContentController, didReceiveScriptMessage: message)
            }
        }
    }
    
    private func addScript(_ helper: TabContentScript, for name: String) {
        helpers[name] = helper
        
        if let scriptMessageHandlerName = helper.scriptMessageHandlerName() {
            webView.configuration.userContentController.add(self, name: scriptMessageHandlerName)
        }
    }
}

extension OnboardingWebViewController {
    private struct UX {
        static let buttonEnabledColor = UIColor(rgb: 0x5E6770)
        static let buttonDisabledColor = UIColor(rgb: 0xE0E1E5)
        static let secureWebPageColor = UIColor(rgb: 0x03A402)
        static let insecureWebPageColor = UIColor.red
        static let unknownWebPageColor = UIColor.darkGray
    }
    
    class Toolbar: UIView {
        let exitButton = UIButton().then {
            $0.setImage(#imageLiteral(resourceName: "onboarding_exit").template, for: .normal)
            $0.contentMode = .scaleAspectFit
            $0.tintColor = UX.buttonEnabledColor
            $0.setContentHuggingPriority(.required, for: .horizontal)
            $0.setContentCompressionResistancePriority(.required, for: .horizontal)
        }
        
        let backButton = UIButton().then {
            $0.setImage(#imageLiteral(resourceName: "onboarding_back").template, for: .normal)
            $0.contentMode = .scaleAspectFit
            $0.tintColor = UX.buttonDisabledColor
            $0.setContentHuggingPriority(.required, for: .horizontal)
            $0.setContentCompressionResistancePriority(.required, for: .horizontal)
            $0.isEnabled = false
        }
        
        let secureIcon = UIImageView().then {
            $0.image = #imageLiteral(resourceName: "onboarding_secure_page_lock").template
            $0.contentMode = .scaleAspectFit
            $0.tintColor = UX.unknownWebPageColor
            $0.setContentHuggingPriority(.required, for: .horizontal)
            $0.setContentCompressionResistancePriority(.required, for: .horizontal)
            $0.isHidden = true
        }
        
        let urlLabel = UILabel().then {
            $0.textColor = UX.unknownWebPageColor
            $0.font = UIFont.systemFont(ofSize: 15.0, weight: .medium)
            $0.numberOfLines = 0
        }
        
        let forwardButton = UIButton().then {
            $0.setImage(#imageLiteral(resourceName: "onboarding_forward").template, for: .normal)
            $0.contentMode = .scaleAspectFit
            $0.tintColor = UX.buttonDisabledColor
            $0.setContentHuggingPriority(.required, for: .horizontal)
            $0.setContentCompressionResistancePriority(.required, for: .horizontal)
            $0.isEnabled = false
        }
        
        private let leftStackview = UIStackView().then {
            $0.spacing = 16.0
            $0.isLayoutMarginsRelativeArrangement = true
            $0.layoutMargins = UIEdgeInsets(equalInset: 13.0)
        }
        
        private let middleStackview = UIStackView().then {
            $0.spacing = 4.0
            $0.alignment = .center
            $0.isLayoutMarginsRelativeArrangement = true
            $0.layoutMargins = UIEdgeInsets(equalInset: 13.0)
        }
        
        private let rightStackview = UIStackView().then {
            $0.spacing = 16.0
            $0.isLayoutMarginsRelativeArrangement = true
            $0.layoutMargins = UIEdgeInsets(equalInset: 13.0)
        }
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            backgroundColor = .white
            
            addSubview(leftStackview)
            addSubview(middleStackview)
            addSubview(rightStackview)
            
            [exitButton, backButton].forEach(leftStackview.addArrangedSubview(_:))
            [secureIcon, urlLabel].forEach(middleStackview.addArrangedSubview(_:))
            rightStackview.addArrangedSubview(forwardButton)
            
            middleStackview.snp.makeConstraints {
                $0.centerX.equalTo(self.snp.centerX)
                $0.top.bottom.equalToSuperview()
            }
            
            leftStackview.snp.makeConstraints {
                $0.left.top.bottom.equalToSuperview()
                $0.right.equalTo(middleStackview.snp.left).offset(-20.0)
            }
            
            rightStackview.snp.makeConstraints {
                $0.right.top.bottom.equalToSuperview()
                $0.left.equalTo(middleStackview.snp.right).offset(20.0)
            }
        }
        
        required init?(coder: NSCoder) {
            fatalError("init(coder:) has not been implemented")
        }
    }
}
