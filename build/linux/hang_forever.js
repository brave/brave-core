// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Intentionally hangs forever. Used with //brave/build:diagnostic_node_hang_forever
// (devtools node_action) to exercise build hang diagnostics. Low CPU.

while (true) {
  await new Promise(resolve => setTimeout(resolve, 10000));
}
