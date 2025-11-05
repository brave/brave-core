// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import CarPlay

/// A CarPlay controller that is shown when playlist is unavailable due to admin policies
///
/// This version simply shows the now playing screen at the root
public class CarPlayUnavailableController {
  public let interface: CPInterfaceController
  public init(interface: CPInterfaceController) {
    self.interface = interface
    interface.setRootTemplate(CPNowPlayingTemplate.shared, animated: false, completion: nil)
  }
}
