// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Shared
import SnapKit
import Storage
import UIKit
import WebKit

private struct BackForwardViewUX {
  static let rowHeight: CGFloat = 50
  static let backgroundColor: UIColor = .braveBackground
}

class BackForwardListViewController: UIViewController, UIGestureRecognizerDelegate,
  ToolbarUrlActionsProtocol
{

  weak var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?

  private let cellIdentifier = "BackForwardListViewController"
  private var profile: Profile
  private lazy var sites = [String: Site]()
  private var dismissing = false
  private var currentRow = 0
  private var verticalConstraints: [Constraint] = []

  lazy var tableView: UITableView = {
    let tableView = UITableView()
    tableView.separatorStyle = .none
    tableView.dataSource = self
    tableView.delegate = self
    tableView.alwaysBounceVertical = false
    tableView.register(
      BackForwardTableViewCell.self,
      forCellReuseIdentifier: self.cellIdentifier
    )
    tableView.backgroundColor = BackForwardViewUX.backgroundColor
    return tableView
  }()

  private lazy var shadow: UIView = {
    let shadow = UIView()
    shadow.backgroundColor = UIColor(white: 0, alpha: 0.2)
    return shadow
  }()

  var tabManager: TabManager?
  weak var bvc: BrowserViewController?
  private var currentItem: CWVBackForwardListItem?
  private var backForwardListData = [CWVBackForwardListItem]()

  var tableHeight: CGFloat {
    assert(
      Thread.isMainThread,
      "tableHeight interacts with UIKit components - cannot call from background thread."
    )
    return min(
      BackForwardViewUX.rowHeight * CGFloat(backForwardListData.count),
      self.view.frame.height / 2
    )
  }

  var backForwardTransitionDelegate: UIViewControllerTransitioningDelegate? {
    didSet {
      self.transitioningDelegate = backForwardTransitionDelegate
    }
  }

  private var snappedToBottom = true

  private var isPrivateBrowsing: Bool {
    return tabManager?.privateBrowsingManager.isPrivateBrowsing ?? false
  }

  init(profile: Profile, backForwardList: CWVBackForwardList) {
    self.profile = profile
    super.init(nibName: nil, bundle: nil)

    loadSites(backForwardList)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
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

  func homeAndNormalPagesOnly(_ bfList: CWVBackForwardList) {
    let items =
      bfList.forwardList.reversed() + [bfList.currentItem].compactMap({ $0 })
      + bfList.backList.reversed()

    // error url's are OK as they are used to populate history on session restore.
    backForwardListData = items.filter {
      guard let internalUrl = InternalURL($0.url) else { return true }
      if internalUrl.isAboutHomeURL {
        return true
      }
      return true
    }
  }

  func loadSites(_ bfList: CWVBackForwardList) {
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

  override func willTransition(
    to newCollection: UITraitCollection,
    with coordinator: UIViewControllerTransitionCoordinator
  ) {
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

  override func viewWillTransition(
    to size: CGSize,
    with coordinator: UIViewControllerTransitionCoordinator
  ) {
    super.viewWillTransition(to: size, with: coordinator)
    let correctHeight = {
      self.tableView.snp.updateConstraints { make in
        make.height.equalTo(
          min(
            BackForwardViewUX.rowHeight * CGFloat(self.backForwardListData.count),
            size.height / 2
          )
        )
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
        verticalConstraints += [
          make.bottom.equalTo(self.view).offset(-bvc.footer.frame.height).constraint
        ]
      } else {
        verticalConstraints += [
          make.top.equalTo(self.view).offset(bvc.header.frame.height).constraint
        ]
      }
    }
    shadow.snp.makeConstraints { make in
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

  func gestureRecognizer(
    _ gestureRecognizer: UIGestureRecognizer,
    shouldReceive touch: UITouch
  ) -> Bool {
    if touch.view?.isDescendant(of: tableView) ?? true {
      return false
    }
    return true
  }
}

extension BackForwardListViewController: UITableViewDataSource {
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return backForwardListData.count
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {

    let cell =
      self.tableView.dequeueReusableCell(
        withIdentifier: cellIdentifier,
        for: indexPath
      ) as! BackForwardTableViewCell
    let item = backForwardListData[indexPath.item]
    let urlString = { () -> String in
      guard let url = InternalURL(item.url), let extracted = url.extractedUrlParam else {
        return item.url.absoluteString
      }
      return extracted.absoluteString
    }()

    cell.isPrivateBrowsing = isPrivateBrowsing
    cell.isCurrentTab = backForwardListData[indexPath.item] == self.currentItem
    cell.connectingBackwards = indexPath.item != backForwardListData.count - 1
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
}

extension BackForwardListViewController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tabManager?.selectedTab?.goToBackForwardListItem(backForwardListData[indexPath.item])
    dismiss(animated: true, completion: nil)
  }

  func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    return BackForwardViewUX.rowHeight
  }

  func tableView(
    _ tableView: UITableView,
    contextMenuConfigurationForRowAt indexPath: IndexPath,
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    let listItemURL = backForwardListData[indexPath.item].url

    return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) {
      [unowned self] _ in
      let openInNewTabAction = UIAction(
        title: Strings.openNewTabButtonTitle,
        image: UIImage(systemName: "plus.square.on.square"),
        handler: UIAction.deferredActionHandler { _ in
          self.toolbarUrlActionsDelegate?.openInNewTab(
            listItemURL,
            isPrivate: self.isPrivateBrowsing
          )
          self.presentingViewController?.dismiss(animated: true)
        }
      )

      let copyAction = UIAction(
        title: Strings.copyLinkActionTitle,
        image: UIImage(systemName: "doc.on.doc"),
        handler: UIAction.deferredActionHandler { _ in
          self.toolbarUrlActionsDelegate?.copy(listItemURL)
        }
      )
      let shareAction = UIAction(
        title: Strings.shareLinkActionTitle,
        image: UIImage(systemName: "square.and.arrow.up"),
        handler: UIAction.deferredActionHandler { _ in
          self.toolbarUrlActionsDelegate?.share(listItemURL)
        }
      )

      let newTabActionMenu: [UIAction] = [openInNewTabAction]
      let urlMenu = UIMenu(title: "", options: .displayInline, children: newTabActionMenu)
      let linkMenu = UIMenu(
        title: "",
        options: .displayInline,
        children: [copyAction, shareAction]
      )

      return UIMenu(
        title: listItemURL.absoluteString,
        identifier: nil,
        children: [urlMenu, linkMenu]
      )
    }
  }
}
