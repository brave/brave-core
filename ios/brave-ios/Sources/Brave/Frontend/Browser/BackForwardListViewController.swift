/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import WebKit
import Storage
import SnapKit

private struct BackForwardViewUX {
  static let rowHeight: CGFloat = 50
  static let backgroundColor: UIColor = .braveBackground
}

class BackForwardListViewController: UIViewController, UITableViewDataSource, UITableViewDelegate, UIGestureRecognizerDelegate {

  fileprivate let BackForwardListCellIdentifier = "BackForwardListViewController"
  fileprivate var profile: Profile
  fileprivate lazy var sites = [String: Site]()
  fileprivate var dismissing = false
  fileprivate var currentRow = 0
  fileprivate var verticalConstraints: [Constraint] = []

  lazy var tableView: UITableView = {
    let tableView = UITableView()
    tableView.separatorStyle = .none
    tableView.dataSource = self
    tableView.delegate = self
    tableView.alwaysBounceVertical = false
    tableView.register(BackForwardTableViewCell.self, forCellReuseIdentifier: self.BackForwardListCellIdentifier)
    tableView.backgroundColor = BackForwardViewUX.backgroundColor
    return tableView
  }()

  lazy var shadow: UIView = {
    let shadow = UIView()
    shadow.backgroundColor = UIColor(white: 0, alpha: 0.2)
    return shadow
  }()

  var tabManager: TabManager!
  weak var bvc: BrowserViewController?
  var currentItem: WKBackForwardListItem?
  var listData = [WKBackForwardListItem]()

  var tableHeight: CGFloat {
    get {
      assert(Thread.isMainThread, "tableHeight interacts with UIKit components - cannot call from background thread.")
      return min(BackForwardViewUX.rowHeight * CGFloat(listData.count), self.view.frame.height / 2)
    }
  }

  var backForwardTransitionDelegate: UIViewControllerTransitioningDelegate? {
    didSet {
      self.transitioningDelegate = backForwardTransitionDelegate
    }
  }

  var snappedToBottom: Bool = true

  init(profile: Profile, backForwardList: WKBackForwardList) {
    self.profile = profile
    super.init(nibName: nil, bundle: nil)

    loadSites(backForwardList)
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    view.addSubview(shadow)
    view.addSubview(tableView)
    snappedToBottom = bvc?.toolbar != nil
    tableView.snp.makeConstraints { make in
      make.height.equalTo(0)
      make.left.right.equalTo(self.view)
    }
    shadow.snp.makeConstraints { make in
      make.left.right.equalTo(self.view)
    }
    remakeVerticalConstraints()
    view.layoutIfNeeded()
    scrollTableViewToIndex(currentRow)
    setupDismissTap()
  }

  func homeAndNormalPagesOnly(_ bfList: WKBackForwardList) {
    let items = bfList.forwardList.reversed() + [bfList.currentItem].compactMap({ $0 }) + bfList.backList.reversed()

    // error url's are OK as they are used to populate history on session restore.
    listData = items.filter {
      guard let internalUrl = InternalURL($0.url) else { return true }
      if internalUrl.isAboutHomeURL {
        return true
      }
      if let url = internalUrl.originalURLFromErrorPage, InternalURL.isValid(url: url) {
        return false
      }
      return true
    }
  }

  func loadSites(_ bfList: WKBackForwardList) {
    currentItem = bfList.currentItem

    homeAndNormalPagesOnly(bfList)
  }

  func scrollTableViewToIndex(_ index: Int) {
    guard index > 1 else {
      return
    }
    let moveToIndexPath = IndexPath(row: index - 2, section: 0)
    tableView.reloadRows(at: [moveToIndexPath], with: .none)
    tableView.scrollToRow(at: moveToIndexPath, at: .middle, animated: false)
  }

  override func willTransition(to newCollection: UITraitCollection, with coordinator: UIViewControllerTransitionCoordinator) {
    super.willTransition(to: newCollection, with: coordinator)
    guard let bvc = self.bvc else {
      return
    }
    if bvc.shouldShowFooterForTraitCollection(newCollection) != snappedToBottom {
      tableView.snp.updateConstraints { make in
        if snappedToBottom {
          make.bottom.equalTo(self.view).offset(0)
        } else {
          make.top.equalTo(self.view).offset(0)
        }
        make.height.equalTo(0)
      }
      snappedToBottom = !snappedToBottom
    }
  }

  override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)
    let correctHeight = {
      self.tableView.snp.updateConstraints { make in
        make.height.equalTo(min(BackForwardViewUX.rowHeight * CGFloat(self.listData.count), size.height / 2))
      }
    }
    coordinator.animate(alongsideTransition: nil) { _ in
      self.remakeVerticalConstraints()
      correctHeight()
    }
  }

  func remakeVerticalConstraints() {
    guard let bvc = self.bvc else {
      return
    }
    for constraint in self.verticalConstraints {
      constraint.deactivate()
    }
    self.verticalConstraints = []
    tableView.snp.makeConstraints { make in
      if snappedToBottom {
        verticalConstraints += [make.bottom.equalTo(self.view).offset(-bvc.footer.frame.height).constraint]
      } else {
        verticalConstraints += [make.top.equalTo(self.view).offset(bvc.header.frame.height).constraint]
      }
    }
    shadow.snp.makeConstraints() { make in
      if snappedToBottom {
        verticalConstraints += [
          make.bottom.equalTo(tableView.snp.top).constraint,
          make.top.equalTo(self.view).constraint,
        ]

      } else {
        verticalConstraints += [
          make.top.equalTo(tableView.snp.bottom).constraint,
          make.bottom.equalTo(self.view).constraint,
        ]
      }
    }
  }

  func setupDismissTap() {
    let tap = UITapGestureRecognizer(target: self, action: #selector(handleTap))
    tap.cancelsTouchesInView = false
    tap.delegate = self
    view.addGestureRecognizer(tap)
  }

  @objc func handleTap() {
    dismiss(animated: true, completion: nil)
  }

  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldReceive touch: UITouch) -> Bool {
    if touch.view?.isDescendant(of: tableView) ?? true {
      return false
    }
    return true
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: - Table view
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return listData.count
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    // swiftlint:disable:next force_cast
    let cell = self.tableView.dequeueReusableCell(withIdentifier: BackForwardListCellIdentifier, for: indexPath) as! BackForwardTableViewCell
    let item = listData[indexPath.item]
    let urlString = { () -> String in
      guard let url = InternalURL(item.url), let extracted = url.extractedUrlParam else {
        return item.url.absoluteString
      }
      return extracted.absoluteString
    }()

    cell.isPrivateBrowsing = tabManager.privateBrowsingManager.isPrivateBrowsing
    cell.isCurrentTab = listData[indexPath.item] == self.currentItem
    cell.connectingBackwards = indexPath.item != listData.count - 1
    cell.connectingForwards = indexPath.item != 0

    let isAboutHomeURL = InternalURL(item.url)?.isAboutHomeURL ?? false
    guard !isAboutHomeURL else {
      cell.site = Site(url: item.url.absoluteString, title: Strings.home)
      return cell
    }

    cell.site = sites[urlString] ?? Site(url: urlString, title: item.title ?? "")
    cell.setNeedsDisplay()

    return cell
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tabManager.selectedTab?.goToBackForwardListItem(listData[indexPath.item])
    dismiss(animated: true, completion: nil)
  }

  func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    return BackForwardViewUX.rowHeight
  }
}
