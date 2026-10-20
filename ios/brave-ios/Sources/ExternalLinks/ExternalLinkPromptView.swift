// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveStrings
import BraveUI
import DesignSystem
import Strings
import SwiftUI
import UIKit

public struct ExternalLinkPromptView: View {
  var url: URL
  var onChoice: (ExternalLinkChoice, _ remember: Bool) -> Void

  @State private var rememberChoice = false
  @Environment(\.dismiss) private var dismiss

  var onContentHeightChanged: ((CGFloat) -> Void)?

  public init(url: URL, onChoice: @escaping (ExternalLinkChoice, _ remember: Bool) -> Void) {
    self.url = url
    self.onChoice = onChoice
  }

  public var body: some View {
    NavigationStack {
      Form {
        Section {
          Text(urlLabel)
            .font(.footnote)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
            .truncationMode(.middle)
            .fixedSize(horizontal: false, vertical: true)
            .frame(maxWidth: .infinity, alignment: .leading)
            .contentShape(.rect)
            .contextMenu {
              Button {
                UIPasteboard.general.url = url
              } label: {
                Label(Strings.copyLinkActionTitle, braveSystemImage: "leo.copy")
              }
            }
        }
        Section {
          ForEach(ExternalLinkChoice.allAvailable) { choice in
            Button {
              onChoice(choice, rememberChoice)
            } label: {
              Label {
                LabelView(
                  title: choice.contentSettingMode.localizedTitle,
                  subtitle: choice.localizedPromptDescription
                )
              } icon: {
                Image(braveSystemName: choice.braveSystemImage)
              }
              .frame(maxWidth: .infinity, alignment: .leading)
              .contentShape(.rect)
            }
            .buttonStyle(.plain)
          }
        }
        if let baseDomain = url.baseDomain {
          Section {
            Toggle(isOn: $rememberChoice) {
              Text(rememberChoiceLabel(baseDomain: baseDomain))
            }
            .tint(Color(braveSystemName: .primary40))
          }
        }
      }
      .listSectionSpacing(.compact)
      .contentMargins(.top, 8, for: .scrollContent)
      // A `Form` has no usable ideal size, so measure the underlying scroll view. The top inset
      // covers the navigation bar; the bottom safe area is excluded since sheet detents already
      // account for it, and including it feeds back into the measured height.
      .onScrollGeometryChange(for: CGFloat.self) { geometry in
        geometry.contentSize.height + geometry.contentInsets.top
      } action: { _, height in
        onContentHeightChanged?(height)
      }
      .navigationTitle(Strings.ExternalLinks.promptTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .topBarLeading) {
          Button {
            dismiss()
          } label: {
            Label(Strings.close, braveSystemImage: "leo.close")
          }
        }
      }
    }
  }

  private var urlLabel: AttributedString {
    var label = AttributedString(url.absoluteString)
    if let host = url.host(), let baseDomain = url.baseDomain,
      let hostRange = label.range(of: host),
      let range = label[hostRange].range(of: baseDomain, options: .backwards)
    {
      label[range].inlinePresentationIntent = .stronglyEmphasized
    }
    return label
  }

  private func rememberChoiceLabel(baseDomain: String) -> AttributedString {
    var label = AttributedString(
      String.localizedStringWithFormat(Strings.ExternalLinks.rememberChoiceForDomain, baseDomain)
    )
    if let range = label.range(of: baseDomain) {
      label[range].inlinePresentationIntent = .stronglyEmphasized
    }
    return label
  }
}

public class ExternalLinkPromptViewController: UIHostingController<ExternalLinkPromptView> {
  public init(url: URL, onChoice: @escaping (ExternalLinkChoice, _ remember: Bool) -> Void) {
    super.init(rootView: ExternalLinkPromptView(url: url, onChoice: onChoice))
    rootView.onContentHeightChanged = { [weak self] height in
      self?.updateContentHeight(height)
    }
  }

  @MainActor required dynamic init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  /// The height the prompt's contents need, as measured by SwiftUI. Zero until the first
  /// measurement lands.
  private var contentHeight: CGFloat = 0

  private func updateContentHeight(_ height: CGFloat) {
    // Ignore rounding differences during a resize to avoid needlessly invalidating detents
    guard height > 0, abs(height - contentHeight) > 1 else { return }
    let isFirstMeasurement = contentHeight == 0
    contentHeight = height
    guard let sheetController = sheetPresentationController else { return }
    if isFirstMeasurement {
      sheetController.invalidateDetents()
    } else {
      sheetController.animateChanges {
        sheetController.invalidateDetents()
      }
    }
  }

  public override func viewIsAppearing(_ animated: Bool) {
    super.viewIsAppearing(animated)

    if let sheetController = sheetPresentationController {
      sheetController.detents = [
        .custom(identifier: .fitsContent) { [weak self] context in
          guard let height = self?.contentHeight, height > 0 else {
            return context.maximumDetentValue
          }
          return min(context.maximumDetentValue, height)
        },
        .large(),
      ]
      sheetController.prefersGrabberVisible = true
      sheetController.prefersEdgeAttachedInCompactHeight = true
    }
  }
}

extension UISheetPresentationController.Detent.Identifier {
  fileprivate static let fitsContent: Self = .init("fitsContent")
}

#if DEBUG
#Preview("Prompt") {
  ExternalLinkPromptView(
    url: URL(string: "https://search.brave.com/search?q=brave+browser&source=ios")!
  ) { _, _ in }
}

#Preview("Sheet Presented") {
  Color.clear
    .sheet(isPresented: .constant(true)) {
      ExternalLinkPromptView(
        url: URL(string: "https://news.example.co.uk/articles/2026/10/a-very-long-path")!
      ) { _, _ in }
      .presentationDetents([.medium, .large])
      .presentationDragIndicator(.visible)
    }
}
#endif
