// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import XCTest

@testable import AIChat

final class AIChatTest: XCTestCase {

  override func setUpWithError() throws {
    // Put setup code here. This method is called before the invocation of each test method in the class.
  }

  override func tearDownWithError() throws {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
  }

  func testAttributedStringTrimming() throws {
    var string = AttributedString(" test ")
    XCTAssertEqual(
      String(string.trimmingCharacters(in: .whitespacesAndNewlines).characters),
      "test"
    )

    var string2 = AttributedString(" test\n")
    XCTAssertEqual(
      String(string2.trimmingCharacters(in: .whitespacesAndNewlines).characters),
      "test"
    )

    var string3 = AttributedString(" ")
    XCTAssertEqual(String(string3.trimmingCharacters(in: .whitespacesAndNewlines).characters), "")

    var string4 = AttributedString("\n")
    XCTAssertEqual(String(string4.trimmingCharacters(in: .whitespacesAndNewlines).characters), "")

    var string5 = AttributedString("")
    XCTAssertEqual(String(string5.trimmingCharacters(in: .whitespacesAndNewlines).characters), "")

    var string6 = AttributedString("test")
    XCTAssertEqual(
      String(string6.trimmingCharacters(in: .whitespacesAndNewlines).characters),
      "test"
    )
  }

  func testAttributedSubstringTrimming() throws {
    let string = AttributedString(" test ")
    XCTAssertEqual(
      String(
        string[string.startIndex..<string.endIndex].trimmingCharacters(in: .whitespacesAndNewlines)
          .characters
      ),
      "test"
    )

    let string2 = AttributedString(" test\n")
    XCTAssertEqual(
      String(
        string2[string2.startIndex..<string2.endIndex].trimmingCharacters(
          in: .whitespacesAndNewlines
        ).characters
      ),
      "test"
    )

    let string3 = AttributedString(" ")
    XCTAssertEqual(
      String(
        string3[string3.startIndex..<string3.endIndex].trimmingCharacters(
          in: .whitespacesAndNewlines
        ).characters
      ),
      ""
    )

    let string4 = AttributedString("\n")
    XCTAssertEqual(
      String(
        string4[string4.startIndex..<string4.endIndex].trimmingCharacters(
          in: .whitespacesAndNewlines
        ).characters
      ),
      ""
    )

    let string5 = AttributedString("")
    XCTAssertEqual(
      String(
        string5[string5.startIndex..<string5.endIndex].trimmingCharacters(
          in: .whitespacesAndNewlines
        ).characters
      ),
      ""
    )

    let string6 = AttributedString("test")
    XCTAssertEqual(
      String(
        string6[string6.startIndex..<string6.endIndex].trimmingCharacters(
          in: .whitespacesAndNewlines
        ).characters
      ),
      "test"
    )
  }

  func testMarkdown() throws {
    let response =
      """
      The following is ane example of hello world in c++
      ```c++
      #include <iostream>

      int main() {
          std::cout<<"Hello World."
          return 0;
      }
      ```
      This code using the header `iostream` in order to print.
      """

    guard
      let result = MarkdownParser.parse(
        string: response,
        preferredFont: .systemFont(ofSize: 13.0),
        useHLJS: false,
        isDarkTheme: true
      )
    else {
      XCTFail()
      return
    }

    XCTAssert(result.count == 3)
    XCTAssert(result[0].codeBlock == nil)
    XCTAssert(result[1].codeBlock != nil)
    XCTAssert(result[2].codeBlock == nil)

    XCTAssertEqual(
      String(result[0].string.characters),
      "The following is ane example of hello world in c++"
    )
    XCTAssertEqual(
      String(result[1].string.characters),
      "#include <iostream>\n\nint main() {\n    std::cout<<\"Hello World.\"\n    return 0;\n}"
    )
    XCTAssertEqual(
      String(result[2].string.characters),
      "This code using the header iostream in order to print."
    )
  }

