// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation

struct ReaderModeUtils {

  static let domainPrefixesToSimplify = ["www.", "mobile.", "m.", "blog."]

  static func simplifyDomain(_ domain: String) -> String {
    return domainPrefixesToSimplify.first { domain.hasPrefix($0) }.map {
      String($0.suffix(from: $0.index($0.startIndex, offsetBy: $0.count)))
    } ?? domain
  }

  static func generateReaderContent(
    _ readabilityResult: ReadabilityResult,
    initialStyle: ReaderModeStyle,
    titleNonce: String
  ) async -> String? {
    guard let stylePath = Bundle.module.url(forResource: "Reader", withExtension: "css"),
      let css = await AsyncFileManager.default.utf8Contents(at: stylePath),
      let tmplPath = Bundle.module.url(forResource: "Reader", withExtension: "html"),
      let tmpl = await AsyncFileManager.default.utf8Contents(at: tmplPath)
    else { return nil }

    // This MUST be the first line/replacement!
    return tmpl.replacingOccurrences(of: "%READER-TITLE-NONCE%", with: titleNonce)

      .replacingOccurrences(of: "%READER-CSS%", with: css)
      .replacingOccurrences(of: "%READER-STYLE%", with: initialStyle.encode())
      .replacingOccurrences(of: "%READER-DOMAIN%", with: simplifyDomain(readabilityResult.domain))
      .replacingOccurrences(of: "%READER-URL%", with: readabilityResult.url)
      .replacingOccurrences(
        of: "%READER-TITLE%",
        with: readabilityResult.title.javaScriptEscapedString?.unquotedIfNecessary
          ?? readabilityResult.title.htmlEntityEncodedString
      )
      .replacingOccurrences(
        of: "%READER-CREDITS%",
        with: readabilityResult.credits.javaScriptEscapedString?.unquotedIfNecessary
          ?? readabilityResult.credits.htmlEntityEncodedString
      )
      .replacingOccurrences(
        of: "%READER-DIRECTION%",
        with: readabilityResult.direction.javaScriptEscapedString?.unquotedIfNecessary
          ?? readabilityResult.direction.htmlEntityEncodedString
      )
      .replacingOccurrences(of: "%READER-MESSAGE%", with: "")

      // PAGE UNESCAPED REPLACEMENTS MUST BE DONE AFTER THIS LINE
      .replacingOccurrences(
        of: "%READER-ORIGINAL-PAGE-META-TAGS%",
        with: readabilityResult.cspMetaTags.joined(separator: "  \n")
      )

      // DO NOT DO ANY REPLACEMENTS AFTER THIS LINE
      .replacingOccurrences(of: "%READER-CONTENT%", with: readabilityResult.content)
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
