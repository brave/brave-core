// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Shared
import Foundation
import UIKit

extension SearchViewController {
  func handleKeyCommands(sender: UIKeyCommand) {
    let initialSection = SearchSuggestionDataSource.SearchListSection.openTabsAndHistoryAndBookmarks.rawValue

    guard let current = tableView.indexPathForSelectedRow else {
      let numberOfRows = tableView(tableView, numberOfRowsInSection: initialSection)
      if sender.input == UIKeyCommand.inputDownArrow, numberOfRows > 0 {
        let next = IndexPath(item: 0, section: initialSection)
        self.tableView(tableView, didHighlightRowAt: next)
        tableView.selectRow(at: next, animated: false, scrollPosition: .top)
      }
      return
    }

    let nextSection: Int
    let nextItem: Int
    guard let input = sender.input else { return }

    switch input {
    case UIKeyCommand.inputUpArrow:
      // we're going down, we should check if we've reached the first item in this section.
      if current.item == 0 {
        // We have, so check if we can decrement the section.
        if current.section == initialSection {
          // We've reached the first item in the first section.
          searchDelegate?.searchViewController(
            self,
            didHighlightText: dataSource.searchQuery,
            search: false)
          return
        } else {
          nextSection = current.section - 1
          nextItem = tableView(tableView, numberOfRowsInSection: nextSection) - 1
        }
      } else {
        nextSection = current.section
        nextItem = current.item - 1
      }
    case UIKeyCommand.inputDownArrow:
      let currentSectionItemsCount = tableView(tableView, numberOfRowsInSection: current.section)
      if current.item == currentSectionItemsCount - 1 {
        if current.section == tableView.numberOfSections - 1 {
          // We've reached the last item in the last section
          return
        } else {
          // We can go to the next section.
          nextSection = current.section + 1
          nextItem = 0
        }
      } else {
        nextSection = current.section
        nextItem = current.item + 1
      }
    default:
      return
    }

    guard nextItem >= 0 else { return }

    let next = IndexPath(item: nextItem, section: nextSection)
    tableView(tableView, didHighlightRowAt: next)
    tableView.selectRow(at: next, animated: false, scrollPosition: .middle)
  }
}
