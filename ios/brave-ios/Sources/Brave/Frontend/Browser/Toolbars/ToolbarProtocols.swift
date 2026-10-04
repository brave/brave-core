// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import BraveStrings
import BraveUI
import DesignSystem
import Foundation
import UIKit
import Web

protocol ToolbarProtocol: AnyObject {

  var tabToolbarDelegate: ToolbarDelegate? { get set }
  var tabsButton: TabsButton { get }
  var backButton: ToolbarButton { get }
  var forwardButton: ToolbarButton { get }
  var shareButton: ToolbarButton { get }
  var addTabButton: ToolbarButton { get }
  var searchButton: ToolbarButton { get }
  var menuButton: ToolbarButton { get }
  var actionButtons: [UIButton] { get }
}

extension ToolbarProtocol {
  /// Assigns the menus displayed when long pressing the tabs, add tab & search buttons.
  ///
  /// The menu contents are resolved from `state` each time a menu is displayed. The add tab &
  /// search buttons share a menu since a toolbar never displays them at the same time.
  @MainActor
  func configureToolbarMenus(state: BrowserToolbarState) {
    tabsButton.menu = UIMenu(children: [
      UIDeferredMenuElement.uncached { [weak self, weak state] completion in
        guard let self, let state else {
          completion([])
          return
        }
        completion(self.tabsMenuElements(state: state))
      }
    ])
    let newTabMenu = UIMenu(children: [
      UIDeferredMenuElement.uncached { [weak self, weak state] completion in
        guard let self, let state else {
          completion([])
          return
        }
        completion([
          UIMenu(options: .displayInline, children: self.newTabMenuActions(state: state))
        ])
      }
    ])
    addTabButton.menu = newTabMenu
    searchButton.menu = newTabMenu
  }

  @MainActor
  private func tabsMenuElements(state: BrowserToolbarState) -> [UIMenuElement] {
    let closeTab = [
      UIAction(
        title: String(format: Strings.Hotkey.closeTabTitle),
        image: UIImage(braveSystemNamed: "leo.close"),
        attributes: .destructive,
        handler: UIAction.deferredActionHandler { [weak self] _ in
          guard let self else { return }
          self.tabToolbarDelegate?.tabToolbarDidSelectCloseTab(self)
        }
      )
    ]

    var closeTabs: [UIAction] = []
    if state.canShredSiteData {
      closeTabs.append(
        UIAction(
          title: Strings.Shields.shredSiteData,
          image: UIImage(braveSystemNamed: "leo.shred.data"),
          attributes: .destructive,
          handler: UIAction.deferredActionHandler { [weak self] _ in
            guard let self else { return }
            self.tabToolbarDelegate?.tabToolbarDidSelectShredSiteData(self)
          }
        )
      )
    }
    if state.tabCount > 1 {
      closeTabs.append(
        UIAction(
          title: Strings.closeAllOtherTabsTitle,
          image: UIImage(braveSystemNamed: "leo.close"),
          attributes: .destructive,
          handler: UIAction.deferredActionHandler { [weak self] _ in
            guard let self else { return }
            self.tabToolbarDelegate?.tabToolbarDidSelectCloseOtherTabs(self)
          }
        )
      )
      closeTabs.append(
        UIAction(
          title: String(format: Strings.closeAllTabsTitle, state.tabCount),
          image: UIImage(braveSystemNamed: "leo.close"),
          attributes: .destructive,
          handler: UIAction.deferredActionHandler { [weak self] _ in
            guard let self else { return }
            self.tabToolbarDelegate?.tabToolbarDidSelectCloseAllTabs(self)
          }
        )
      )
    }

    var recentlyClosed: [UIAction] = []
    if state.hasRecentlyClosedTabs {
      recentlyClosed.append(
        UIAction(
          title: Strings.RecentlyClosed.viewRecentlyClosedTab,
          image: UIImage(braveSystemNamed: "leo.browser.mobile-recent-tabs"),
          handler: UIAction.deferredActionHandler { [weak self] _ in
            guard let self else { return }
            self.tabToolbarDelegate?.tabToolbarDidSelectViewRecentlyClosedTabs(self)
          }
        )
      )
      recentlyClosed.append(
        UIAction(
          title: Strings.RecentlyClosed.recentlyClosedReOpenLastActionTitle,
          image: UIImage(braveSystemNamed: "leo.browser.mobile-tab-ntp"),
          handler: UIAction.deferredActionHandler { [weak self] _ in
            guard let self else { return }
            self.tabToolbarDelegate?.tabToolbarDidSelectReopenRecentlyClosedTab(self)
          }
        )
      )
    }

    var duplicateTab: [UIAction] = []
    if state.canDuplicateTab {
      duplicateTab.append(
        UIAction(
          title: Strings.duplicateActiveTab,
          image: UIImage(braveSystemNamed: "leo.browser.mobile-tabs"),
          handler: UIAction.deferredActionHandler { [weak self] _ in
            guard let self else { return }
            self.tabToolbarDelegate?.tabToolbarDidSelectDuplicateTab(self)
          }
        )
      )
    }

    var bookmarks: [UIAction] = []
    if state.canBookmarkTab {
      bookmarks.append(
        UIAction(
          title: Strings.addToMenuItem,
          image: UIImage(braveSystemNamed: "leo.browser.bookmark-add"),
          handler: UIAction.deferredActionHandler { [weak self] _ in
            guard let self else { return }
            self.tabToolbarDelegate?.tabToolbarDidSelectBookmarkTab(self)
          }
        )
      )
      // To bookmark all tabs there must be more than 1 bookmarkable tab
      let bookmarkableTabCount = state.bookmarkableTabCount
      if bookmarkableTabCount > 1 {
        bookmarks.append(
          UIAction(
            title: String.localizedStringWithFormat(
              Strings.bookmarkAllTabsTitle,
              bookmarkableTabCount
            ),
            image: UIImage(braveSystemNamed: "leo.browser.bookmark-add"),
            handler: UIAction.deferredActionHandler { [weak self] _ in
              guard let self else { return }
              self.tabToolbarDelegate?.tabToolbarDidSelectBookmarkAllTabs(self)
            }
          )
        )
      }
    }

    let sections = [closeTab, closeTabs, recentlyClosed, duplicateTab, bookmarks]
    return sections.filter { !$0.isEmpty }.map {
      UIMenu(options: .displayInline, children: $0)
    }
  }

