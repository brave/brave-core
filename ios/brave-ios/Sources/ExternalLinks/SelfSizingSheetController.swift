// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import UIKit

private struct SheetContentHeightChangedKey: EnvironmentKey {
  static let defaultValue: ((CGFloat) -> Void)? = nil
}

extension EnvironmentValues {
  fileprivate var sheetContentHeightChanged: ((CGFloat) -> Void)? {
    get { self[SheetContentHeightChangedKey.self] }
    set { self[SheetContentHeightChangedKey.self] = newValue }
  }
}

extension View {
  /// Reports the height of this scrollable content (e.g. a `Form`) to an enclosing
  /// `SelfSizingSheetController`.
  func reportsSheetContentHeight() -> some View {
    modifier(SheetContentHeightModifier())
  }
}

private struct SheetContentHeightModifier: ViewModifier {
  @Environment(\.sheetContentHeightChanged) private var sheetContentHeightChanged

  func body(content: Content) -> some View {
    // A `Form` has no usable ideal size, so measure the underlying scroll view. The top inset
    // covers the navigation bar; the bottom safe area is excluded since sheet detents already
    // account for it, and including it feeds back into the measured height.
    content
      .onScrollGeometryChange(for: CGFloat.self) { geometry in
        geometry.contentSize.height + geometry.contentInsets.top
      } action: { _, height in
        sheetContentHeightChanged?(height)
      }
  }
}

/// Presents SwiftUI content in a sheet sized to fit it. The content must apply
/// `reportsSheetContentHeight()` to its scrollable container.
public class SelfSizingSheetController: UIHostingController<AnyView> {
  public init(rootView: some View) {
    super.init(rootView: AnyView(EmptyView()))
    self.rootView = AnyView(
      rootView.environment(
        \.sheetContentHeightChanged,
        { [weak self] height in
          self?.updateContentHeight(height)
        }
      )
    )
  }

  @MainActor required dynamic init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  /// The height the contents need, as measured by SwiftUI. Zero until the first measurement lands.
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
      sheetController.prefersGrabberVisible = !isModalInPresentation
      sheetController.prefersEdgeAttachedInCompactHeight = true
    }
  }
}

extension UISheetPresentationController.Detent.Identifier {
  fileprivate static let fitsContent: Self = .init("fitsContent")
}
