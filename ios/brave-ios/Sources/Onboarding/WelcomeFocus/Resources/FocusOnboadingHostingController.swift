// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import UIKit

public class FocusOnboardingHostingController<Content>: UIHostingController<Content>
where Content: View {
  public override var preferredStatusBarStyle: UIStatusBarStyle {
    return .lightContent
  }

  public override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    return .portrait
  }

  public override var preferredInterfaceOrientationForPresentation: UIInterfaceOrientation {
    return .portrait
  }

  public override var shouldAutorotate: Bool {
    return false
  }
}
