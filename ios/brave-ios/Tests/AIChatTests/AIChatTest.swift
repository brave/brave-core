// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
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

  func testReceiptParsing() throws {
    let receipt =
      "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"

    let data = try XCTUnwrap(Data(base64Encoded: receipt))
    let parsedReceipt = try XCTUnwrap(BraveStoreKitReceipt(data: data))

    XCTAssertEqual(parsedReceipt.bundleId, "com.brave.ios.browser")
    XCTAssertEqual(parsedReceipt.sha1Hash, "42D750D02D626DD0F8BBBBB55E9FC00DDD47AC26")
    XCTAssertNotNil(parsedReceipt.receiptCreationDate)
    XCTAssertEqual(
      parsedReceipt.receiptCreationDate!.timeIntervalSince1970,
      1712094975.0,
      accuracy: Float64.leastNonzeroMagnitude
    )
    XCTAssertEqual(parsedReceipt.inAppPurchaseReceipts.count, 5)
    XCTAssertEqual(parsedReceipt.inAppPurchaseReceipts[0].productId, "bravevpn.yearly")
    XCTAssertEqual(parsedReceipt.inAppPurchaseReceipts[1].productId, "braveleo.monthly")
    XCTAssertEqual(parsedReceipt.inAppPurchaseReceipts[2].productId, "braveleo.monthly")
    XCTAssertEqual(parsedReceipt.inAppPurchaseReceipts[3].productId, "bravevpn.monthly")
    XCTAssertEqual(parsedReceipt.inAppPurchaseReceipts[4].productId, "bravevpn.monthly")

    for purchase in parsedReceipt.inAppPurchaseReceipts {
      XCTAssertNotNil(purchase.originalPurchaseDate)
      XCTAssertNotNil(purchase.purchaseDate)
      XCTAssertNotNil(purchase.subscriptionExpirationDate)
    }
  }
}
