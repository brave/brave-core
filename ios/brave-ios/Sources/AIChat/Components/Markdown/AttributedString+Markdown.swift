// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

extension AttributedString {
  private static let codeBlockForegroundColor = Color.white
  private static let codeInlineForegroundColor = Color.purple

  private static var options: AttributedString.MarkdownParsingOptions {
    var result = AttributedString.MarkdownParsingOptions()
    result.allowsExtendedAttributes = true
    result.interpretedSyntax = .full
    result.failurePolicy = .returnPartiallyParsedIfPossible
    result.languageCode = nil
    result.appliesSourcePositionAttributes = false
    return result
  }

  init(markdown: String, preferredFont: Font) throws {
    var result = try AttributedString(
      markdown: markdown,
      options: AttributedString.options,
      baseURL: nil
    )

    result.font = preferredFont

    result.runs[InlinePresentationAttribute.self]
      .reversed()
      .forEach { (intent, range) in
        guard let intent = intent else {
          return
        }

        var sourceAttributes = AttributeContainer()
        sourceAttributes.inlinePresentationIntent = intent

        var targetAttributes = AttributeContainer()

        switch intent {
        case .emphasized:
          targetAttributes.font = preferredFont.italic()
        case .stronglyEmphasized:
          targetAttributes.font = preferredFont.bold()
        case .code:
          targetAttributes.font = preferredFont.monospaced()
          targetAttributes.foregroundColor = AttributedString.codeInlineForegroundColor
        case .strikethrough:
          targetAttributes.strikethroughStyle = .single
        case .softBreak:
          // A space
          break
        case .lineBreak:
          // A new line
          break
        case .inlineHTML:
          break
        case .blockHTML:
          break
        default:
          break
        }

        result = result.replacingAttributes(sourceAttributes, with: targetAttributes)
      }

    result.runs[PresentationAttribute.self]
      .reversed()
      .forEach { (intentAttribute, range) in
        guard let intentAttribute = intentAttribute else {
          return
        }

        var listType = MarkdownListType.none

        for intent in intentAttribute.components {
          switch intent.kind {
          case .paragraph:
            break
          case .header(let level):
            switch level {
            case 1:
              result[range].font = .system(.title).bold()
            case 2:
              result[range].font = .system(.title2).bold()
            case 3:
              result[range].font = .system(.title3).bold()
            case 4:
              result[range].font = .system(.headline).bold()
            case 5:
              result[range].font = .system(.body).bold()
            case 6:
              result[range].font = .system(.callout).bold()
            default:
              break
            }
          case .orderedList:
            listType = .ordered
          case .unorderedList:
            listType = .unordered
          case .listItem(let index):
            if listType != .unordered {
              listType = .ordered
            }

            if listType == .ordered {
              result.characters.insert(contentsOf: "\(index).\t", at: range.lowerBound)
            } else {
              result.characters.insert(contentsOf: "•\t", at: range.lowerBound)
            }
          case .codeBlock(_):
            result[range].font = preferredFont.monospaced()  //.italic()
            result[range].foregroundColor = AttributedString.codeBlockForegroundColor
          case .blockQuote:
            result[range].font = preferredFont.monospaced()

          case .thematicBreak:
            break

          case .table(_):
            break

          case .tableHeaderRow:
            break

          case .tableRow(_):
            break

          case .tableCell(_):
            break

          @unknown default:
            break
          }
        }

        if range.lowerBound != result.startIndex {
          result.characters.insert(contentsOf: "\n", at: range.lowerBound)
        }
      }

    self = result
  }

  private typealias LinkAttribute = AttributeScopes.FoundationAttributes.LinkAttribute
  private typealias PresentationAttribute = AttributeScopes.FoundationAttributes
    .PresentationIntentAttribute
  private typealias InlinePresentationAttribute = AttributeScopes.FoundationAttributes
    .InlinePresentationIntentAttribute

  private enum MarkdownListType {
    case none
    case unordered
    case ordered
  }
}

extension AttributedString {
  public mutating func trimmingCharacters(in characterSet: CharacterSet) -> AttributedString {
    let invertedSet = characterSet.inverted

    let startIndex = self.characters.firstIndex(where: {
      $0.unicodeScalars.allSatisfy(invertedSet.contains(_:))
    })

    let endIndex = self.characters.lastIndex(where: {
      $0.unicodeScalars.allSatisfy(invertedSet.contains(_:))
    })

    guard let startIndex = startIndex,
      let endIndex = endIndex,
      startIndex < self.index(afterCharacter: endIndex)
    else {
      self.removeSubrange(self.startIndex..<self.endIndex)
      return self
    }

    self.removeSubrange(self.index(afterCharacter: endIndex)...)
    self.removeSubrange(self.startIndex..<startIndex)

    return self
  }
}

extension AttributedSubstring {
  public func trimmingCharacters(in characterSet: CharacterSet) -> AttributedSubstring {
    let invertedSet = characterSet.inverted

    let startIndex = self.characters.firstIndex(where: {
      $0.unicodeScalars.allSatisfy(invertedSet.contains(_:))
    })

    let endIndex = self.characters.lastIndex(where: {
      $0.unicodeScalars.allSatisfy(invertedSet.contains(_:))
    })

    guard let startIndex = startIndex,
      let endIndex = endIndex,
      startIndex < endIndex
    else {
      return self[self.startIndex..<self.startIndex]
    }

    return self[startIndex...endIndex]
  }
}
