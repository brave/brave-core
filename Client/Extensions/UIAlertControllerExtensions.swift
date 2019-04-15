/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

typealias UIAlertActionCallback = (UIAlertAction) -> Void

// MARK: - Extension methods for building specific UIAlertController instances used across the app
extension UIAlertController {
    
    /**
     Builds the Alert view that asks the user if they wish to opt into crash reporting.
     
     - parameter sendReportCallback: Send report option handler
     - parameter alwaysSendCallback: Always send option handler
     - parameter dontSendCallback:   Dont send option handler
     - parameter neverSendCallback:  Never send option handler
     
     - returns: UIAlertController for opting into crash reporting after a crash occurred
     */
    class func crashOptInAlert(
        _ sendReportCallback: @escaping UIAlertActionCallback,
        alwaysSendCallback: @escaping UIAlertActionCallback,
        dontSendCallback: @escaping UIAlertActionCallback) -> UIAlertController {
        
        let alert = UIAlertController(
            title: Strings.SendCrashReportAlertTitle,
            message: Strings.SendCrashReportAlertMessage,
            preferredStyle: .alert
        )
        
        let sendReport = UIAlertAction(
            title: Strings.SendReportButtonTitle,
            style: .default,
            handler: sendReportCallback
        )
        
        let alwaysSend = UIAlertAction(
            title: Strings.AlwaysSendButtonTitle,
            style: .default,
            handler: alwaysSendCallback
        )
        
        let dontSend = UIAlertAction(
            title: Strings.DontSendButtonTitle,
            style: .default,
            handler: dontSendCallback
        )
        
        alert.addAction(sendReport)
        alert.addAction(alwaysSend)
        alert.addAction(dontSend)
        
        return alert
    }
    
    /**
     Builds the Alert view that asks the user if they wish to restore their tabs after a crash.
     
     - parameter okayCallback: Okay option handler
     - parameter noCallback:   No option handler
     
     - returns: UIAlertController for asking the user to restore tabs after a crash
     */
    class func restoreTabsAlert(okayCallback: @escaping UIAlertActionCallback, noCallback: @escaping UIAlertActionCallback) -> UIAlertController {
        let alert = UIAlertController(
            title: Strings.RestoreTabOnCrashAlertTitle,
            message: Strings.RestoreTabOnCrashAlertMessage,
            preferredStyle: .alert
        )
        
        let noOption = UIAlertAction(
            title: Strings.RestoreTabNegativeButtonTitle,
            style: .cancel,
            handler: noCallback
        )
        
        let okayOption = UIAlertAction(
            title: Strings.RestoreTabAffirmativeButtonTitle,
            style: .default,
            handler: okayCallback
        )
        
        alert.addAction(okayOption)
        alert.addAction(noOption)
        return alert
    }
    
    class func clearPrivateDataAlert(okayCallback: @escaping (UIAlertAction) -> Void) -> UIAlertController {
        let alert = UIAlertController(
            title: "",
            message: Strings.ClearPrivateDataAlertMessage,
            preferredStyle: .alert
        )
        
        let noOption = UIAlertAction(
            title: Strings.ClearPrivateDataAlertCancelButtonTitle,
            style: .cancel,
            handler: nil
        )
        
        let okayOption = UIAlertAction(
            title: Strings.ClearPrivateDataAlertOkButtonTitle,
            style: .destructive,
            handler: okayCallback
        )
        
        alert.addAction(okayOption)
        alert.addAction(noOption)
        return alert
    }
    
    /**
     Builds the Alert view that asks if the users wants to also delete history stored on their other devices.
     
     - parameter okayCallback: Okay option handler.
     
     - returns: UIAlertController for asking the user to restore tabs after a crash
     */
    
    class func clearSyncedHistoryAlert(okayCallback: @escaping (UIAlertAction) -> Void) -> UIAlertController {
        let alert = UIAlertController(
            title: "",
            message: Strings.ClearSyncedHistoryAlertMessage,
            preferredStyle: .alert
        )
        
        let noOption = UIAlertAction(
            title: Strings.ClearSyncedHistoryAlertCancelButtoTitle,
            style: .cancel,
            handler: nil
        )
        
        let okayOption = UIAlertAction(
            title: Strings.ClearSyncedHistoryAlertOkButtoTitle,
            style: .destructive,
            handler: okayCallback
        )
        
        alert.addAction(okayOption)
        alert.addAction(noOption)
        return alert
    }
    
    /**
     Creates an alert view to warn the user that their logins will either be completely deleted in the
     case of local-only logins or deleted across synced devices in synced account logins.
     
     - parameter deleteCallback: Block to run when delete is tapped.
     - parameter hasSyncedLogins: Boolean indicating the user has logins that have been synced.
     
     - returns: UIAlertController instance
     */
    class func deleteLoginAlertWithDeleteCallback(
        _ deleteCallback: @escaping UIAlertActionCallback,
        hasSyncedLogins: Bool) -> UIAlertController {
        
        let deleteAlert: UIAlertController
        if hasSyncedLogins {
            deleteAlert = UIAlertController(title: Strings.DeleteLoginAlertTitle, message: Strings.DeleteLoginAlertSyncedDevicesMessage, preferredStyle: .alert)
        } else {
            deleteAlert = UIAlertController(title: Strings.DeleteLoginAlertTitle, message: Strings.DeleteLoginAlertLocalMessage, preferredStyle: .alert)
        }
        
        let cancelAction = UIAlertAction(title: Strings.DeleteLoginAlertCancelActionTitle, style: .cancel, handler: nil)
        let deleteAction = UIAlertAction(title: Strings.DeleteLoginButtonTitle, style: .destructive, handler: deleteCallback)
        
        deleteAlert.addAction(cancelAction)
        deleteAlert.addAction(deleteAction)
        
        return deleteAlert
    }
    
