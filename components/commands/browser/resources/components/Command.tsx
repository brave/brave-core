// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import type * as CommandsMojo from 'gen/brave/components/commands/common/commands.mojom.m.js'
import styled from 'styled-components'
import { api } from '../commands'
import { allKeys } from '../utils/accelerator'

const Grid = styled.div`
  display: grid;
  grid-template-columns: 200px min-content auto;
  gap: 16px;
  align-items: center;
  padding: 4px;
  min-height: 32px;

  &:hover {
    background-color: lightgray;
  }
`

const Column = styled.div`
  display: flex;
  flex-direction: column;
  gap: 4px;
`

const Kbd = styled.div`
  display: inline-block;
  border-radius: 4px;
  padding: 4px;
  background-color: #f6f8fa;
  border: 1px solid rgba(174, 184, 193, 0.2);
  box-shadow: inset 0 -1px 0 rgba(174, 184, 193, 0.2);
`

let isSure = false
const ifSure = () => {
  if (isSure) return true
  return isSure = window.confirm('This is experimental. Executing commands may not behave as expected, or cause your browser to crash. Continue?')
}

function Accelerator ({
  accelerator
}: {
  accelerator: CommandsMojo.Accelerator
}) {
  return (
    <div>
      {allKeys(accelerator).map((k, i) => (
        <React.Fragment key={i}>
          {i !== 0 && <span>+</span>}
          <Kbd>{k}</Kbd>
        </React.Fragment>
      ))}
    </div>
  )
}

export default function Command ({
  command
}: {
  command: CommandsMojo.Command
}) {
  return (
    <Grid>
      <div>{command.name}</div>
      <button onClick={() => { ifSure() && api.tryExecuteCommand(command.id) }}>Execute</button>
      <Column>
        {command.accelerators.map((a, i) => (
          <Accelerator key={i} accelerator={a} />
        ))}
      </Column>
    </Grid>
  )
}
