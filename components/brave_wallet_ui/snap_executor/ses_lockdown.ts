// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// SES (Secure EcmaScript) lockdown must run before any snap code is executed.
// This hardens the JS environment inside the snap executor iframe, preventing
// prototype pollution and other sandbox escape attacks.

import 'ses'

lockdown({
  errorTaming: 'unsafe',
  overrideTaming: 'severe',
  consoleTaming: 'unsafe'
})