    // Enabled this facade for much easier discoverability, instead of using class directly
    /**
     Creates an alert view to collect a string from the user
     
     - parameter title: String to display as the alert title.
     - parameter message: String to display as the alert message.
     - parameter startingText: String to prefill the textfield with.
     - parameter placeholder: String to use for the placeholder text on the text field.
     - parameter keyboardType: Keyboard type of the text field.
     - parameter startingText2: String to prefill the second optional textfield with.
     - parameter placeholder2: String to use for the placeholder text on the second optional text field.
     - parameter keyboardType2: Keyboard type of the text second optional field.
     - parameter forcedInput: Bool whether the user needs to enter _something_ in order to enable OK button.
     - parameter callbackOnMain: Block to run on main thread when the user performs an action.
     
     - returns: UIAlertController instance
     */
    class func userTextInputAlert(title: String,
                                  message: String,
                                  startingText: String? = nil,
                                  placeholder: String? = Strings.Name,
                                  keyboardType: UIKeyboardType? = nil,
                                  startingText2: String? = nil,
                                  placeholder2: String? = Strings.Name,
                                  keyboardType2: UIKeyboardType? = nil,
                                  forcedInput: Bool = true,
                                  callbackOnMain: @escaping (_ input: String?, _ input2: String?) -> Void) -> UIAlertController {
        // Returning alert, so no external, strong reference to initial instance
        return UserTextInputAlert(title: title, message: message,
                                  startingText: startingText,
                                  placeholder: placeholder,
                                  keyboardType: keyboardType,
                                  startingText2: startingText2,
                                  placeholder2: placeholder2,
                                  keyboardType2: keyboardType2,
                                  forcedInput: forcedInput,
                                  callbackOnMain: callbackOnMain).alert
    }
}

// Not part of extension due to needing observing
// Would make private but objc runtime cannot find textfield observing callback
class UserTextInputAlert {
    private weak var okAction: UIAlertAction!
    private(set) var alert: UIAlertController!
    
    required init(title: String, message: String,
                  startingText: String?,
                  placeholder: String?,
                  keyboardType: UIKeyboardType? = nil,
                  startingText2: String? = nil,
                  placeholder2: String? = nil,
                  keyboardType2: UIKeyboardType? = nil,
                  forcedInput: Bool = true,
                  callbackOnMain: @escaping (_ input: String?, _ input2: String?) -> Void) {
        
        alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
        
        func actionSelected(input: String?, input2: String?) {
            DispatchQueue.main.async {
                callbackOnMain(input, input2)
            }
        }
        
        let okAlertAction = UIAlertAction(title: Strings.OKString, style: .default) { _ in
            guard let textFields = self.alert.textFields else { return }
            
            let secondOptionalInput = textFields.count > 1 ? textFields[1].text : nil
            actionSelected(input: textFields.first?.text, input2: secondOptionalInput)
        }
        okAction = okAlertAction
        
        let cancelAction = UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel) { (alertA: UIAlertAction!) in
            actionSelected(input: nil, input2: nil)
        }
        
        self.okAction.isEnabled = !forcedInput
        
        alert.addAction(self.okAction)
        alert.addAction(cancelAction)
        
        alert.addTextField(configurationHandler: textFieldConfig(text: startingText, placeholder: placeholder,
                                                                 keyboardType: keyboardType, forcedInput: forcedInput))
        
        if startingText2 != nil {
            alert.addTextField(configurationHandler: textFieldConfig(text: startingText2, placeholder: placeholder2,
                                                                     keyboardType: keyboardType2, forcedInput: forcedInput))
        }
    }
    
    private func textFieldConfig(text: String?, placeholder: String?, keyboardType: UIKeyboardType?, forcedInput: Bool)
        -> (UITextField) -> Void {
            return { textField in
                textField.placeholder = placeholder
                textField.isSecureTextEntry = false
                textField.keyboardAppearance = .dark
                textField.autocapitalizationType = keyboardType == .URL ? .none : .words
                textField.autocorrectionType = keyboardType == .URL ? .no : .default
                textField.returnKeyType = .done
                textField.text = text
                textField.keyboardType = keyboardType ?? .default
                if forcedInput {
                    textField.addTarget(self, action: #selector(self.textFieldChanged), for: .editingChanged)
                }
            }
    }
    
    @objc private func textFieldChanged() {
        guard let textFields = alert.textFields, let firstText = textFields.first?.text  else { return }
        
        switch textFields.count {
        case 1:
            okAction.isEnabled = !firstText.isEmpty
        case 2:
            guard let lastText = textFields.last?.text else { break }
            okAction.isEnabled = !firstText.isEmpty && !lastText.isEmpty
        default:
            return
        }
    }
}
