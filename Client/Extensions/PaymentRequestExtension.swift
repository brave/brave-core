// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveRewards
import BraveShared
import Data
import Shared
import WebKit

private let log = Logger.browserLogger

let popup = PaymentHandlerPopupView(imageView: nil, title: Strings.paymentRequestTitle, message: "")

class PaymentRequestExtension: NSObject {
    fileprivate weak var tab: Tab?
    fileprivate weak var rewards: BraveRewards?
    fileprivate var response = ""
    fileprivate var token: String
    
    fileprivate enum PaymentRequestErrors: String {
        case NotSupportedError = "NotSupportedError"
        case AbortError = "AbortError"
    }
    
    init(rewards: BraveRewards, tab: Tab) {
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
    
    func sendPaymentRequestError(errorName: String, errorMessage: String) {
        ensureMainThread {
            self.tab?.webView?.evaluateJavaScript("PaymentRequestCallback\(self.token).paymentreq_postCreate('', '\(errorName)', '\(errorMessage)')") { _, error in
                    if error != nil {
                        log.error(error)
                    }
                }
        }
    }
    
    func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage) {
        guard message.name == PaymentRequestExtension.name(), let body = message.body as? NSDictionary else { return }
        
        do {
            let messageData = try JSONSerialization.data(withJSONObject: body, options: [])
            let body = try JSONDecoder().decode(PaymentRequest.self, from: messageData)
            if body.name != "payment-request-show" {
                return
            }
            popup.initPaymentUI()
            
            guard body.methodData.contains(where: {$0.supportedMethods == "bat"}) else {
                sendPaymentRequestError(errorName: PaymentRequestErrors.NotSupportedError.rawValue, errorMessage: Strings.unsupportedInstrumentMessage)
                return
            }
            
            for item in body.details.displayItems {
                popup.addDisplayItemLabel(message: "\(item.label): \(item.amount.value) \(item.amount.currency)\n")
                log.info(item.label)
            }
            
            popup.addTotalLabel(message: "\(body.details.total.label):  \(body.details.total.amount.value) \(body.details.total.amount.currency)\n")
            
            popup.addButton(title: Strings.paymentRequestPay) { [weak self] in
                guard let self = self else {
                    return .flyDown
                }
                guard let rewards = self.rewards, let publisher = self.tab?.publisher, let amount = Double(body.details.total.amount.value) else {
                    return .flyDown
                }
                
                // To-Do: Replace tipping code with SKU APIs - https://github.com/brave/brave-ios/issues/2383
                rewards.ledger.tipPublisherDirectly(publisher, amount: amount, currency: "BAT") { _ in
                    self.response = """
                        {
                          "requestId": "a62c29b3-f840-47cd-b895-4573d3190227",
                          "methodName": "bat",
                          "details": {
                            "transaction_id": "bcbbd947-346d-439f-96b4-101bbd966675",
                            "message": "Payment for Ethiopian Coffee!"
                          }
                        }
                    """
                    
                    ensureMainThread {
                        let trimmed = self.response.components(separatedBy: .newlines).joined()
                        self.tab?.webView?.evaluateJavaScript("PaymentRequestCallback\(self.token).paymentreq_postCreate('\(trimmed)', '', '')") { _, error in
                                if error != nil {
                                    log.error(error)
                                }
                            }
                    }
                }
                
                return .flyDown
            }
            
            popup.addButton(title: Strings.paymentRequestCancel) { [weak self] in
                guard let self = self else {
                    return .flyDown
                }
                
                ensureMainThread {
                    self.sendPaymentRequestError(errorName: PaymentRequestErrors.AbortError.rawValue, errorMessage: Strings.userCancelledMessage)
                }
                
                return .flyDown
            }
            popup.showWithType(showType: .flyUp)
        } catch {
            log.error(error)
        }
            
    }
}

extension Strings {
    public static let paymentRequestTitle = NSLocalizedString("paymentRequestTitle", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Review your payment", comment: "Title for Payment Request modal that shows the user what items are being purchased.")
    public static let paymentRequestPay = NSLocalizedString("paymentRequestPay", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Pay", comment: "Text for the Pay button on Payment Request modal that shows the order summary")
    public static let paymentRequestCancel = NSLocalizedString("paymentRequestCancel", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Cancel", comment: "Text for the Cancel button on Payment Request modal that shows the order summary")
    
    //Errors
    public static let unsupportedInstrumentMessage = NSLocalizedString("unsupportedInstrumentMessage", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Unsupported payment instruments", comment: "Error message if list of Payment Instruments doesn't include BAT")
    public static let userCancelledMessage = NSLocalizedString("userCancelledMessage", tableName: "BraveShared", bundle: Bundle.braveShared, value: "User cancelled", comment: "Error message if the payment workflow is canceled by the user")
}
