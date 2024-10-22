// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// This code is loosely based on https://github.com/Antol/APAutocompleteTextField

import BraveCore
import BraveUI
import Shared
import UIKit

/// Delegate for the text field events. Since AutocompleteTextField owns the UITextFieldDelegate,
/// callers must use this instead.
protocol AutocompleteTextFieldDelegate: AnyObject {
  func autocompleteTextField(
    _ autocompleteTextField: AutocompleteTextField,
    didEnterText text: String
  )
  func autocompleteTextField(
    _ autocompleteTextField: AutocompleteTextField,
    didDeleteAutoSelectedText text: String
  )
  func autocompleteTextFieldShouldReturn(_ autocompleteTextField: AutocompleteTextField) -> Bool
  func autocompleteTextFieldShouldClear(_ autocompleteTextField: AutocompleteTextField) -> Bool
  func autocompleteTextFieldDidBeginEditing(_ autocompleteTextField: AutocompleteTextField)
  func autocompleteTextFieldDidCancel(_ autocompleteTextField: AutocompleteTextField)
}

private struct AutocompleteTextFieldUX {
  static let highlightColor = UIColor.braveInfoBackground
}

public class AutocompleteTextField: UITextField, UITextFieldDelegate {
  var autocompleteDelegate: AutocompleteTextFieldDelegate?

  // AutocompleteTextLabel repersents the actual autocomplete text.
  // The textfields "text" property only contains the entered text, while this label holds the autocomplete text
  // This makes sure that the autocomplete doesnt mess with keyboard suggestions provided by third party keyboards.
  private var autocompleteTextLabel: UILabel?
  private var hideCursor: Bool = false
  private let privateBrowsingManager: PrivateBrowsingManager

  var isSelectionActive: Bool {
    return autocompleteTextLabel != nil
  }

  // This variable is a solution to get the right behavior for refocusing
  // the AutocompleteTextField. The initial transition into Overlay Mode
  // doesn't involve the user interacting with AutocompleteTextField.
  // Thus, we update shouldApplyCompletion in touchesBegin() to reflect whether
  // the highlight is active and then the text field is updated accordingly
  // in touchesEnd() (eg. applyCompletion() is called or not)
  fileprivate var notifyTextChanged: (() -> Void)?
  fileprivate var notifyTextDeleted: (() -> Void)?
  public var lastReplacement: String?

  var highlightColor = AutocompleteTextFieldUX.highlightColor

  override public var text: String? {
    didSet {
      super.text = text
      self.textDidChange(self)
    }
  }

  override public var accessibilityValue: String? {
    get {
      return (self.text ?? "") + (self.autocompleteTextLabel?.text ?? "")
    }
    set(value) {
      super.accessibilityValue = value
    }
  }

