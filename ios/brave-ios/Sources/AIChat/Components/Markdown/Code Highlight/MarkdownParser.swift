// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

public class MarkdownParser {
  public struct CodeBlock: Hashable {
    public let languageHint: String?
    public let backgroundColor: Color
  }

  public struct StringBlock: Hashable {
    public let string: AttributedString
    public let codeBlock: CodeBlock?
  }

  public static func prepareMarkdownParser(
    isDarkTheme: Bool
  ) async {
    let highlighter = isDarkTheme ? HighlightJS.dark : HighlightJS.light
    await highlighter.prepareHighlighter()
  }

  public static func parse(
    string: String,
    preferredFont: UIFont,
    useHLJS: Bool,
    isDarkTheme: Bool
  ) -> [StringBlock]? {
    guard let string = try? AttributedString(markdown: string, preferredFont: .init(preferredFont))
    else {
      return nil
    }

    var codeBlocks = [(languageHint: String?, range: Range<AttributedString.Index>)]()

    // Chunk the string into text and code-blocks with their ranges
    string.runs[AttributeScopes.FoundationAttributes.PresentationIntentAttribute.self]
      .forEach { (intentAttribute, range) in
        guard let intentAttribute = intentAttribute else {
          return
        }

        for intent in intentAttribute.components {
          if case .codeBlock(let languageHint) = intent.kind {
            codeBlocks.append((languageHint, range))
          }
        }
      }

    codeBlocks.sort { $0.range.lowerBound < $1.range.lowerBound }

    var result = [StringBlock]()
    var startIndex = string.startIndex
    for (languageHint, range) in codeBlocks {
      if startIndex < range.lowerBound {
        result.append(
          StringBlock(
            string: AttributedString(
              string[startIndex..<range.lowerBound]
                .trimmingCharacters(in: .whitespacesAndNewlines)
            ),
            codeBlock: nil
          )
        )
      }

      let string = string[range]
        .trimmingCharacters(in: .whitespacesAndNewlines)

      // Use Highlight-JS
      if useHLJS {
        let highlighter = isDarkTheme ? HighlightJS.dark : HighlightJS.light
        if let highlighted = highlighter.highlight(
          String(string.characters),
          preferredFont: preferredFont,
          language: languageHint
        ) {
          result.append(
            StringBlock(
              string: highlighted.string,
              codeBlock: CodeBlock(
                languageHint: languageHint,
                backgroundColor: highlighted.backgroundColor
                  ?? Color(UIColor(rgb: isDarkTheme ? 0x282C34 : 0xFAFAFA))
              )
            )
          )

          // Advance to the next range
          startIndex = range.upperBound
          continue
        }
      }

      result.append(
        StringBlock(
          string: AttributedString(string),
          codeBlock: CodeBlock(
            languageHint: languageHint,
            backgroundColor: Color(UIColor(rgb: isDarkTheme ? 0x282C34 : 0xFAFAFA))
          )
        )
      )

      // Advance to the next range
      startIndex = range.upperBound
    }

    if startIndex < string.endIndex {
      result.append(
        StringBlock(
          string: AttributedString(
            string[startIndex..<string.endIndex].trimmingCharacters(in: .whitespacesAndNewlines)
          ),
          codeBlock: nil
        )
      )
    }
    return result.isEmpty ? nil : result
  }
}
