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

  @concurrent static func generateReaderContent(
    _ readabilityResult: ReadabilityResult,
    initialStyle: ReaderModeStyle,
    titleNonce: String
  ) async -> String? {
    guard let stylePath = Bundle.module.url(forResource: "Reader", withExtension: "css"),
      let css = await AsyncFileManager.default.utf8Contents(at: stylePath),
      let tmplPath = Bundle.module.url(forResource: "Reader", withExtension: "html"),
      let tmpl = await AsyncFileManager.default.utf8Contents(at: tmplPath)
    else { return nil }
    let languageCode = Locale.LanguageCode(readabilityResult.documentLanguage)

    let substitutions = [
      "%READER-TITLE-NONCE%": titleNonce,
      "%READER-CSS%": css,
      "%READER-STYLE%": initialStyle.encode(),
      "%READER-DOMAIN%": simplifyDomain(readabilityResult.domain),
      "%READER-URL%": readabilityResult.url,
      "%READER-PAGE-LANGUAGE%": languageCode.isISOLanguage ? languageCode.identifier : "",
      "%READER-TITLE%": readabilityResult.title.javaScriptEscapedString?.unquotedIfNecessary
        ?? readabilityResult.title.htmlEntityEncodedString,
      "%READER-CREDITS%": readabilityResult.credits.javaScriptEscapedString?.unquotedIfNecessary
        ?? readabilityResult.credits.htmlEntityEncodedString,
      "%READER-DIRECTION%": readabilityResult.direction.htmlEntityEncodedString,
      "%READER-MESSAGE%": "",
      "%READER-ORIGINAL-PAGE-META-TAGS%": readabilityResult.cspMetaTags
        .flatMap { ReaderModeHandler.adoptedImageSourcePolicyContents(from: $0) }
        .map {
          "<meta http-equiv=\"Content-Security-Policy\" content=\"\($0.htmlEntityEncodedString)\">"
        }
        .joined(separator: "\n"),
      "%READER-CONTENT%": readabilityResult.content,
    ]

    return tmpl.replacingOccurances(substitutions) ?? tmpl
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
