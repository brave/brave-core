// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as CommandsMojo from 'gen/brave/components/commands/common/commands.mojom.m.js'
import * as React from 'react'
import { render } from 'react-dom'
import styled, { StyleSheetManager } from 'styled-components'
import Command from './components/Command'
import { CommandsCache } from './utils/commandsCache'
import { match } from './utils/match'

import '@brave/leo/tokens/css/variables.css'

import Icon, { setIconBasePath } from '@brave/leo/react/icon'
import { color, font, radius, spacing } from '@brave/leo/tokens/css'
import Button from '@brave/leo/react/button'
setIconBasePath('chrome://resources/brave-icons')

const Container = styled.div`
  padding: ${spacing[24]};
`

const CommandsContainer = styled.div`
  display: flex;
  flex-direction: column;
  font: ${font.desktop.primary.default.regular};
  margin-bottom: ${spacing[24]};
  border: 1px solid ${color.divider.subtle};
  border-radius: ${radius[8]};
`

const SearchContainer = styled.div`
  position: relative;
  display: flex;
  justify-content: stretch;
`

const SearchBox = styled.input`
  border: 1px solid lightgray;
  border-top-left-radius: ${radius[8]};
  border-top-right-radius: ${radius[8]};
  background: ${color.container.highlight};
  color: ${color.text.secondary};
  padding: ${spacing[16]} ${spacing[24]} ${spacing[16]} 44px;
  flex: 1;
`

const SearchIcon = styled(Icon)`
  --leo-icon-size: 16px;
  position: absolute;
  left: ${spacing[16]};
  top: 50%;
  transform: translateY(-50%);
`

export const commandsCache = new CommandsCache()
export function useCommands() {
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

  const commands = useCommands()

  const filteredCommands = React.useMemo(
    () => Object.values(commands)?.filter((c) => match(filter, c)),
    [filter, commands]
  )
  return (
    <Container>
      <CommandsContainer>
        <SearchContainer>
          <SearchIcon name="search" />
          <SearchBox
            placeholder="Search for a command or shortcut"
            value={filter}
            onChange={(e) => {
              setFilter(e.target.value)
            }}
          />
        </SearchContainer>
        {filteredCommands?.map((c) => (
          <Command key={c.id} command={c} />
        ))}
      </CommandsContainer>
      <Button kind='plain-faint' onClick={() => commandsCache.resetAll()}>Reset all to defaults</Button>
    </Container>
  )
}

export const mount = (at: HTMLElement) => {
  render(
    <StyleSheetManager target={at}>
      <App />
    </StyleSheetManager>,
    at
  )
}

(window as any).mountCommands = mount

document.addEventListener('DOMContentLoaded', () => {
  mount(document.getElementById('commandsRoot')!)
})
