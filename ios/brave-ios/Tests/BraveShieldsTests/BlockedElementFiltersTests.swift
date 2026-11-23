// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import XCTest

class BlockedElementFiltersTests: XCTestCase {
  private let preservedFilters = [
    #"host0.com##main [role="reg"] > [role="row"]:has(span:has-text(/^Prom/))"#,
    #"host0.com##button:matches-attr(class="/[\w]{7}/")"#,
    "host0.com##body > div[class]:matches-css(position: absolute)",
    "host0.com##body > div[class]:matches-css-before(position: absolute)",
    "host0.com##body > div[class]:matches-css-after(position: absolute)",
    "host0.com###target-1 > .target-2:matches-media((min-width: 800px))",
    "host0.com##:matches-path(/shop) p",
    "host0.com##div:matches-prop(imanad)",
    #"host0.com##^script:has-text(/[\w\W]{35000}/)"#,
    #"host0.com##main [role="reg"] > [role="row"]:has(span:not(:has-text(/^Promo/)))"#,
    #"host0.com##:matches-path(/^/home/) [data-testid="primaryColumn"]:others()"#,
    "host0.com###pcf #a19 b:upward(2)",
    "host0.com##.j-mini-player[class]:watch-attr(class):remove-attr(class)",
    #"host0.com##:xpath(//div[@id="pag"]//div[starts-with(@id,"hyperfeed_story_id_")]"#,
    "host0.com##+js(nobab)",
  ]

  private func rules(host0: String, host1: String) -> String {
    let preserved = preservedFilters.enumerated().map { index, line in
      index == 2 ? "    \(line)" : index == 6 ? " \(line)" : line
    }
    return
      (preserved + [
        "\(host0)##body > div.logged-in.env-production.page-responsive",
        "\(host1)###post-864297 > div.text > img:nth-child(9)",
      ]).joined(separator: "\n") + "\n"
  }

  private func checkCase(host0: String, host1: String, resetFor host: String) {
    let result = BlockedElementFilters.removingFilters(
      for: host,
      from: rules(host0: host0, host1: host1)
    )
    guard let result else {
      XCTFail("Expected filters to be returned")
      return
    }
    for filter in preservedFilters {
      XCTAssertTrue(result.contains("\(filter)\n"), "Missing preserved filter: \(filter)")
    }
    XCTAssertEqual(
      !result.contains("\(host0)##body > div.logged-in.env-production.page-responsive\n"),
      host0 == host
    )
    XCTAssertEqual(
      !result.contains("\(host1)###post-864297 > div.text > img:nth-child(9)\n"),
      host1 == host
    )
  }

  func testRemovingFiltersIgnoresScriptletsAndProcedural() {
    XCTAssertNil(BlockedElementFilters.removingFilters(for: "", from: ""))
    XCTAssertNil(
      BlockedElementFilters.removingFilters(
        for: "",
        from: "###post-864297 > div.text > img:nth-child(9)\n"
      )
    )
    checkCase(host0: "host0.com", host1: "host0.com", resetFor: "host0.com")
    checkCase(host0: "host1.com", host1: "host0.com", resetFor: "host0.com")
    checkCase(host0: "host1.com", host1: "host1.com", resetFor: "host1.com")
  }

  func testHasFilters() {
    XCTAssertFalse(BlockedElementFilters.hasFilters(for: "", in: "a.com##div"))
    XCTAssertFalse(BlockedElementFilters.hasFilters(for: "a.com", in: ""))
    XCTAssertFalse(
      BlockedElementFilters.hasFilters(
        for: "host0.com",
        in: preservedFilters.joined(separator: "\n")
      )
    )
    XCTAssertTrue(
      BlockedElementFilters.hasFilters(
        for: "host0.com",
        in: rules(host0: "host0.com", host1: "x.com")
      )
    )
    XCTAssertTrue(BlockedElementFilters.hasFilters(for: "a.com", in: "  a.com##.ad  \n"))
    XCTAssertFalse(BlockedElementFilters.hasFilters(for: "a.com", in: "b.com##.ad\n"))
  }
}
