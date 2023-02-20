// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as CommandsMojo from 'gen/brave/components/commands/common/commands.mojom.m.js'
import * as React from 'react'
import { render } from 'react-dom'
import styled from 'styled-components'
import Command from './components/Command'
import { CommandsCache } from './utils/commandsCache'
import { match } from './utils/match'

const CommandsContainer = styled.div`
  display: flex;
  flex-direction: column;
  gap: 4px;
`

const FilterBox = styled.input`
  margin: 4px;
  padding: 4px;
  border: 1px solid lightgray;
`

const FiltersRow = styled.div`
  display: flex;
  flex-direction: row;
  gap: 8px;
`

export const commandsCache = new CommandsCache()
function useCommands() {
  const [commands, setCommands] = React.useState<{
    [id: number]: CommandsMojo.Command
  }>({})
  React.useEffect(() => {
    const listener = (l: { [command: number]: CommandsMojo.Command }) => {
      setCommands(l)
    }
    commandsCache.addListener(listener)
    return () => commandsCache.removeListener(listener)
  }, [])
  return commands
}

function App() {
  const [filter, setFilter] = React.useState('')
  const [withAccelerator, setWithAccelerator] = React.useState(false)

  const commands = useCommands()

  const filteredCommands = React.useMemo(
    () =>
      Object.values(commands)
        ?.filter((c) => match(filter, c))
        .filter((c) => !withAccelerator || c.accelerators.length),
    [filter, withAccelerator, commands]
  )
  return (
    <CommandsContainer>
      <FilterBox
        value={filter}
        onChange={(e) => {
          setFilter(e.target.value)
        }}
      />
      <FiltersRow>
        <label>
          <input
            type="checkbox"
            checked={withAccelerator}
            onChange={(e) => {
              setWithAccelerator(e.target.checked)
            }}
          />
          Only with accelerator
        </label>
      </FiltersRow>
      {filteredCommands?.map((c) => (
        <Command key={c.id} command={c} />
      ))}
    </CommandsContainer>
  )
}

document.addEventListener('DOMContentLoaded', () => {
  render(<App />, document.getElementById('root'))
})
