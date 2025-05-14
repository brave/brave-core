// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Alias } from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

export type EditMode =
  | 'None'
  | 'Create'
  | 'Edit'

export type EditState = {
  mode: EditMode,
  alias?: Alias
}
