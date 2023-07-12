/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import UIKit

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
      \n\(Strings.CustomSearchEngine.searchTemplateTitle)
      \(engine.searchTemplate)
      \n\(Strings.CustomSearchEngine.suggestionTemplateTitle)
      \(engine.suggestTemplate ?? "N/A")
      \n\(Strings.CustomSearchEngine.thirdPartySearchEngineAddAlertDescription)
      """
    let alert = ThirdPartySearchAlerts(
      title: Strings.CustomSearchEngine.thirdPartySearchEngineAddAlertTitle,
      message: alertMessage,
      preferredStyle: .alert
    )

    let noOption = UIAlertAction(
      title: Strings.cancelButtonTitle,
      style: .cancel,
      handler: completion
    )

    let okayOption = UIAlertAction(
      title: Strings.OKString,
      style: .default,
      handler: completion
    )

    alert.addAction(okayOption)
    alert.addAction(noOption)

    return alert
  }
  
  static func insecureSearchTemplateURL(_ engine: OpenSearchEngine) -> UIAlertController {
    let alertMessage = """
      \n\(Strings.CustomSearchEngine.insecureSearchTemplateURLErrorDescription)"
      \(engine.displayName)
      \n\(Strings.CustomSearchEngine.searchTemplateTitle)
      \(engine.searchTemplate)
      """
    return searchAlertWithOK(
      title: Strings.CustomSearchEngine.customSearchEngineAddErrorTitle,
      message: alertMessage)
  }
  
  static func engineAlreadyExists(_ engine: OpenSearchEngine) -> UIAlertController {
    let alertMessage = """
      \n\(engine.displayName)
      \n\(Strings.CustomSearchEngine.engineExistsAlertDescription)
      """
    return searchAlertWithOK(
      title: Strings.CustomSearchEngine.customSearchEngineAddErrorTitle,
      message: alertMessage)
  }
  
  static func insecureSuggestionTemplateURL(_ engine: OpenSearchEngine) -> UIAlertController {
    let alertMessage = """
      \n\(Strings.CustomSearchEngine.insecureSuggestionTemplateURLErrorDescription)
      \(engine.displayName)
      \n\(Strings.CustomSearchEngine.suggestionTemplateTitle)
      \(engine.suggestTemplate ?? "")
      """
    return searchAlertWithOK(
      title: Strings.CustomSearchEngine.customSearchEngineAddErrorTitle,
      message: alertMessage)
  }

  /**
   Builds the Alert view that shows the user an error in case a search engine could not be added.
   - returns: UIAlertController with an error dialog
  **/

  static func failedToAddThirdPartySearch() -> UIAlertController {
    return searchAlertWithOK(
      title: Strings.CustomSearchEngine.thirdPartySearchEngineAddErrorTitle,
      message: Strings.CustomSearchEngine.thirdPartySearchEngineAddErrorDescription)
  }

  static func missingInfoToAddThirdPartySearch() -> UIAlertController {
    return searchAlertWithOK(
      title: Strings.CustomSearchEngine.thirdPartySearchEngineAddErrorTitle,
      message: Strings.CustomSearchEngine.thirdPartySearchEngineMissingInfoErrorDescription)
  }

  static func incorrectCustomEngineForm() -> UIAlertController {
    return searchAlertWithOK(
      title: Strings.CustomSearchEngine.thirdPartySearchEngineIncorrectFormErrorTitle,
      message: Strings.CustomSearchEngine.thirdPartySearchEngineIncorrectFormErrorDescription)
  }

  static func duplicateCustomEngine() -> UIAlertController {
    return searchAlertWithOK(
      title: Strings.CustomSearchEngine.thirdPartySearchEngineAddErrorTitle,
      message: Strings.CustomSearchEngine.thirdPartySearchEngineDuplicateErrorDescription)
  }

  static func insecureURLEntryThirdPartySearch() -> UIAlertController {
    return searchAlertWithOK(
      title: Strings.CustomSearchEngine.thirdPartySearchEngineAddErrorTitle,
      message: Strings.CustomSearchEngine.thirdPartySearchEngineInsecureURLErrorDescription)
  }

  private static func searchAlertWithOK(title: String, message: String) -> UIAlertController {
    let alert = ThirdPartySearchAlerts(
      title: title,
      message: message,
      preferredStyle: .alert
    )

    let okayOption = UIAlertAction(
      title: Strings.OKString,
      style: .default,
      handler: nil
    )

    alert.addAction(okayOption)
    return alert
  }

}
