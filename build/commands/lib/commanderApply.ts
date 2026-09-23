// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Command, type OptionValues } from 'commander'

// Passes the command to a function that adds arguments or options to it, so
// reusable groups of them can be applied without breaking the call chain.
declare module '@commander-js/extra-typings' {
  interface Command<
    Args extends any[] = [],
    Opts extends OptionValues = {},
    GlobalOpts extends OptionValues = {},
  > {
    apply<R>(fn: (command: this) => R): R
  }
}

Command.prototype.apply = function <R>(
  this: Command,
  fn: (command: Command) => R,
): R {
  return fn(this)
}
