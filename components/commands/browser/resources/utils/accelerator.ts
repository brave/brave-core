// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { type Accelerator } from 'gen/brave/components/commands/common/commands.mojom.m'

export const allKeys = (accelerator: Accelerator) => [...accelerator.modifiers, accelerator.keycode]
