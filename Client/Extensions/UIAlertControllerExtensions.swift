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
            title: NSLocalizedString("Oops! Brave crashed", comment: "Title for prompt displayed to user after the app crashes"),
            message: NSLocalizedString("Send a crash report so Mozilla can fix the problem?", comment: "Message displayed in the crash dialog above the buttons used to select when sending reports"),
            preferredStyle: .alert
        )
        
        let sendReport = UIAlertAction(
            title: NSLocalizedString("Send Report", comment: "Used as a button label for crash dialog prompt"),
            style: .default,
            handler: sendReportCallback
        )
        
        let alwaysSend = UIAlertAction(
            title: NSLocalizedString("Always Send", comment: "Used as a button label for crash dialog prompt"),
            style: .default,
            handler: alwaysSendCallback
        )
        
        let dontSend = UIAlertAction(
            title: NSLocalizedString("Don’t Send", comment: "Used as a button label for crash dialog prompt"),
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
            title: NSLocalizedString("Well, this is embarrassing.", comment: "Restore Tabs Prompt Title"),
            message: NSLocalizedString("Looks like Brave crashed previously. Would you like to restore your tabs?", comment: "Restore Tabs Prompt Description"),
            preferredStyle: .alert
        )
        
        let noOption = UIAlertAction(
            title: NSLocalizedString("No", comment: "Restore Tabs Negative Action"),
            style: .cancel,
            handler: noCallback
        )
        
        let okayOption = UIAlertAction(
            title: NSLocalizedString("Okay", comment: "Restore Tabs Affirmative Action"),
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
            message: NSLocalizedString("This action will clear all of your private data. It cannot be undone.", tableName: "ClearPrivateDataConfirm", comment: "Description of the confirmation dialog shown when a user tries to clear their private data."),
            preferredStyle: .alert
        )
        
        let noOption = UIAlertAction(
            title: NSLocalizedString("Cancel", tableName: "ClearPrivateDataConfirm", comment: "The cancel button when confirming clear private data."),
            style: .cancel,
            handler: nil
        )
        
        let okayOption = UIAlertAction(
            title: NSLocalizedString("OK", tableName: "ClearPrivateDataConfirm", comment: "The button that clears private data."),
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
            message: NSLocalizedString("This action will clear all of your private data, including history from your synced devices.", tableName: "ClearHistoryConfirm", comment: "Description of the confirmation dialog shown when a user tries to clear history that's synced to another device."),
            preferredStyle: .alert
        )
        
        let noOption = UIAlertAction(
            title: NSLocalizedString("Cancel", tableName: "ClearHistoryConfirm", comment: "The cancel button when confirming clear history."),
            style: .cancel,
            handler: nil
        )
        
        let okayOption = UIAlertAction(
            title: NSLocalizedString("OK", tableName: "ClearHistoryConfirm", comment: "The confirmation button that clears history even when Sync is connected."),
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
        
        let areYouSureTitle = NSLocalizedString("Are you sure?",
                                                tableName: "LoginManager",
                                                comment: "Prompt title when deleting logins")
        let deleteLocalMessage = NSLocalizedString("Logins will be permanently removed.",
                                                   tableName: "LoginManager",
                                                   comment: "Prompt message warning the user that deleting non-synced logins will permanently remove them")
        let deleteSyncedDevicesMessage = NSLocalizedString("Logins will be removed from all connected devices.",
                                                           tableName: "LoginManager",
                                                           comment: "Prompt message warning the user that deleted logins will remove logins from all connected devices")
        let cancelActionTitle = NSLocalizedString("Cancel",
                                                  tableName: "LoginManager",
                                                  comment: "Prompt option for cancelling out of deletion")
        let deleteActionTitle = NSLocalizedString("Delete",
                                                  tableName: "LoginManager",
                                                  comment: "Label for the button used to delete the current login.")
        
        let deleteAlert: UIAlertController
        if hasSyncedLogins {
            deleteAlert = UIAlertController(title: areYouSureTitle, message: deleteSyncedDevicesMessage, preferredStyle: .alert)
        } else {
            deleteAlert = UIAlertController(title: areYouSureTitle, message: deleteLocalMessage, preferredStyle: .alert)
        }
        
        let cancelAction = UIAlertAction(title: cancelActionTitle, style: .cancel, handler: nil)
        let deleteAction = UIAlertAction(title: deleteActionTitle, style: .destructive, handler: deleteCallback)
        
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
        
        let okAlertAction = UIAlertAction(title: Strings.OK, style: .default) { _ in
            guard let textFields = self.alert.textFields else { return }
            
            let secondOptionalInput = textFields.count > 1 ? textFields[1].text : nil
            actionSelected(input: textFields.first?.text, input2: secondOptionalInput)
        }
        okAction = okAlertAction
        
        let cancelAction = UIAlertAction(title: Strings.Cancel, style: UIAlertActionStyle.cancel) { (alertA: UIAlertAction!) in
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
