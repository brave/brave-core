//
//  SwiftUIView.swift
//  Brave
//
//  Created by Brandon T on 2025-01-22.
//

import BraveCore
import SwiftUI

struct BraveDataImporterLoadingView: View {
  var body: some View {
    EmptyView()
  }

  //  private func importBookmarks(_ file: URL) async throws {
  //    let importer = BookmarksImportExportUtility()
  //    let didImport = await importer.importBookmarks(from: file)
  //    if !didImport {
  //      throw ImportError.failedToImportBookmarks
  //    }
  //  }
  //
  //  private func importHistory(_ file: URL) async throws {
  //    let importer = HistoryImportExportUtility()
  //    let didImport = await importer.importHistory(from: file)
  //    if !didImport {
  //      throw ImportError.failedToImportHistory
  //    }
  //  }
  //
  //  private func importPasswords(_ file: URL) async throws {
  //    let importer = PasswordsImportExportUtility()
  //    let didImport = await importer.importPasswords(from: file)
  //    if !didImport {
  //      throw ImportError.failedToImportPasswords
  //    }
  //  }
}

#Preview {
  BraveDataImporterLoadingView()
}
