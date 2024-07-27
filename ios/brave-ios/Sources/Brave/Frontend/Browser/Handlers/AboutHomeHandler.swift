// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Foundation
import UIKit

public class AboutHomeHandler: InternalSchemeResponse {
  public static let path = "about/home"

  // Return a blank page, the webview delegate will look at the current URL and load the home panel based on that
  public func response(forRequest request: URLRequest) async -> (URLResponse, Data)? {
    guard let url = request.url else { return nil }
    let response = InternalSchemeHandler.response(forUrl: url)
    let bg = UIColor.braveBackground.toHexString()
    // Blank page with a color matching the background of the panels which is displayed for a split-second until the panel shows.
    let html = """
          <!DOCTYPE html>
          <html>
            <body style='background-color:\(bg)'></body>
          </html>
      """
    let data = Data(html.utf8)
    return (response, data)
  }

  public init() {}
}

public class AboutLicenseHandler: InternalSchemeResponse {
  public static let path = "about/license"

  public func response(forRequest request: URLRequest) async -> (URLResponse, Data)? {
    guard let url = request.url else { return nil }
    let response = InternalSchemeHandler.response(forUrl: url)
    guard let asset = Bundle.module.url(forResource: "Licenses", withExtension: "html"),
      let html = await AsyncFileManager.default.utf8Contents(at: asset)
    else {
      return nil
    }
    let data = Data(html.utf8)
    return (response, data)
  }

  public init() {}
}
