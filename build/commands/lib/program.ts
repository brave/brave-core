// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Command } from 'commander'
import './commanderApply.ts'
import { Config, setConfigInstance } from './config.ts'

// Installs the checkout's config as `config` once `command`'s arguments are
// parsed. Apply it before adding any other hook: commander runs a command's
// hooks in the order they are added, and before its subcommands', so the
// config is then there for every hook and action.
export function installConfig<
  Args extends any[],
  Opts extends {},
  GlobalOpts extends {},
>(command: Command<Args, Opts, GlobalOpts>) {
  return command.hook('preAction', () => setConfigInstance(Config.load()))
}

// A command-line script's program, with installConfig() applied.
export const program = installConfig(new Command())
