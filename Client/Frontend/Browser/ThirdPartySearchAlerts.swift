/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

class ThirdPartySearchAlerts: UIAlertController {

    /**
    Allows the keyboard to pop back up after an alertview.
    **/
    override var canBecomeFirstResponder: Bool {
        return false
    }

    /**
     Builds the Alert view that asks if the users wants to add a third party search engine.

     - parameter engine: To add engine details to alert

     - parameter completion: Okay option handler.

     - returns: UIAlertController for asking the user to add a search engine
     **/

    static func addThirdPartySearchEngine(_ engine: OpenSearchEngine, completion: @escaping (UIAlertAction) -> Void) -> UIAlertController {
        let alertMessage = """
                            \n\(engine.displayName)
                            \(engine.searchTemplate)
                            \n\(Strings.thirdPartySearchAddMessage)
                            """
        let alert = ThirdPartySearchAlerts(
            title: Strings.thirdPartySearchAddTitle,
            message: alertMessage,
            preferredStyle: .alert
        )

        let noOption = UIAlertAction(
            title: Strings.thirdPartySearchCancelButton,
            style: .cancel,
            handler: completion
        )

        let okayOption = UIAlertAction(
            title: Strings.thirdPartySearchOkayButton,
            style: .default,
            handler: completion
        )

        alert.addAction(okayOption)
        alert.addAction(noOption)

        return alert
    }

    /**
     Builds the Alert view that shows the user an error in case a search engine could not be added.

     - returns: UIAlertController with an error dialog
     **/

    static func failedToAddThirdPartySearch() -> UIAlertController {
        return searchAlertWithOK(title: Strings.thirdPartySearchFailedTitle,
                                 message: Strings.thirdPartySearchFailedMessage)
    }
    
    static func incorrectCustomEngineForm() -> UIAlertController {
        return searchAlertWithOK(title: Strings.thirdPartySearchFailedTitle,
                                      message: Strings.customEngineFormErrorMessage)
    }
    
    static func duplicateCustomEngine() -> UIAlertController {
        return searchAlertWithOK(title: Strings.thirdPartySearchFailedTitle,
                                 message: Strings.customEngineDuplicateErrorMessage)
    }
    
    static func missingInfoToAddThirdPartySearch() -> UIAlertController {
        return searchAlertWithOK(title: Strings.thirdPartySearchFailedTitle,
                                 message: Strings.customEngineFillAllFieldsErrorMessage)
    }
    
    static func insecureURLEntryThirdPartySearch() -> UIAlertController {
        return searchAlertWithOK(title: Strings.thirdPartySearchFailedTitle,
                                 message: Strings.customEngineFormErrorMessage)
    }
    
    private static func searchAlertWithOK(title: String, message: String) -> UIAlertController {
        let alert = ThirdPartySearchAlerts(
            title: title,
            message: message,
            preferredStyle: .alert
        )
        
        let okayOption = UIAlertAction(
            title: Strings.thirdPartySearchOkayButton,
            style: .default,
            handler: nil
        )
        
        alert.addAction(okayOption)
        return alert
    }

}
