// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import XCTest

@testable import Growth

class LanguageMetricsTests: XCTestCase {
  func testInvalidLanguageCodes() {
    let expectedValue = Int.max - 1
    XCTAssertEqual(LanguageMetrics.answerForLangaugeCode(nil), expectedValue)
    XCTAssertEqual(LanguageMetrics.answerForLangaugeCode("notalangugage"), expectedValue)
  }

  func testValidLanguageCodes() {
    for (index, languageCode) in LanguageMetrics.acceptedLanguages.enumerated() {
      XCTAssertEqual(LanguageMetrics.answerForLangaugeCode(languageCode), index)
    }
  }

  func testLanguageSynonyms() {
    let (deprecated, active) = ("in", "id")
    let expectedAnswer = LanguageMetrics.answerForLangaugeCode(active)
    XCTAssertEqual(LanguageMetrics.answerForLangaugeCode(deprecated), expectedAnswer)
  }
}
