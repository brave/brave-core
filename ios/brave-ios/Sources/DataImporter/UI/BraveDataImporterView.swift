// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import DesignSystem
import SwiftUI
import UniformTypeIdentifiers
import os.log

private struct ImporterInfo {
  var shouldShowFileImporter: Bool
  var fileTypes = [UTType.zip]
  var importType = DataImportType.all
}

public struct BraveDataImportView: View {

  @Environment(\.dismiss)
  private var dismiss

  @State
  private var importerInfo = ImporterInfo(shouldShowFileImporter: false)

  @ObservedObject
  private var model = DataImportModel()

  public init() {}

  public var body: some View {
    if model.importState == .success {
      BraveDataImporterStateView(kind: .success) {
        model.removeZipFile()
        model.resetAllStates()

        print("SYNC WITH BRAVE")  // TODO: Something...???
      } secondaryAction: {
        model.removeZipFile()
        dismiss()
      }
      .navigationBarHidden(true)
    } else if model.importState == .failure {
      BraveDataImporterStateView(kind: .failure) {
      } secondaryAction: {
        model.removeZipFile()
        model.resetAllStates()
      }
      .navigationBarHidden(true)
    } else if model.importState == .importing {
      BraveDataImporterLoadingView()
        .navigationBarHidden(true)
    } else {
      VStack {
        VStack(alignment: .center, spacing: 16.0) {
          Image(
            "main_import_logo",
            bundle: .module
          )
          .padding(.bottom, 24.0)

          Text("Import Bookmarks and more")
            .font(.body.weight(.semibold))
            .multilineTextAlignment(.center)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            .frame(maxWidth: .infinity, alignment: .center)
            .fixedSize(horizontal: false, vertical: true)
            .padding(.horizontal, 24.0)

          Text("Bring your bookmarks, history, and other browser data into Brave.")
            .font(.footnote)
            .multilineTextAlignment(.center)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
            .frame(maxWidth: .infinity, alignment: .center)
            .fixedSize(horizontal: false, vertical: true)
            .padding(.horizontal, 24.0)

          Button(
            action: {
              importerInfo = ImporterInfo(
                shouldShowFileImporter: true,
                fileTypes: [.zip],
                importType: .all
              )
            },
            label: {
              Text("Choose a file...")
                .font(.body.weight(.semibold))
                .padding()
                .foregroundStyle(Color(braveSystemName: .schemesOnPrimary))
                .frame(maxWidth: .infinity, alignment: .center)
                .background(
                  Color(braveSystemName: .buttonBackground),
                  in: RoundedRectangle(cornerRadius: 12.0, style: .continuous)
                )
            }
          )
          .buttonStyle(.plain)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)

        Spacer()

        NavigationLink(destination: BraveDataImporterTutorialView()) {
          HStack {
            Image(
              "safari_icon",
              bundle: .module
            )

            Text("How to export from Safari")
              .font(.subheadline.weight(.semibold))
              .foregroundStyle(Color(braveSystemName: .textInteractive))
              .frame(maxWidth: .infinity, alignment: .leading)
              .padding(12.0)

            Image(braveSystemName: "leo.carat.right")
              .foregroundStyle(Color(braveSystemName: .iconDefault))
              .frame(alignment: .trailing)
          }
          .frame(maxWidth: .infinity, alignment: .leading)
          .padding(.horizontal, 16.0)
          .background(
            Color(braveSystemName: .containerDisabled),
            in: RoundedRectangle(cornerRadius: 12.0, style: .continuous)
          )
        }
      }
      .padding(16.0)
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
      .navigationTitle("Import Browsing Data")
      .navigationBarHidden(false)
      .braveSheet(
        isPresented: Binding(
          get: { model.importState == .loadingProfiles },
          set: { if !$0 { model.importState = .none } }
        ),
        onDismiss: {
          model.removeZipFile()
        },
        content: {
          if let zipFileURL = model.zipFileURL {
            BraveDataImporterMultipleProfilesView(
              zipFileExtractedURL: zipFileURL,
              profiles: model.profiles,
              onProfileSelected: onProfileSelected
            )
            .edgesIgnoringSafeArea(.bottom)
          }
        }
      )
      .braveSheet(
        isPresented: Binding(
          get: { model.importState == .dataConflict },
          set: {
            if !$0 {
              model.removeZipFile()
              model.resetAllStates()
            }
          }
        ),
        onDismiss: {
          Task {
            await model.keepPasswords(option: .abortImport)
            model.removeZipFile()
            model.resetAllStates()
          }
        },
        content: {
          BraveDataImporterStateView(
            kind: .passwordConflict
          ) {
            Task {
              await model.keepPasswords(option: .keepBravePasswords)
            }
          } secondaryAction: {
            Task {
              await model.keepPasswords(option: .keepSafariPasswords)
            }
          }
          .clipShape(RoundedRectangle(cornerRadius: 15.0, style: .continuous))
          .edgesIgnoringSafeArea(.all)
        }
      )
      .fileImporter(
        isPresented: $importerInfo.shouldShowFileImporter,
        allowedContentTypes: importerInfo.fileTypes,
        allowsMultipleSelection: false
      ) { result in
        switch result {
        case .success(let files):
          Task {
            await onFileSelected(files: files)
          }
        case .failure(let error):
          Logger.module.error("[DataImporter] - File Selector Error: \(error)")
          model.importError = .unknown
        }
      }
    }
  }

  private func onFileSelected(files: [URL]) async {
    await files.asyncForEach { file in
      if file.startAccessingSecurityScopedResource() {
        defer { file.stopAccessingSecurityScopedResource() }

        await model.importData(from: file, importType: importerInfo.importType)
      }
    }
  }

  private func onProfileSelected(profile: String) {
    if let selectedProfile = model.profiles[profile] {
      Task {
        defer {
          model.removeZipFile()
        }

        await model.importData(from: selectedProfile)
      }
    }
  }
}

#Preview {
  BraveDataImportView()
}
