/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveUI
import Foundation
import UIKit
import BraveShared

public class AboutHomeHandler: InternalSchemeResponse {
  public static let path = "about/home"

  // Return a blank page, the webview delegate will look at the current URL and load the home panel based on that
  public func response(forRequest request: URLRequest) -> (URLResponse, Data)? {
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
    guard let data = html.data(using: .utf8) else {
      return nil
    }
    return (response, data)
  }
  
  public init() { }
}

public class AboutLicenseHandler: InternalSchemeResponse {
  public static let path = "about/license"

  public func response(forRequest request: URLRequest) -> (URLResponse, Data)? {
    guard let url = request.url else { return nil }
    let response = InternalSchemeHandler.response(forUrl: url)
    guard let path = Bundle.module.path(forResource: "Licenses", ofType: "html"), let html = try? String(contentsOfFile: path, encoding: .utf8),
      let data = html.data(using: .utf8)
    else {
      return nil
    }
    return (response, data)
  }
  
  public init() { }
}
