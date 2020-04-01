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
        case notSupportedError = "NotSupportedError"
        case abortError = "AbortError"
        case typeError = "TypeError"
        case rangeError = "RangeError"
        case unknownError = "UnknownError"
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
                sendPaymentRequestError(errorName: PaymentRequestErrors.unknownError.rawValue, errorMessage: Strings.clientErrorMessage)
                return
            }
            
            guard body.methodData.contains(where: {$0.supportedMethods.lowercased() == "bat"}) else {
                sendPaymentRequestError(errorName: PaymentRequestErrors.notSupportedError.rawValue, errorMessage: Strings.unsupportedInstrumentMessage)
                return
            }
            
            // All currencies should match
            guard body.details.displayItems.map({$0.amount.currency}).allSatisfy({ $0 == body.details.total.amount.currency }) else {
                sendPaymentRequestError(errorName: PaymentRequestErrors.typeError.rawValue, errorMessage: Strings.invalidDetailsMessage)
                return
            }
            
            // Sum of individual items does not match the total
            guard Double(body.details.total.amount.value) == body.details.displayItems.compactMap({(Double($0.amount.value) )}).reduce(0, +) else {
                sendPaymentRequestError(errorName: PaymentRequestErrors.rangeError.rawValue, errorMessage: Strings.invalidDetailsMessage)
                return
            }

            paymentRequested(body) { response in
                switch response {
                case .cancelled:
                    ensureMainThread {
                        self.sendPaymentRequestError(errorName: PaymentRequestErrors.abortError.rawValue, errorMessage: Strings.userCancelledMessage)
                    }
                case .completed(let orderId):
                    ensureMainThread {
                        self.tab?.webView?.evaluateJavaScript("PaymentRequestCallback\(self.token).paymentreq_postCreate('\(orderId)', '', '')") { _, error in
                            if error != nil {
                                log.error(error)
                            }
                        }
                    }
                }
            }
        } catch {
            sendPaymentRequestError(errorName: PaymentRequestErrors.typeError.rawValue, errorMessage: Strings.invalidDetailsMessage)
            return
        }
            
    }
}

extension Strings {
    //Errors
    public static let unsupportedInstrumentMessage = NSLocalizedString("unsupportedInstrumentMessage", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Unsupported payment instruments", comment: "Error message if list of Payment Instruments doesn't include BAT")
    public static let userCancelledMessage = NSLocalizedString("userCancelledMessage", tableName: "BraveShared", bundle: Bundle.braveShared, value: "User cancelled", comment: "Error message if the payment workflow is canceled by the user")
    public static let invalidDetailsMessage = NSLocalizedString("invalidDetailsMessage", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Invalid details in payment request", comment: "Error message if details don't have the right type or values")
    public static let clientErrorMessage = NSLocalizedString("clientErrorMessage", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Client error", comment: "Client is in an invalid state which caused the error")
}
