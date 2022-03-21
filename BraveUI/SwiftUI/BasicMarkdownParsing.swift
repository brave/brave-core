// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Markdown

/// A basic markdown visitor used to create a SwiftUI `Text` view.
///
/// Currently only supports parsing:
///   - italic
///   - bold
///   - strikethrough
private struct BasicMarkdownVisitor: MarkupVisitor {
  typealias Result = SwiftUI.Text

  mutating func defaultVisit(_ markup: Markup) -> Result {
    markup.children.reduce(Result(""), { $0 + visit($1) })
  }

  mutating func visitText(_ text: Markdown.Text) -> Result {
    Result(text.plainText)
  }

  mutating func visitEmphasis(_ emphasis: Emphasis) -> Result {
    emphasis.children.reduce(Result(""), { $0 + visit($1) }).italic()
  }

  mutating func visitStrong(_ strong: Strong) -> Result {
    strong.children.reduce(Result(""), { $0 + visit($1) }).bold()
  }

  mutating func visitStrikethrough(_ strikethrough: Strikethrough) -> Result {
    strikethrough.children.reduce(Result(""), { $0 + visit($1) }).strikethrough()
  }
}

@available(iOS, introduced: 14.0, deprecated: 15.0, message: "iOS 15 introduces built-in markdown parsing that includes more supported formats")
extension SwiftUI.Text {
  /// Creates a text view from a parsed markdown document
  ///
  /// - note: Only bold, italic and strikethrough are supported currently.
  public init(_ document: Markdown.Document) {
    var visitor = BasicMarkdownVisitor()
    self = visitor.visit(document)
  }
  /// Creates a text view from a markdown formatted string.
  ///
  /// - note: Only bold, italic and strikethrough are supported currently.
  /// - warning: This parses markdown each time `Text` is initialized which will happen at each `body`
  ///            recomputation. If you need better performance and your View updates frequently, consider
  ///            using the initializer that takes a `Markdown.Document` that you can store and pass in
  public init(markdown text: String) {
    self.init(Document(parsing: text))
  }
}
