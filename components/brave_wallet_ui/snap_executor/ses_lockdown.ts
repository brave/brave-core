// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// SES (Secure EcmaScript) lockdown must run before MetaMask's bundle.js so
// that Compartment and harden are available when IFrameSnapExecutor initializes.
// MetaMask's executor expects lockdown() to have already been called (they use
// LavaMoat to do this; we call it explicitly here instead).

import 'ses'

lockdown({
  errorTaming: 'unsafe',
  overrideTaming: 'severe',
  consoleTaming: 'unsafe',
  dateTaming: 'unsafe',
})
