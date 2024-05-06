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
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { color, spacing } from '@brave/leo/tokens/css/variables'
import { getLocale } from '$web-common/locale'

const Grid = styled.div`
  display: grid;
  grid-template-columns: 200px auto min-content min-content;
  column-gap: ${spacing.xl};
  align-items: center;
  padding: ${spacing.xl};
  min-height: 68px;

  border-top: 1px solid ${color.divider.subtle};
  border-bottom: 1px solid ${color.divider.subtle};
`

const Column = styled.div`
  display: flex;
  flex-direction: column;
  gap: 4px;
`

const RemoveButton = styled(Button)`
  visibility: hidden;
`

const Row = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 4px;

  &:hover ${RemoveButton} {
    visibility: visible;
  }
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
      {!accelerator.unmodifiable &&
        <RemoveButton
          size="small"
          kind="plain-faint"
          onClick={() =>
            commandsCache.unassignAccelerator(commandId, accelerator.codes)
          }
        >
          <Icon name="remove-circle-outline" />
        </RemoveButton>}
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
      {command.modified && (
        <Button
          size="small"
          kind="plain-faint"
          onClick={() => commandsCache.reset(command.id)}
        >
          {getLocale('shortcutsPageResetCommand')}
        </Button>
      )}
      <Button
        size="small"
        kind="plain"
        disabled={adding}
        onClick={() => setAdding(true)}
      >
        {getLocale('shortcutsPageAddShortcut')}
      </Button>
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