  @MainActor
  private func newTabMenuActions(state: BrowserToolbarState) -> [UIAction] {
    let isPrivateTab = state.isPrivateTab
    var actions: [UIAction] = []
    if !isPrivateTab {
      actions.append(
        UIAction(
          title: Strings.Hotkey.newPrivateTabTitle,
          image: UIImage(braveSystemNamed: "leo.product.private-window"),
          handler: UIAction.deferredActionHandler { [weak self] _ in
            guard let self else { return }
            self.tabToolbarDelegate?.tabToolbarDidSelectNewTab(self, isPrivate: true)
          }
        )
      )
    }
    actions.append(
      UIAction(
        title: isPrivateTab ? Strings.Hotkey.newPrivateTabTitle : Strings.Hotkey.newTabTitle,
        image: UIImage(
          braveSystemNamed: isPrivateTab
            ? "leo.product.private-window" : "leo.browser.mobile-tab-new"
        ),
        handler: UIAction.deferredActionHandler { [weak self] _ in
          guard let self else { return }
          self.tabToolbarDelegate?.tabToolbarDidSelectNewTab(self, isPrivate: isPrivateTab)
        }
      )
    )
    if UIApplication.shared.supportsMultipleScenes {
      actions.append(
        UIAction(
          title: Strings.newWindowTitle,
          image: UIImage(braveSystemNamed: "leo.window.tab-new"),
          handler: UIAction.deferredActionHandler { [weak self] _ in
            guard let self else { return }
            self.tabToolbarDelegate?.tabToolbarDidSelectNewWindow(self, isPrivate: false)
          }
        )
      )
      actions.append(
        UIAction(
          title: Strings.newPrivateWindowTitle,
          image: UIImage(braveSystemNamed: "leo.window.tab-private"),
          handler: UIAction.deferredActionHandler { [weak self] _ in
            guard let self else { return }
            self.tabToolbarDelegate?.tabToolbarDidSelectNewWindow(self, isPrivate: true)
          }
        )
      )
    }
    return actions
  }
}

protocol ToolbarUrlActionsProtocol where Self: UIViewController {
  var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate? { get }
}

protocol ToolbarDelegate: AnyObject {
  func tabToolbarDidPressBack(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidPressForward(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidLongPressBack(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidLongPressForward(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidPressTabs(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidPressMenu(_ tabToolbar: ToolbarProtocol)
  func tabToolbarDidPressShare()
  func tabToolbarDidPressAddTab(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidPressSearch(_ tabToolbar: ToolbarProtocol, button: UIButton)
  func tabToolbarDidSelectNewTab(_ tabToolbar: ToolbarProtocol, isPrivate: Bool)
  func tabToolbarDidSelectNewWindow(_ tabToolbar: ToolbarProtocol, isPrivate: Bool)
  func tabToolbarDidSelectBookmarkTab(_ tabToolbar: ToolbarProtocol)
  func tabToolbarDidSelectBookmarkAllTabs(_ tabToolbar: ToolbarProtocol)
  func tabToolbarDidSelectDuplicateTab(_ tabToolbar: ToolbarProtocol)
  func tabToolbarDidSelectViewRecentlyClosedTabs(_ tabToolbar: ToolbarProtocol)
  func tabToolbarDidSelectReopenRecentlyClosedTab(_ tabToolbar: ToolbarProtocol)
  func tabToolbarDidSelectCloseTab(_ tabToolbar: ToolbarProtocol)
  func tabToolbarDidSelectShredSiteData(_ tabToolbar: ToolbarProtocol)
  func tabToolbarDidSelectCloseOtherTabs(_ tabToolbar: ToolbarProtocol)
  func tabToolbarDidSelectCloseAllTabs(_ tabToolbar: ToolbarProtocol)
  func tabToolbarDidSwipeToChangeTabs(
    _ tabToolbar: ToolbarProtocol,
    direction: UISwipeGestureRecognizer.Direction
  )
}
