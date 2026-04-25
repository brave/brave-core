// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import Strings
@_spi(ChromiumWebViewAccess) import Web

extension TabDataValues {
  private struct PrintTabHelperKey: TabDataKey {
    static var defaultValue: PrintTabHelper?
  }
  var print: PrintTabHelper? {
    get { self[PrintTabHelperKey.self] }
    set { self[PrintTabHelperKey.self] = newValue }
  }
}

class PrintTabHelper: TabObserver {
  private weak var tab: (any TabState)?
  private weak var baseViewController: UIViewController?

  private var delegate: PrintControllerDelegate?
  private var printHandler: PrintJavaScriptFeatureHandler?
  private var isPrintControllerPresented: Bool = false
  private var isPrintingSupressed: Bool = false
  private var shouldSupressFuturePrints: Bool = false

  init(tab: some TabState, baseViewController: UIViewController) {
    self.tab = tab
    self.baseViewController = baseViewController
    self.delegate = .init(baseViewController)

    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      self.printHandler = .init { [weak self] view, title in
        self?.print(view: view, title: title)
      }
    }

    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  func print(view: UIView, title: String?) {
    if isPrintControllerPresented || isPrintingSupressed {
      return
    }

    if shouldSupressFuturePrints {
      isPrintingSupressed = true
      presentSuppressionDialog { [weak self] supress in
        guard let self else { return }
        isPrintingSupressed = supress
        if !supress {
          presentPrintController(for: view, title: title)
        }
      }
      return
    }

    shouldSupressFuturePrints = true
    presentPrintController(for: view, title: title)
  }

  private func presentPrintController(for view: UIView, title: String?) {
    let printController = UIPrintInteractionController.shared
    printController.delegate = delegate
    printController.printFormatter = view.viewPrintFormatter()
    if let title {
      let info = UIPrintInfo.printInfo()
      info.outputType = .general
      info.jobName = title
      printController.printInfo = info
    }
    isPrintControllerPresented = true
    printController.present(animated: true) { [weak self] _, _, _ in
      self?.isPrintControllerPresented = false
    }
  }

  private func presentSuppressionDialog(_ decisionHandler: @escaping (_ supress: Bool) -> Void) {
    guard let baseViewController else {
      decisionHandler(false)
      return
    }
    let confirmationDialog = UIAlertController(
      title: nil,
      message: Strings.suppressAlertsActionMessage,
      preferredStyle: UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
    )
    confirmationDialog.addAction(
      .init(
        title: Strings.suppressAlertsActionTitle,
        style: .destructive,
        handler: { _ in
          decisionHandler(true)
        }
      )
    )
    confirmationDialog.addAction(
      .init(
        title: Strings.cancelButtonTitle,
        style: .cancel,
        handler: { _ in
          decisionHandler(false)
        }
      )
    )
    if let popoverController = confirmationDialog.popoverPresentationController {
      popoverController.sourceView = baseViewController.view
      popoverController.permittedArrowDirections = []
    }
    baseViewController.present(confirmationDialog, animated: true)
  }

  // MARK: - TabObserver

  func tabDidCreateWebView(_ tab: some TabState) {
    if let printHandler {
      BraveWebView.from(tab: tab)?.setPrintHandler(printHandler)
    }
  }

  func tabDidStartNavigation(_ tab: some TabState) {
    // Reset previous supressions when a navigation starts
    shouldSupressFuturePrints = false
    isPrintingSupressed = false
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  // MARK: -

  private class PrintControllerDelegate: NSObject, UIPrintInteractionControllerDelegate {
    weak var baseViewController: UIViewController?
    init(_ baseViewController: UIViewController? = nil) {
      self.baseViewController = baseViewController
    }
    func printInteractionControllerParentViewController(
      _ printInteractionController: UIPrintInteractionController
    ) -> UIViewController? {
      baseViewController
    }
  }

  private class PrintJavaScriptFeatureHandler: NSObject, PrintHandler {
    let print: (UIView, String) -> Void
    init(print: @escaping (UIView, String) -> Void) {
      self.print = print
    }

    func printView(_ view: UIView, withTitle title: String) {
      print(view, title)
    }

    func printView(_ view: UIView, withTitle title: String, baseViewController: UIViewController) {
      // This is never called by PrintJavaScriptFeature
      assertionFailure()
    }

    func printImage(_ image: UIImage, title: String, baseViewController: UIViewController) {
      // This is never called by PrintJavaScriptFeature
      assertionFailure()
    }
  }
}
