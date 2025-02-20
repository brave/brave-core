// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import DesignSystem
import SwiftUI
import UniformTypeIdentifiers
import os.log

private struct ImporterInfo {
  var shouldShowFileImporter: Bool
  var fileTypes = [UTType.zip]
  var importType = DataImportType.all
}

public struct DataImportView: View {

  @Environment(\.dismiss)
  private var dismiss

  @State
  private var showSuccessView = false

  @State
  private var showFailureView = false

  @State
  private var showLoadingView = false

  @State
  private var showMainView = true

  @State
  private var importerInfo = ImporterInfo(shouldShowFileImporter: false)

  @ObservedObject
  private var model = DataImportModel()

  private var openURL: (URL) -> Void
  private var onDismiss: () -> Void

  public init(openURL: @escaping (URL) -> Void, onDismiss: @escaping () -> Void) {
    self.openURL = openURL
    self.onDismiss = onDismiss
  }

  public var body: some View {
    ZStack {
      if model.importState == .success {
        DataImporterStateView(kind: .success, model: model) {
          model.removeZipFile()

          onDismiss()
          dismiss()
        }
        .padding(16.0)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(braveSystemName: .pageBackground))
        .toolbar(.hidden, for: .navigationBar)
        .opacity(showSuccessView ? 1.0 : 0.0)
        .onAppear {
          withAnimation(.easeInOut(duration: 0.5)) {
            showSuccessView = true
          }
        }
        .onDisappear {
          withAnimation(.easeInOut(duration: 0.5)) {
            showSuccessView = false
          }
        }
      } else if model.importState == .failure {
        DataImporterStateView(kind: .failure, model: model) {
          model.removeZipFile()
          model.resetAllStates()
        }
        .padding(16.0)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(braveSystemName: .pageBackground))
        .toolbar(.hidden, for: .navigationBar)
        .opacity(showFailureView ? 1.0 : 0.0)
        .onAppear {
          withAnimation(.easeInOut(duration: 0.5)) {
            showFailureView = true
          }
        }
        .onDisappear {
          withAnimation(.easeInOut(duration: 0.5)) {
            showFailureView = false
          }
        }
      } else if model.importState == .importing {
        DataImporterLoadingView()
          .padding(16.0)
          .frame(maxWidth: .infinity, maxHeight: .infinity)
          .background(Color(braveSystemName: .pageBackground))
          .toolbar(.hidden, for: .navigationBar)
          .opacity(showLoadingView ? 1.0 : 0.0)
          .onAppear {
            withAnimation(.easeInOut(duration: 0.5)) {
              showLoadingView = true
            }
          }
          .onDisappear {
            withAnimation(.easeInOut(duration: 0.5)) {
              showLoadingView = false
            }
          }
      } else {
        GeometryReader { proxy in
          ScrollView {
            mainView()
              .frame(minHeight: proxy.size.height)
          }
          .background(Color(braveSystemName: .pageBackground))
        }
        .sheet(
          isPresented: $model.isLoadingProfiles,
          onDismiss: {
            model.removeZipFile()
          },
          content: {
            if let zipFileURL = model.zipFileURL {
              GeometryReader { proxy in
                ScrollView {
                  multiProfileView(zipFileURL: zipFileURL)
                    .frame(minHeight: proxy.size.height)
                }
                .osAvailabilityModifiers({
                  if #available(iOS 16.4, *) {
                  } else {
                    $0.background(.thickMaterial)
                  }
                })
              }
            }
          }
        )
        .sheet(
          isPresented: $model.hasDataConflict,
          onDismiss: {
            Task {
              await model.keepPasswords(option: .abortImport)
              model.removeZipFile()
              model.resetAllStates()
            }
          },
          content: {
            GeometryReader { proxy in
              ScrollView {
                passwordsConflictView()
                  .frame(minHeight: proxy.size.height)
              }
              .osAvailabilityModifiers({
                if #available(iOS 16.4, *) {
                } else {
                  $0.background(.thickMaterial)
                }
              })
            }
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
    .onDisappear {
      model.removeZipFile()
      onDismiss()
    }
  }

  @ViewBuilder
  private func mainView() -> some View {
    VStack {
      VStack(alignment: .center, spacing: 16.0) {
        Image("main_import_logo", bundle: .module)
          .padding(.bottom, 24.0)

        VStack {
          Text(Strings.DataImporter.importDataFileSelectorTitle)
            .font(.headline)
            .foregroundStyle(Color(braveSystemName: .textPrimary))

          Text(Strings.DataImporter.importDataFileSelectorMessage)
            .font(.footnote)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
        .multilineTextAlignment(.center)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
        .frame(maxWidth: .infinity)
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
            Text(Strings.DataImporter.importDataFileSelectorButtonTitle)
              .font(.headline)
              .padding(.horizontal)
              .padding(.vertical, 12.0)
              .foregroundStyle(Color(braveSystemName: .schemesOnPrimary))
              .frame(maxWidth: .infinity)
              .background(
                Color(braveSystemName: .buttonBackground),
                in: RoundedRectangle(cornerRadius: 12.0, style: .continuous)
              )
          }
        )
        .buttonStyle(.plain)
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity)

      Spacer()

      NavigationLink(
        destination: DataImporterTutorialView().environment(
          \.openURL,
          OpenURLAction(handler: {
            openURL($0)
            return .handled
          })
        )
      ) {
        HStack {
          Image("safari_icon", bundle: .module)

          Text(Strings.DataImporter.importDataViewTutorialButtonTitle)
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
    .navigationTitle(Strings.DataImporter.importDataFileSelectorNavigationTitle)
    .toolbar(.visible, for: .navigationBar)
    .opacity(showMainView ? 1.0 : 0.0)
    .animation(.easeInOut(duration: 0.5), value: model.importState)
    .onAppear {
      withAnimation(.easeInOut(duration: 0.5)) {
        showMainView = true
      }
    }
    .onDisappear {
      withAnimation(.easeInOut(duration: 0.5)) {
        showMainView = false
      }
    }
  }

  @ViewBuilder
  private func passwordsConflictView() -> some View {
    DataImporterStateView(
      kind: .passwordConflict,
      model: model
    ) {
      Task {
        await model.keepPasswords(option: .keepBravePasswords)
      }
    } secondaryAction: {
      Task {
        await model.keepPasswords(option: .keepSafariPasswords)
      }
    }
    .padding(16.0)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .presentationDetents([.fraction(0.60)])
    .presentationDragIndicator(.visible)
    .osAvailabilityModifiers({
      if #available(iOS 16.4, *) {
        $0.presentationBackground(.thickMaterial)
          .presentationCornerRadius(15.0)
          .presentationCompactAdaptation(.sheet)
      } else {
        $0.background(.thickMaterial)
      }
    })
  }

  @ViewBuilder
  private func multiProfileView(zipFileURL: URL) -> some View {
    DataImporterMultipleProfilesView(
      zipFileExtractedURL: zipFileURL,
      profiles: model.profiles,
      onProfileSelected: onProfileSelected
    )
    .padding(16.0)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .presentationDetents([.fraction(0.60)])
    .presentationDragIndicator(.visible)
    .osAvailabilityModifiers({
      if #available(iOS 16.4, *) {
        $0.presentationBackground(.thickMaterial)
          .presentationCornerRadius(15.0)
          .presentationCompactAdaptation(.sheet)
      } else {
        $0.background(.thickMaterial)
      }
    })
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

#if DEBUG
#Preview {
  DataImportView(openURL: { _ in }, onDismiss: {})
}
#endif
