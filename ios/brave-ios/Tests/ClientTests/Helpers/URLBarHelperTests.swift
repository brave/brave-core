// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import XCTest

@testable import Brave

class URLBarHelperTests: XCTestCase {

  private var validQueryList: [String] = []
  private var suspiciousQueryList: [String] = []

  override func setUp() {
    super.setUp()
    generateTestQueryList()
  }

  override func tearDown() {
    super.tearDown()
    clearTestQueryList()
  }

  func testSafeQueryList() {
    testQuery(list: validQueryList, isSuspicious: false)
  }

  func testSuspiciousQueryList() {
    testQuery(list: suspiciousQueryList, isSuspicious: true)
  }

  private func testQuery(list: [String], isSuspicious: Bool) {
    for item in list {
      XCTAssert(URLBarHelper.isSuspiciousQuery(query: item) == isSuspicious)
    }
  }

  private func generateTestQueryList() {
    // Setup the valid passing queries
    validQueryList = [
      "amazon",
      "bank of america",
      "bild",
      "craigslist",
      "ebay",
      "ebay kleinanzeigen",
      "facebook",
      "finance",
      "gmail",
      "gmx",
      "gmx.de",
      "google docs",
      "google drive",
      "google maps",
      "google translate",
      "maps",
      "netflix",
      "speed test",
      "paypal",
      "postbank",
      "t-online",
      "translate",
      "weather",
      "yahoo mail",
      "youtube",
      "Fu?ball",
      "ma? bier",
      "ma?krug",
      "c# book",
      "c# for dummies",
      "d#nisches bettenlager",
      "kleinanzeigen#",
      "to.be.true vs to.equal(true)",
      "chrome.runtime.id",
      "Yandex.Kit",
      "Node.Js",
      "org.apache.log4j.Logger upgrade",
      "http://a",
      "test query",
      "http://a asdfasdfasdfasdf",
      "http://sinonjs.test/releases/v4.0.0/spies/",
      "one two three four five six seven",
      "a 1234341 b 1234561",
      "seti@home",
      "a seti@home b",
    ]

    // Setup the invalid suspicious queries
    suspiciousQueryList = [
      "Dr. Strangelove or: How I Learned to Stop Worrying and Love the Bomb",
      "Intel NUC Kit Barebone NUC7I5BNH Intel Core i5-7260U, Intel Iris Plus Grafik 640, 2x DDR",
      "Install error - 0x80248007",
      "segfault at 0 ip 00007fb3cdf2afad sp 00007fb3cc2d7ae0 error 6 in libxul.so",
      "CPU0: Core temperature above threshold, cpu clock throttled (total events = 340569",
      "test query  \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t \t",
      "net.ipv4.tcp_tw_reuse",
      "https://github.test/cliqz/navigation-extension/pull/6200/commits/74f65ce53e5e163c7ec2770ba51470eaa8d24ca4",
      "https://eu-central-1.console.aws.amazon.test/console/home?region=eu-central-1#",
      "http://www.spiegel.test/politik/ausland/carles-puigdemont-gericht-erklaert-auslieferung-wegen-veruntreuung-fuer-zulaessig-a-1218049.html",
      "https://www.amazon.de/Samsung-MLT-D101S-Toner-Cartridge-Black/dp/B006WARUYQ",
      "http://198.51.100.1/admin/foo/bar/?o=123456",
      "Inplacement - neue Mitarbeiter erfolgreich einarbeiten und integrieren : wie sie das Potenzial neuer Mitarbeiter erschließen und für ihr Unternehmen nutzbar machen; eine Arbeitshilfe für Führungskräfte / von Doris Brenner; Frank Brenner",
      "Mehrere Mütter kommentieren und bewerten eine Arbeit im weißen Raum, im Atelier des Künstlers Jonathan Meese, das zur mehrdimensionalen Leinwand wird. In der ersten Virtual-Reality-Produktion des Künstlers verschwimmen Wirklichkeit und Künstlermythos.",
      "An open label, randomized, two arm phase III study of nivolumab incombination with ipilimumab versus extreme study regimen as first linetherapy in recurrent or metastatic squamous cell carcinoma of the headand neck",
      "Afflerbach, Patrick, Gergor Kastner, Felix Krause, and Maximilian Röglinger. 2014. The Business Value of Pro-cess Flexibility - An Optimization Model and its Application in the Service Sector.",
      "Those Magnificent Men in Their Flying Machines or How I Flew from London to Paris in 25 hours 11 minutes",
      "Critical dependency: require function is used in a way in which dependencies cannot be statically extracted",
      "Error:Android Source Generator: Error: Can't find bundle for base name messages.AndroidJpsBundle, locale de_DEjava.util.MissingResourceException: Can't find bundle for base name messages.AndroidJpsBundle, locale de_DEat java.ut",
      "one two three four five six seven eight",
      "a 1234341 b 12345611",
      "a 12343411 b 1234561",
      "seti@home.com",
      "a seti@home.com b",
    ]
  }

  private func clearTestQueryList() {
    validQueryList.removeAll()
    suspiciousQueryList.removeAll()
  }
}
