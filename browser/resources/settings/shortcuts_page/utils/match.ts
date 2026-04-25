// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { type Command } from 'gen/brave/components/commands/common/commands.mojom.m'
import { stringToKeys } from './accelerator'

export const match = (query: string, command: Command) => {
  if (command.id === parseInt(query)) return true

  const queryUpper = query.toUpperCase()
  if (command.name.toUpperCase().includes(queryUpper)) return true

  const keys = queryUpper
    .split('+')
    .map((k) => k.trim())
    .filter((k) => k)
  return command.accelerators.some((a) => {
    const acceleratorKeys = new Set(
      stringToKeys(a.keys).map((k) => k.toUpperCase())
    )
    return keys.every((k) => acceleratorKeys.has(k))
  })
}
