// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  public struct DataImporter {
    /// Important: Do NOT change the `KEY` parameter without updating it in
    /// BraveCore's brave_bookmarks_importer.mm file.
    public static let importFolderName =
      NSLocalizedString(
        "SyncImportFolderName",
        tableName: "DataImporter",
        bundle: .module,
        value: "Imported Bookmarks",
        comment:
          "Folder name for where bookmarks are imported into when the root folder is not empty."
      )
  }
}
