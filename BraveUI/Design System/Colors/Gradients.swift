// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
import UIKit

final public class Gradients {
  public struct Light {
    public static var gradient02: GradientView {
      GradientView(colors: [#colorLiteral(red: 0.4352941176, green: 0.2980392157, blue: 0.8235294118, alpha: 1), #colorLiteral(red: 0.7490196078, green: 0.07843137255, blue: 0.6352941176, alpha: 1), #colorLiteral(red: 0.968627451, green: 0.2274509804, blue: 0.1098039216, alpha: 1)],
                          positions: [0.1581, 0.6317, 1.0],
                          angle: 304.74)
    }
  }
  
  public struct Dark {
    public static var gradient02: GradientView {
      GradientView(colors: [#colorLiteral(red: 0.4352941176, green: 0.2980392157, blue: 0.8235294118, alpha: 1), #colorLiteral(red: 0.7490196078, green: 0.07843137255, blue: 0.6352941176, alpha: 1), #colorLiteral(red: 0.968627451, green: 0.2274509804, blue: 0.1098039216, alpha: 1)],
                          positions: [0.1581, 0.6317, 1.0],
                          angle: 304.74)
    }
  }
}
