// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import DesignSystem
import SwiftUI
import UniformTypeIdentifiers
import os.log

public struct DataImportView: View {
  @State private var isPresentingFileImporter: Bool = false
  @State private var isPresentingConflictResolution: Bool = false

  private var model: SafariDataImportModel

  private var openURL: (URL) -> Void
  private var dismiss: () -> Void
  private var onDismiss: () -> Void

  public init(
    model: SafariDataImportModel,
    openURL: @escaping (URL) -> Void,
    dismiss: @escaping () -> Void,
    onDismiss: @escaping () -> Void
  ) {
    self.model = model
    self.openURL = openURL
    self.dismiss = dismiss
    self.onDismiss = onDismiss
  }

  public var body: some View {
    ZStack {
      if model.state == .success {
        DataImporterSuccessView {
          onDismiss()
          dismiss()
        }
        .padding(16.0)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(braveSystemName: .pageBackground))
        .transition(.opacity)
      } else if model.state == .failure {
        DataImporterFailedView {
          model.reset()
        }
        .padding(16.0)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(braveSystemName: .pageBackground))
        .transition(.opacity)
      } else if model.state == .importing || model.state == .passwordConflict {
        DataImporterLoadingView()
          .padding(16.0)
          .frame(maxWidth: .infinity, maxHeight: .infinity)
          .background(Color(braveSystemName: .pageBackground))
          .transition(.opacity)
      } else {
        GeometryReader { proxy in
          ScrollView {
            mainView
              .frame(minHeight: proxy.size.height)
          }
          .background(Color(braveSystemName: .pageBackground))
        }
      }
    }
    .sheet(
      isPresented: $isPresentingConflictResolution,
      content: {
        DataImporterPasswordConflictView(model: model)
          .interactiveDismissDisabled()
      }
    )
    .fileImporter(
      isPresented: $isPresentingFileImporter,
      allowedContentTypes: [.zip],
      allowsMultipleSelection: false
    ) { result in
      switch result {
      case .success(let files):
        guard let file = files.first else { return }
        model.beginImport(for: file)
      case .failure(let error):
        Logger.module.error("[DataImporter] - File Selector Error: \(error)")
        model.state = .failure
      }
    }
    .animation(.default, value: model.state)
    .toolbar(
      [.success, .failure, .importing, .passwordConflict].contains(model.state)
        ? .hidden : .visible,
      for: .navigationBar
    )
    .onDisappear {
      onDismiss()
    }
    .onChange(of: model.state) { _, newValue in
      isPresentingConflictResolution = newValue == .passwordConflict
    }
  }

  @ViewBuilder
  private var mainView: some View {
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
            isPresentingFileImporter = true
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
  }
}
