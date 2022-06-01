/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit

private struct PrintedPageUX {
  static let pageInsets = CGFloat(36.0)
  static let pageTextFont = DynamicFontHelper.defaultHelper.DefaultSmallFont
  static let pageMarginScale = CGFloat(0.5)
}

class TabPrintPageRenderer: UIPrintPageRenderer {
  private weak var tab: Tab?
  private let displayTitle: String

  let textAttributes = [NSAttributedString.Key.font: PrintedPageUX.pageTextFont]
  let dateString: String

  required init(tab: Tab) {
    self.tab = tab
    let dateFormatter = DateFormatter()
    dateFormatter.dateStyle = .short
    dateFormatter.timeStyle = .short

    self.displayTitle = tab.displayTitle
    self.dateString = dateFormatter.string(from: Date())

    super.init()

    self.footerHeight = PrintedPageUX.pageMarginScale * PrintedPageUX.pageInsets
    self.headerHeight = PrintedPageUX.pageMarginScale * PrintedPageUX.pageInsets

    if let tab = self.tab {
      let formatter = tab.webView!.viewPrintFormatter()
      formatter.perPageContentInsets = UIEdgeInsets(top: PrintedPageUX.pageInsets, left: PrintedPageUX.pageInsets, bottom: PrintedPageUX.pageInsets, right: PrintedPageUX.pageInsets)
      addPrintFormatter(formatter, startingAtPageAt: 0)
    }
  }

  override func drawFooterForPage(at pageIndex: Int, in headerRect: CGRect) {
    let headerInsets = UIEdgeInsets(top: headerRect.minY, left: PrintedPageUX.pageInsets, bottom: paperRect.maxY - headerRect.maxY, right: PrintedPageUX.pageInsets)
    let headerRect = paperRect.inset(by: headerInsets)

    // url on left
    self.drawTextAtPoint(tab!.url?.displayURL?.absoluteString ?? "", rect: headerRect, onLeft: true)

    // page number on right
    let pageNumberString = "\(pageIndex + 1)"
    self.drawTextAtPoint(pageNumberString, rect: headerRect, onLeft: false)
  }

  override func drawHeaderForPage(at pageIndex: Int, in headerRect: CGRect) {
    let headerInsets = UIEdgeInsets(top: headerRect.minY, left: PrintedPageUX.pageInsets, bottom: paperRect.maxY - headerRect.maxY, right: PrintedPageUX.pageInsets)
    let headerRect = paperRect.inset(by: headerInsets)

    // page title on left
    self.drawTextAtPoint(displayTitle, rect: headerRect, onLeft: true)

    // date on right
    self.drawTextAtPoint(dateString, rect: headerRect, onLeft: false)
  }

  func drawTextAtPoint(_ text: String, rect: CGRect, onLeft: Bool) {
    let size = text.size(withAttributes: textAttributes)
    let x, y: CGFloat
    if onLeft {
      x = rect.minX
      y = rect.midY - size.height / 2
    } else {
      x = rect.maxX - size.width
      y = rect.midY - size.height / 2
    }
    text.draw(at: CGPoint(x: x, y: y), withAttributes: textAttributes)
  }

}