  func testHighlightJS() throws {
    let response =
      """
      The following is ane example of hello world in c++
      ```c++
      #include <iostream>

      int main() {
          std::cout<<"Hello World."
          return 0;
      }
      ```
      This code using the header `iostream` in order to print.
      """

    guard
      let result = MarkdownParser.parse(
        string: response,
        preferredFont: .systemFont(ofSize: 13.0),
        useHLJS: true,
        isDarkTheme: true
      )
    else {
      XCTFail()
      return
    }

    XCTAssert(result.count == 3)
    XCTAssert(result[0].codeBlock == nil)
    XCTAssert(result[1].codeBlock != nil)
    XCTAssert(result[2].codeBlock == nil)

    XCTAssertEqual(
      String(result[0].string.characters),
      "The following is ane example of hello world in c++"
    )
    XCTAssertEqual(
      String(result[1].string.characters),
      "#include <iostream>\n\nint main() {\n    std::cout<<\"Hello World.\"\n    return 0;\n}"
    )
    XCTAssertEqual(
      String(result[2].string.characters),
      "This code using the header iostream in order to print."
    )
  }

  func testCSSParserMinified() throws {
    let css = BasicCSSParser.parse(
      """
      pre code.hljs{display:block;overflow-x:auto;padding:1em}code.hljs{padding:3px 5px}.hljs{color:#abb2bf;background:#282c34}.hljs-comment,.hljs-quote{color:#5c6370;font-style:italic}
      """
    )

    XCTAssert(css.count == 3)

    XCTAssertEqual(css[0].selector, ".hljs")
    XCTAssertEqual(
      css[0].declaration,
      Set([
        BasicCSSParser.CSSDeclaration(property: "display", value: "block"),
        BasicCSSParser.CSSDeclaration(property: "padding", value: "1em"),
        BasicCSSParser.CSSDeclaration(property: "background", value: "#282c34"),
        BasicCSSParser.CSSDeclaration(property: "overflow-x", value: "auto"),
        BasicCSSParser.CSSDeclaration(property: "padding", value: "3px 5px"),
        BasicCSSParser.CSSDeclaration(property: "color", value: "#abb2bf"),
      ])
    )

    XCTAssertEqual(css[1].selector, ".hljs-comment")
    XCTAssertEqual(
      css[1].declaration,
      Set([
        BasicCSSParser.CSSDeclaration(property: "font-style", value: "italic"),
        BasicCSSParser.CSSDeclaration(property: "color", value: "#5c6370"),
      ])
    )

    XCTAssertEqual(css[2].selector, ".hljs-quote")
    XCTAssertEqual(
      css[2].declaration,
      Set([
        BasicCSSParser.CSSDeclaration(property: "font-style", value: "italic"),
        BasicCSSParser.CSSDeclaration(property: "color", value: "#5c6370"),
      ])
    )
  }

  func testCSSParserPretty() throws {
    let css = BasicCSSParser.parse(
      """
      pre code.hljs {
        display: block;
        overflow-x: auto;
        padding:1em
      }

      code.hljs {
        padding: 3px 5px
      }

      .hljs {
        color: #abb2bf;
        background: #282c34
      }

      .hljs-comment, .hljs-quote {
        color: #5c6370;
        font-style: italic
      }
      """
    )

    XCTAssert(css.count == 3)

    XCTAssertEqual(css[0].selector, ".hljs")
    XCTAssertEqual(
      css[0].declaration,
      Set([
        BasicCSSParser.CSSDeclaration(property: "display", value: "block"),
        BasicCSSParser.CSSDeclaration(property: "padding", value: "1em"),
        BasicCSSParser.CSSDeclaration(property: "background", value: "#282c34"),
        BasicCSSParser.CSSDeclaration(property: "overflow-x", value: "auto"),
        BasicCSSParser.CSSDeclaration(property: "padding", value: "3px 5px"),
        BasicCSSParser.CSSDeclaration(property: "color", value: "#abb2bf"),
      ])
    )

    XCTAssertEqual(css[1].selector, ".hljs-comment")
    XCTAssertEqual(
      css[1].declaration,
      Set([
        BasicCSSParser.CSSDeclaration(property: "font-style", value: "italic"),
        BasicCSSParser.CSSDeclaration(property: "color", value: "#5c6370"),
      ])
    )

    XCTAssertEqual(css[2].selector, ".hljs-quote")
    XCTAssertEqual(
      css[2].declaration,
      Set([
        BasicCSSParser.CSSDeclaration(property: "font-style", value: "italic"),
        BasicCSSParser.CSSDeclaration(property: "color", value: "#5c6370"),
      ])
    )
  }
}
