// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

#if DEBUG
struct BraveSearchDebugMenuFixture {
  static let loggingSample = BraveSearchLogEntry(isEnabled: true, logs: [sample, sample])

  static let sample: BraveSearchLogEntry.FallbackLogEntry =
    .init(
      date: Date(),
      url: URL(string: "https://example.com/search?q=sample+query")!,
      query: "Sample+query",
      cookies: cookies,
      canAnswerTime: "0.25",
      backupQuery:
        """
        {"found":true,"found_fresh":false, \
        "language":null,"country":"us", \
        "safesearch":"moderate"}
        """,
      fallbackTime: "0.67",
      fallbackData: sampleHTML.data(using: .utf8))

  private static let cookies: [HTTPCookie] =
    [
      HTTPCookie(properties: [
        .domain: "example.com",
        .name: "Cookie 1",
        .value: "Value 1",
      ]),
      HTTPCookie(properties: [
        .domain: "example.com",
        .name: "Cookie 1",
        .value: 123,
      ]),
    ]
    .compactMap { $0 }

  private static let sampleHTML =
    """
    <!DOCTYPE html>
    <html>
    <body>

    <h1>My First Heading</h1>

    <p>My first paragraph.</p>

    </body>
    </html>
    """
}
#endif
