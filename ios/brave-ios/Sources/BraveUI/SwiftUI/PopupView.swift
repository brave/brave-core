/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import DesignSystem

/// Presents a SwiftUI view heirarchy in a popup that displays in the center of the screen
public class PopupViewController<Content: View>: UIViewController, UIViewControllerTransitioningDelegate, BasicAnimationControllerDelegate {
  private let hostingController: UIHostingController<PopupView<Content>>

  private let backgroundView = UIView().then {
    $0.backgroundColor = UIColor(white: 0.0, alpha: 0.3)
  }

  public init(rootView: Content, isDismissable: Bool = false) {
    let popup = PopupView({ rootView })
    hostingController = UIHostingController(rootView: popup)
    super.init(nibName: nil, bundle: nil)
    transitioningDelegate = self
    modalPresentationStyle = .overFullScreen
    addChild(hostingController)
    hostingController.didMove(toParent: self)
    if isDismissable {
      hostingController.rootView.onBackgroundTap = { [unowned self] in
        self.dismiss(animated: true)
      }
    }
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = .clear
    hostingController.view.backgroundColor = .clear

    view.addSubview(backgroundView)
    view.addSubview(hostingController.view)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    hostingController.view.snp.makeConstraints {
      $0.edges.equalTo(view.safeAreaLayoutGuide)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  public func animatePresentation(context: UIViewControllerContextTransitioning) {
    view.frame = context.finalFrame(for: self)
    context.containerView.addSubview(view)

    backgroundView.alpha = 0.0
    hostingController.view.transform = CGAffineTransform(translationX: 0, y: context.containerView.bounds.height)

    UIViewPropertyAnimator(duration: 0.35, dampingRatio: 1.0) { [self] in
      backgroundView.alpha = 1.0
      hostingController.view.transform = .identity
    }.startAnimation()

    context.completeTransition(true)
  }

  public func animateDismissal(context: UIViewControllerContextTransitioning) {
    let animator = UIViewPropertyAnimator(duration: 0.25, dampingRatio: 1.0) { [self] in
      backgroundView.alpha = 0.0
      hostingController.view.transform = CGAffineTransform(translationX: 0, y: context.containerView.bounds.height)
    }
    animator.addCompletion { _ in
      self.view.removeFromSuperview()
      context.completeTransition(true)
    }
    animator.startAnimation()
  }

  public func animationController(forDismissed dismissed: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    BasicAnimationController(delegate: self, direction: .dismissing)
  }

  public func animationController(forPresented presented: UIViewController, presenting: UIViewController, source: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    BasicAnimationController(delegate: self, direction: .presenting)
  }
}

public struct PopupView<Content: View>: View {
  public var content: Content
  public var onBackgroundTap: (() -> Void)?

  public init(@ViewBuilder _ content: () -> Content) {
    self.content = content()
  }

  public var body: some View {
    content
      .frame(maxWidth: 400)
      .background(Color(.braveBackground))
      .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
      .shadow(color: .black.opacity(0.2), radius: 2, x: 0, y: 1)
      .padding()
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .background(
        Color.clear
          .contentShape(Rectangle())
          .onTapGesture {
            onBackgroundTap?()
          }
      )
  }
}

#if DEBUG
struct PopupPreviews: PreviewProvider {
  static var previews: some View {
    Group {
      PopupView {
        VStack {
          Circle()
            .frame(width: 100, height: 100)
          Text(verbatim: "Title")
            .font(.headline)
          Text(verbatim: "Subtitle Subtitle Subtitle Subtitle Subtitle")
            .font(.subheadline)
            .multilineTextAlignment(.center)
          Button(action: {}) {
            Text(verbatim: "Test")
          }
          .padding(.top)
          .buttonStyle(BraveFilledButtonStyle(size: .normal))
        }
      }
      .previewLayout(.sizeThatFits)
      .background(Color.gray)
    }
  }
}
#endif
