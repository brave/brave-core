// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import JavaScriptCore

public struct BookmarkValidation {
  public static func validateBookmark(title: String?, url: String?) -> Bool {
    // Only title field is implemented
    if url == nil {
      return BookmarkValidation.validateTitle(title)
    }

    guard let url = url else { return false }
    return BookmarkValidation.validateTitle(title) && BookmarkValidation.validateUrl(url)
  }

  public static func validateBookmarklet(title: String?, url: String?) -> Bool {
    guard let url = url else { return validateTitle(title) }
    guard let bookmarklet = URL.bookmarkletURL(from: url) else { return false }
    guard let javascriptCode = bookmarklet.bookmarkletCodeComponent else {
      return false
    }

    // A bookmarklet is considered valid if it's code is valid JS.
    // Bookmarklets MIGHT invoke some security flaws allowing the user to run arbitrary
    // JS in the browser.
    // The JS is only ran within the webpage's context and not ran within the application context.
    if let context = JSContext() {
      context.evaluateScript(javascriptCode)
      if context.exception != nil {
        if context.exception.description.contains("ReferenceError") {
          return validateTitle(title)
        }
        return false
      }
      return validateTitle(title)
    }
    return false
  }

  private static func validateTitle(_ title: String?) -> Bool {
    guard let title = title else { return false }
    return !title.isEmpty
  }

  private static func validateUrl(_ urlString: String) -> Bool {
    return URL(string: urlString)?.schemeIsValid == true
  }
}
