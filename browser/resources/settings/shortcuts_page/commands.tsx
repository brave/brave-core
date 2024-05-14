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
import { getLocale } from '$web-common/locale'

import Icon, { setIconBasePath } from '@brave/leo/react/icon'
import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
setIconBasePath('chrome://resources/brave-icons')

const Container = styled.div`
  padding: ${spacing['2Xl']};
`

const CommandsContainer = styled.div`
  display: flex;
  flex-direction: column;
  font: ${font.default.regular};
  margin-bottom: ${spacing['2Xl']};
  border: 1px solid ${color.divider.subtle};
  border-radius: ${radius.m};
`

const SearchContainer = styled.div`
  position: relative;
  display: flex;
  justify-content: stretch;
`

const SearchBox = styled.input`
  all: unset;

  border: 1px solid ${color.divider.subtle};
  border-top-left-radius: ${radius.m};
  border-top-right-radius: ${radius.m};
  background: ${color.container.highlight};
  color: ${color.text.secondary};
  padding: ${spacing.xl} ${spacing['2Xl']} ${spacing.xl} 44px;
  flex: 1;

  &:hover {
    border: 1px solid ${color.divider.strong};
  }

  &:focus-visible {
    box-shadow: 0px 0px 0px 1.5px rgba(255, 255, 255, 0.5),
    0px 0px 4px 2px #423eee;
  }
`

const SearchIcon = styled(Icon)`
  --leo-icon-size: 16px;
  position: absolute;
  left: ${spacing.xl};
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
            placeholder={getLocale('shortcutsPageSearchPlaceholder')}
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
      <Button kind="plain-faint" onClick={() => commandsCache.resetAll()}>
        {getLocale('shortcutsPageResetAll')}
      </Button>
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

;(window as any).mountCommands = mount
