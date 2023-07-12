/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

struct ReaderModeUtils {

  static let domainPrefixesToSimplify = ["www.", "mobile.", "m.", "blog."]

  static func simplifyDomain(_ domain: String) -> String {
    return domainPrefixesToSimplify.first { domain.hasPrefix($0) }.map {
      String($0.suffix(from: $0.index($0.startIndex, offsetBy: $0.count)))
    } ?? domain
  }

  static func generateReaderContent(_ readabilityResult: ReadabilityResult, initialStyle: ReaderModeStyle, titleNonce: String) -> String? {
    guard let stylePath = Bundle.module.path(forResource: "Reader", ofType: "css"),
      let css = try? String(contentsOfFile: stylePath, encoding: .utf8),
      let tmplPath = Bundle.module.path(forResource: "Reader", ofType: "html"),
      let tmpl = try? String(contentsOfFile: tmplPath, encoding: .utf8)
    else { return nil }

    return tmpl.replacingOccurrences(of: "%READER-TITLE-NONCE%", with: titleNonce)  // This MUST be the first line/replacement!

      .replacingOccurrences(of: "%READER-CSS%", with: css)
      .replacingOccurrences(of: "%READER-STYLE%", with: initialStyle.encode())
      .replacingOccurrences(of: "%READER-DOMAIN%", with: simplifyDomain(readabilityResult.domain))
      .replacingOccurrences(of: "%READER-URL%", with: readabilityResult.url)
      .replacingOccurrences(of: "%READER-TITLE%", with: readabilityResult.title.javaScriptEscapedString?.unquotedIfNecessary ?? readabilityResult.title.htmlEntityEncodedString)
      .replacingOccurrences(of: "%READER-CREDITS%", with: readabilityResult.credits.javaScriptEscapedString?.unquotedIfNecessary ?? readabilityResult.credits.htmlEntityEncodedString)
      .replacingOccurrences(of: "%READER-CONTENT%", with: readabilityResult.content)
      .replacingOccurrences(of: "%READER-DIRECTION%", with: readabilityResult.direction.javaScriptEscapedString?.unquotedIfNecessary ?? readabilityResult.direction.htmlEntityEncodedString)
      .replacingOccurrences(of: "%READER-MESSAGE%", with: "")
  }
}

extension String {
  var unquotedIfNecessary: String {
    var str = self
    if str.first == "\"" || str.first == "'" {
      str = String(str.dropFirst())
    }

    if str.last == "\"" || str.last == "'" {
      str = String(str.dropLast())
    }
    return str
  }
}
