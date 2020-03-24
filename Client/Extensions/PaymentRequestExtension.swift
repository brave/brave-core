// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveRewards
import BraveShared
import Data
import Shared
import WebKit

private let log = Logger.browserLogger

class PaymentRequestExtension: NSObject {
    typealias PaymentRequestHandler = (
        PaymentRequest,
        _ completionHandler: @escaping (_ response: PaymentRequestResponse) -> Void
        ) -> Void
    
    fileprivate weak var tab: Tab?
    fileprivate weak var rewards: BraveRewards?
    fileprivate var token: String
    
    fileprivate enum PaymentRequestErrors: String {
        case NotSupportedError = "NotSupportedError"
        case AbortError = "AbortError"
    }
    
    private let paymentRequested: PaymentRequestHandler
    
    init(rewards: BraveRewards, tab: Tab, paymentRequested: @escaping PaymentRequestHandler) {
        self.paymentRequested = paymentRequested
        token = UserScriptManager.securityToken.uuidString.replacingOccurrences(of: "-", with: "", options: .literal)
        self.tab = tab
        self.rewards = rewards
    }
}

extension PaymentRequestExtension: TabContentScript {
    static func name() -> String {
        return "PaymentRequest"
    }
    
    func scriptMessageHandlerName() -> String? {
        return PaymentRequestExtension.name()
    }
    
    private func sendPaymentRequestError(errorName: String, errorMessage: String) {
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("PaymentRequestCallback\(self.token).paymentreq_postCreate('', '\(errorName)', '\(errorMessage)')") { _, error in
                    if error != nil {
                        log.error(error)
                    }
                }
        }
    }
    
    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        guard message.name == Self.name(), let body = message.body as? NSDictionary else { return }
        
        do {
            let messageData = try JSONSerialization.data(withJSONObject: body, options: [])
            let body = try JSONDecoder().decode(PaymentRequest.self, from: messageData)
            if body.name != "payment-request-show" {
                return
            }
            
            guard body.methodData.contains(where: {$0.supportedMethods.lowercased() == "bat"}) else {
                sendPaymentRequestError(errorName: PaymentRequestErrors.NotSupportedError.rawValue, errorMessage: Strings.unsupportedInstrumentMessage)
                return
            }
            
            paymentRequested(body) { response in
                switch response {
                case .cancelled:
                    ensureMainThread {
                        self.sendPaymentRequestError(errorName: PaymentRequestErrors.AbortError.rawValue, errorMessage: Strings.userCancelledMessage)
                    }
                case .completed(let response):
                    ensureMainThread {
                        let trimmed = response.components(separatedBy: .newlines).joined()
                        self.tab?.webView?.evaluateJavaScript("PaymentRequestCallback\(self.token).paymentreq_postCreate('\(trimmed)', '', '')") { _, error in
                            if error != nil {
                                log.error(error)
                            }
                        }
                    }
                }
            }
        } catch {
            log.error(error)
        }
            
    }
}

extension Strings {
    //Errors
    public static let unsupportedInstrumentMessage = NSLocalizedString("unsupportedInstrumentMessage", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Unsupported payment instruments", comment: "Error message if list of Payment Instruments doesn't include BAT")
    public static let userCancelledMessage = NSLocalizedString("userCancelledMessage", tableName: "BraveShared", bundle: Bundle.braveShared, value: "User cancelled", comment: "Error message if the payment workflow is canceled by the user")
}
