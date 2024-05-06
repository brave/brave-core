// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import Keys from './Keys'
import { keysToString } from '../utils/accelerator'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
import Alert from '@brave/leo/react/alert'
import { useCommands } from '../commands'
import Dialog from '@brave/leo/react/dialog'
import { getLocale } from '$web-common/locale'
import { Accelerator } from 'gen/brave/components/commands/common/commands.mojom.m'
import { commandsCache } from '../commands'

const StyledDialog = styled(Dialog)`
  --leo-dialog-width: 402px;
`

const KeysContainer = styled.div`
  align-self: stretch;
  flex: 1;

  display: flex;
  flex-wrap: wrap;
  justify-content: center;
  align-items: center;

  gap: ${spacing.m};
  margin-top: ${spacing['3Xl']};
`

const HintText = styled.div`
  text-align: center;
  margin: ${spacing['4Xl']};
  color: ${color.text.tertiary};
  font: ${font.default.regular};
`

const InUseAlert = styled(Alert)`
  margin-top: ${spacing['2Xl']};
  display: block;
`

const modifiers = ['Control', 'Alt', 'Shift', 'Meta', 'Command']

// Some keys are rendered differently on different platforms. For example Meta
// is Meta on Windows & Linux but Command on OSX.
// Note: This list of transforms should match what is returned by
// GetAllModifierNames in accelerator_parsing.
const keyTransforms = {
  'Meta': navigator.platform?.includes('Mac') && 'Command'
}
const getKey = (key: string) => keyTransforms[key] || key

const keyCodeCache: { [code: string]: string } = {}
const getKeyFromCode = (code: string) => {
  if (modifiers.includes(code)) return code
  if (keyCodeCache[code]) return keyCodeCache[code]

  return commandsCache.getKeyFromCode(code).then(key => keyCodeCache[code] = key)
}
export const useKeys = (codes: string[]) => {
  const [keys, setKeys] = React.useState<string[]>([])
  React.useEffect(() => {
    let cancelled = false
    const keys = codes.map(getKeyFromCode)
    Promise.all(keys).then(keys => !cancelled && setKeys(keys))

    // Eagerly render the keys we already know.
    setKeys(keys.map(key => typeof key === 'string' ? key : ''))

    return () => {
      cancelled = true
    }
  }, [codes])
  return keys
}

export default function ConfigureShortcut(props: {
  onChange: (info: { codes: string; keys: string }) => void
  onCancel?: () => void
}) {
  const [current, setCurrent] = React.useState<{ codes: string[], isValid: boolean }>({ codes: [], isValid: false })
  const commands = useCommands()
  const acceleratorLookup = React.useMemo(() => {
    return Object.values(commands)
      .flatMap((c) => c.accelerators.map((a) => [c.id, a.codes]))
      .reduce((prev, next) => ({ ...prev, [next[1]]: next[0] }), {})
  }, [commands])

  const keys = useKeys(current.codes)

  React.useEffect(() => {
    const onDown = (e: KeyboardEvent) => {
      const hasModifier = e.ctrlKey || e.metaKey || e.altKey || e.shiftKey

      // Enter on it's own is used to accept the current combination, if the
      // current keys are a valid shortcut.
      if (e.code === "Enter" && !hasModifier && current.isValid) {
        props.onChange({
          codes: keysToString(current.codes),
          keys: keysToString(keys)
        })
        return;
      }

      // Escape on it's own cancels the change
      if (e.code === "Escape" && !hasModifier) {
        props.onCancel?.()
        return;
      }

      e.preventDefault()

      const modifiers: string[] = []
      let keyCode: string | undefined

      if (e.ctrlKey) {
        modifiers.push(getKey('Control'))
      }
      if (e.altKey) {
        modifiers.push(getKey('Alt'))
      }

      if (e.shiftKey) {
        modifiers.push(getKey('Shift'))
      }

      if (e.metaKey) {
        modifiers.push(getKey('Meta'))
      }

      if (!modifiers.includes(e.key)) {
        keyCode = e.code
      }

      const codes = [...modifiers, keyCode!].filter(c => c)
      setCurrent({ codes, isValid: !!keyCode })
    }

    document.addEventListener('keydown', onDown)
    return () => {
      document.removeEventListener('keydown', onDown)
    }
  }, [current])

  const shortcut = current.codes.join('+');
  const conflict = acceleratorLookup[shortcut]
  // A shortcut cannot be reused if the existing shortcut is unmodifiable.
  const unmodifiable = commands[conflict]?.accelerators.some((a: Accelerator) => a.codes === shortcut && a.unmodifiable)
  const conflictMessage = React.useMemo(() => {
    if (!conflict) return null
    const messageParts = getLocale(unmodifiable ? 'shortcutsPageShortcutUnmodifiable' : 'shortcutsPageShortcutInUse').split('$1')
    return <>
      {messageParts[0]}
      <b>"{commands[conflict].name}"</b>
      {messageParts[1]}
    </>
  }, [conflict, commands, unmodifiable])

  return (
    <StyledDialog isOpen onClose={props.onCancel}>
      <KeysContainer>
        {keys.length ? (
          <Keys keys={keys} large />
        ) : (
          <HintText>
            {getLocale('shortcutsPageShortcutHint')}
          </HintText>
        )}
      </KeysContainer>
      {conflict && (
        <InUseAlert>
          {conflictMessage}
        </InUseAlert>
      )}
      <div slot='actions'>
        <Button
          size="large"
          kind="plain-faint"
          onClick={() => {
            setCurrent({ codes: [], isValid: false })
            props.onCancel?.()
          }}
        >
          {getLocale('shortcutsPageCancelAddShortcut')}
        </Button>
        <Button
          size="large"
          kind="filled"
          isDisabled={!current.isValid || unmodifiable}
          onClick={() => {
            props.onChange({
              codes: keysToString(current.codes),
              keys: keysToString(keys)
            })
          }}
        >
          {getLocale('shortcutsPageSaveAddShortcut')}
        </Button>
      </div>
    </StyledDialog>
  )
}
