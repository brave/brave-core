/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Static

extension DataSource {
  /// Get the index path of a Row to modify it
  ///
  /// Since they are structs we cannot obtain references to them to alter them, we must directly access them
  /// from `sections[x].rows[y]`
  public func indexPath(rowUUID: String, sectionUUID: String) -> IndexPath? {
    guard let section = sections.firstIndex(where: { $0.uuid == sectionUUID }),
      let row = sections[section].rows.firstIndex(where: { $0.uuid == rowUUID })
    else {
      return nil
    }
    return IndexPath(row: row, section: section)
  }

  public func reloadCell(row: Row, section: Static.Section, displayText: String) {
    if let indexPath = indexPath(rowUUID: row.uuid, sectionUUID: section.uuid) {
      sections[indexPath.section].rows[indexPath.row].detailText = displayText
    }
  }
}
