// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

namespace base {

// A enum with simple key values
enum Keys {
  // First key
  kFirst = 1,

  // Second key
  kSecond,

  // Third key
  kThird,
};

// A enum with colours
[before=kMaxValue] enum Colours {
  // First key
  kBlue,

  // Second key
  kRed,

  [replacement] kMaxValue = kRed
};

};
