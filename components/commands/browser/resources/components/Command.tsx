// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import type * as CommandsMojo from 'gen/brave/components/commands/common/commands.mojom.m.js'
import styled from 'styled-components'
import Keys from './Keys'
import ConfigureShortcut from './ConfigureShortcut'
import { commandsCache } from '../commands'
import { stringToKeys } from '../utils/accelerator'

const Grid = styled.div`
  display: grid;
  grid-template-columns: 200px auto min-content min-content;
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

const Row = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 4px;
`

function Accelerator({
  commandId,
  accelerator
}: {
  commandId: number
  accelerator: CommandsMojo.Accelerator
}) {
  return (
    <Row>
      <Keys keys={stringToKeys(accelerator.keys)} />
      <button
        onClick={() => commandsCache.unassignAccelerator(commandId, accelerator.codes)}
      >
        -
      </button>
    </Row>
  )
}

export default function Command({
  command
}: {
  command: CommandsMojo.Command
}) {
  const [adding, setAdding] = React.useState(false)

  return (
    <Grid>
      <div>{command.name}</div>
      <Column>
        {command.accelerators.map((a, i) => (
          <Accelerator key={i} commandId={command.id} accelerator={a} />
        ))}
      </Column>
      <button disabled={adding} onClick={() => setAdding(true)}>
        Add
      </button>
      <button onClick={() => commandsCache.reset(command.id)}>Reset</button>
      {adding && (
        <ConfigureShortcut
          onChange={(info) => {
            setAdding(false)
            commandsCache.assignAccelerator(command.id, info)
          }}
          onCancel={() => {
            setAdding(false)
          }}
        />
      )}
    </Grid>
  )
}