  init(privateBrowsingManager: PrivateBrowsingManager) {
    self.privateBrowsingManager = privateBrowsingManager
    super.init(frame: .zero)
    self.semanticContentAttribute = .forceLeftToRight
    self.textAlignment = .left

    super.delegate = self
    super.addTarget(
      self,
      action: #selector(AutocompleteTextField.textDidChange),
      for: .editingChanged
    )
    notifyTextChanged = debounce(
      0.1,
      action: {
        if self.isEditing {
          self.autocompleteDelegate?.autocompleteTextField(
            self,
            didEnterText: self.text?.preferredSearchSuggestionText ?? ""
          )
        }
      }
    )

    notifyTextDeleted = debounce(
      0.1,
      action: {
        if self.isEditing {
          var text = self.text
          if text?.isEmpty == true && self.autocompleteTextLabel?.text?.isEmpty == false {
            text = self.autocompleteTextLabel?.text
          }
          self.autocompleteDelegate?.autocompleteTextField(
            self,
            didDeleteAutoSelectedText: text?.preferredSearchSuggestionText ?? ""
          )
        }
      }
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override public var keyCommands: [UIKeyCommand]? {
    return [
      UIKeyCommand(
        input: UIKeyCommand.inputLeftArrow,
        modifierFlags: [],
        action: #selector(self.handleKeyCommand(sender:))
      ),
      UIKeyCommand(
        input: UIKeyCommand.inputRightArrow,
        modifierFlags: [],
        action: #selector(self.handleKeyCommand(sender:))
      ),
      UIKeyCommand(
        input: UIKeyCommand.inputEscape,
        modifierFlags: [],
        action: #selector(self.handleKeyCommand(sender:))
      ),
    ]
  }

  @objc func handleKeyCommand(sender: UIKeyCommand) {
    guard let input = sender.input else {
      return
    }
    switch input {
    case UIKeyCommand.inputLeftArrow:
      if isSelectionActive {
        applyCompletion()

        // Set the current position to the beginning of the text.
        selectedTextRange = textRange(from: beginningOfDocument, to: beginningOfDocument)
      } else if let range = selectedTextRange {
        if range.start == beginningOfDocument {
          break
        }

        guard let cursorPosition = position(from: range.start, offset: -1) else {
          break
        }

        selectedTextRange = textRange(from: cursorPosition, to: cursorPosition)
      }
    case UIKeyCommand.inputRightArrow:
      if isSelectionActive {
        applyCompletion()

        // Set the current position to the end of the text.
        selectedTextRange = textRange(from: endOfDocument, to: endOfDocument)
      } else if let range = selectedTextRange {
        if range.end == endOfDocument {
          break
        }

        guard let cursorPosition = position(from: range.end, offset: 1) else {
          break
        }

        selectedTextRange = textRange(from: cursorPosition, to: cursorPosition)
      }
    case UIKeyCommand.inputEscape:
      autocompleteDelegate?.autocompleteTextFieldDidCancel(self)
    default:
      break
    }
  }

  func highlightAll() {
    let text = self.text
    self.text = ""
    setAutocompleteSuggestion(text ?? "")
    selectedTextRange = textRange(from: endOfDocument, to: endOfDocument)
  }

  fileprivate func normalizeString(_ string: String) -> String {
    return string.stringByTrimmingLeadingCharactersInSet(CharacterSet.whitespaces)
  }

  /// Commits the completion by setting the text and removing the highlight.
  fileprivate func applyCompletion() {

    // Clear the current completion, then set the text without the attributed style.
    let text = (self.text ?? "") + (self.autocompleteTextLabel?.text ?? "")
    let didRemoveCompletion = removeCompletion()
    self.text = text
    hideCursor = false
    // Move the cursor to the end of the completion.
    if didRemoveCompletion {
      selectedTextRange = textRange(from: endOfDocument, to: endOfDocument)
    }
  }

  /// Removes the autocomplete-highlighted. Returns true if a completion was actually removed
  @objc @discardableResult fileprivate func removeCompletion() -> Bool {
    let hasActiveCompletion = isSelectionActive
    autocompleteTextLabel?.removeFromSuperview()
    autocompleteTextLabel = nil
    return hasActiveCompletion
  }

  // `shouldChangeCharactersInRange` is called before the text changes, and textDidChange is called after.
  // Since the text has changed, remove the completion here, and textDidChange will fire the callback to
  // get the new autocompletion.
  public func textField(
    _ textField: UITextField,
    shouldChangeCharactersIn range: NSRange,
    replacementString string: String
  ) -> Bool {
    lastReplacement = string
    return true
  }

  func setAutocompleteSuggestion(_ suggestion: String?) {
    let text = self.text ?? ""

    guard let suggestion = suggestion, isEditing && markedTextRange == nil else {
      hideCursor = false
      return
    }

    let normalized = normalizeString(text).lowercased()
    guard suggestion.hasPrefix(normalized) && normalized.count < suggestion.count else {
      hideCursor = false
      return
    }

    let suggestionText = String(
      suggestion.suffix(from: suggestion.index(suggestion.startIndex, offsetBy: normalized.count))
    )
    let autocompleteText = NSMutableAttributedString(string: suggestionText)
    autocompleteText.addAttribute(
      NSAttributedString.Key.backgroundColor,
      value: highlightColor,
      range: NSRange(location: 0, length: suggestionText.count)
    )
    autocompleteTextLabel?.removeFromSuperview()  // should be nil. But just in case
    autocompleteTextLabel = createAutocompleteLabelWith(autocompleteText)
    if let l = autocompleteTextLabel {
      addSubview(l)
      hideCursor = true
      forceResetCursor()
    }
  }

  override public func caretRect(for position: UITextPosition) -> CGRect {
    return hideCursor ? CGRect.zero : super.caretRect(for: position)
  }

  private func createAutocompleteLabelWith(_ autocompleteText: NSAttributedString) -> UILabel {
    let label = UILabel()
    var frame = self.bounds
    label.attributedText = autocompleteText
    label.font = self.font
    label.accessibilityIdentifier = "autocomplete"
    label.backgroundColor = self.backgroundColor
    label.textColor = self.textColor
    label.textAlignment = .left

    let enteredTextSize = self.attributedText?.boundingRect(
      with: self.frame.size,
      options: NSStringDrawingOptions.usesLineFragmentOrigin,
      context: nil
    )
    frame.origin.x = (enteredTextSize?.width.rounded() ?? 0)
    // The autocomplete label overlaps whole uitextfield covering the clear button.
    // The label's frame must be slightly shorter to make the clear button visible.
    let clearButtonOffset: CGFloat = 30
    frame.size.width = self.frame.size.width - frame.origin.x - clearButtonOffset
    frame.size.height = self.frame.size.height
    label.frame = frame
    return label
  }

  public func textFieldDidBeginEditing(_ textField: UITextField) {
    autocompleteDelegate?.autocompleteTextFieldDidBeginEditing(self)
  }

  public func textFieldShouldEndEditing(_ textField: UITextField) -> Bool {
    applyCompletion()
    return true
  }

  public func textFieldShouldReturn(_ textField: UITextField) -> Bool {
    applyCompletion()
    return autocompleteDelegate?.autocompleteTextFieldShouldReturn(self) ?? true
  }

  public func textFieldShouldClear(_ textField: UITextField) -> Bool {
    removeCompletion()
    return autocompleteDelegate?.autocompleteTextFieldShouldClear(self) ?? true
  }

  public func textField(
    _ textField: UITextField,
    editMenuForCharactersIn range: NSRange,
    suggestedActions: [UIMenuElement]
  ) -> UIMenu? {
    guard let selectedRange = textField.selectedTextRange,
      let text = textField.text(in: selectedRange), !text.isEmpty
    else {
      return nil
    }
    if let match = AutocompleteClassifier.classify(text),
      match.type != .searchWhatYouTyped,
      let service = URLSanitizerServiceFactory.get(
        privateMode: privateBrowsingManager.isPrivateBrowsing
      )
    {
      let cleanedURL = service.sanitizeURL(match.destinationURL) ?? match.destinationURL
      let copyCleanLinkAction = UIAction(
        title: Strings.copyCleanLink,
        image: UIImage(braveSystemNamed: "leo.broom"),
        handler: UIAction.deferredActionHandler { _ in
          UIPasteboard.general.url = cleanedURL
        }
      )
      var actions = suggestedActions
      if actions.count < 2 {
        actions.append(copyCleanLinkAction)
      } else {
        actions.insert(copyCleanLinkAction, at: 1)
      }
      return UIMenu(children: actions)
    }
    return nil
  }

  override public func setMarkedText(_ markedText: String?, selectedRange: NSRange) {
    // Clear the autocompletion if any provisionally inserted text has been
    // entered (e.g., a partial composition from a Japanese keyboard).
    removeCompletion()
    super.setMarkedText(markedText, selectedRange: selectedRange)
  }

  func setTextWithoutSearching(_ text: String) {
    super.text = text
    hideCursor = autocompleteTextLabel != nil
    removeCompletion()
  }

  @objc func textDidChange(_ textField: UITextField) {
    hideCursor = autocompleteTextLabel != nil
    removeCompletion()

    let isKeyboardReplacingText = lastReplacement != nil
    // Should not add typed text before marked text is confirmed by user
    let noMarkedText = markedTextRange == nil

    guard isKeyboardReplacingText, noMarkedText else {
      hideCursor = false
      return
    }

    notifyTextChanged?()
  }

  // Reset the cursor to the end of the text field.
  // This forces `caretRect(for position: UITextPosition)` to be called which will decide if we should show the cursor
  // This exists because ` caretRect(for position: UITextPosition)` is not called after we apply an autocompletion.
  private func forceResetCursor() {
    selectedTextRange = nil
    selectedTextRange = textRange(from: endOfDocument, to: endOfDocument)
  }

  override public func deleteBackward() {
    lastReplacement = ""
    hideCursor = false
    if isSelectionActive {
      removeCompletion()
      forceResetCursor()
      notifyTextDeleted?()
    } else {
      super.deleteBackward()
    }
  }

  override public func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
    applyCompletion()
    super.touchesBegan(touches, with: event)
  }

}
