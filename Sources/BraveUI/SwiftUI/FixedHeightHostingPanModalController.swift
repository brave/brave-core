/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import PanModal

/// A controller that manages some SwiftUI that is expected to be a fixed size and
/// presented in a `PanModal`
///
/// Use this when you want to present some UI at the bottom of the screen via PanModal
/// but you don't need it do expand but still want the ability to dismiss via gesture.
///
/// - note: The SwiftUI within this class should not scroll as `panScrollable` is hard-coded to `nil`
public class FixedHeightHostingPanModalController<Content: View>: UIViewController, PanModalPresentable {
  private let hostingController: UIHostingController<Content>
  public init(rootView: Content) {
    hostingController = .init(rootView: rootView)
    super.init(nibName: nil, bundle: nil)
  }
  @available(*, unavailable)
  public required init(coder: NSCoder) {
    fatalError()
  }
  public override func viewDidLoad() {
    super.viewDidLoad()
    // In order to have a fixed size SwiftUI pan modal, we have to add the SwiftUI
    // as a child controller, and only constrain it to top/leading/trailing. Then
    // when we compute the height for short form & long form we have to use that
    // hosting controller
    addChild(hostingController)
    hostingController.didMove(toParent: self)
    view.addSubview(hostingController.view)
    view.backgroundColor = .braveBackground
    hostingController.view.backgroundColor = .braveBackground
    hostingController.view.snp.makeConstraints {
      $0.top.leading.trailing.equalToSuperview()
    }
    NotificationCenter.default.addObserver(self, selector: #selector(sizeCategoryChanged), name: UIContentSizeCategory.didChangeNotification, object: nil)
  }
  public override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    panModalSetNeedsLayoutUpdate()
  }
  @objc private func sizeCategoryChanged() {
    panModalSetNeedsLayoutUpdate()
  }
  public var panScrollable: UIScrollView? {
    nil
  }
  private func hostingControllerIntrinsicHeight(for containerSize: CGSize) -> CGFloat {
    hostingController.view.systemLayoutSizeFitting(
      containerSize,
      withHorizontalFittingPriority: .required,
      verticalFittingPriority: .fittingSizeLevel
    ).height
  }
  public var shortFormHeight: PanModalHeight {
    return .contentHeight(hostingControllerIntrinsicHeight(for: view.bounds.size))
  }
  public var longFormHeight: PanModalHeight {
    return .contentHeight(hostingControllerIntrinsicHeight(for: view.bounds.size))
  }
  public override var preferredContentSize: CGSize {
    get {
      let containerSize = CGSize(width: min(375, view.bounds.width), height: view.bounds.height)
      return .init(width: containerSize.width, height: hostingControllerIntrinsicHeight(for: containerSize))
    }
    set {}
  }
}
