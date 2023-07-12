// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import Keys from './Keys'
import { keysToString, stringToKeys } from '../utils/accelerator'
import { color, font, spacing } from '@brave/leo/tokens/css'
import Button from '@brave/leo/react/button'
import Alert from '@brave/leo/react/alert'
import { useCommands } from '../commands'
import Dialog from '@brave/leo/react/dialog'
import { getLocale } from '$web-common/locale'
import { Accelerator } from 'gen/brave/components/commands/common/commands.mojom.m'

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

  gap: ${spacing[8]};
  margin-top: ${spacing[32]};
`

const HintText = styled.div`
  text-align: center;
  margin: ${spacing[40]};
  color: ${color.text.tertiary};
  font: ${font.primary.default.regular};
`

const InUseAlert = styled(Alert)`
  margin-top: ${spacing[24]};
  display: block;
`

const modifiers = ['Control', 'Alt', 'Shift', 'Meta']

// Some keys are rendered differently on different platforms. For example Meta
// is Meta on Windows & Linux but Command on OSX.
// Note: This list of transforms should match what is returned by
// GetAllModifierNames in accelerator_parsing.
const keyTransforms = {
  'Meta': navigator.platform?.includes('Mac') && 'Command'
}
const getKey = (key: string) => keyTransforms[key] || key

class AcceleratorInfo {
  get codes() {
    return [...this.#modifiers, this.#key?.code!].filter(k => k)
  }

  get keys() {
    return [...this.#modifiers, this.#key?.key!].filter(k => k)
  }

  #key?: { code: string, key: string }
  #modifiers: string[] = []

  add(e: KeyboardEvent) {
    if (e.ctrlKey) {
      this.#modifiers.push(getKey('Control'))
    }
    if (e.altKey) {
      this.#modifiers.push(getKey('Alt'))
    }

    if (e.shiftKey) {
      this.#modifiers.push(getKey('Shift'))
    }

    if (e.metaKey) {
      this.#modifiers.push(getKey('Meta'))
    }

    if (!modifiers.includes(e.key)) {
      this.#key = {
        key: e.key,
        code: e.code
      }
    }

    this.#modifiers = Array.from(new Set(this.#modifiers))
  }

  isValid() {
    // There needs to be at least one non-modifier key
    return !!this.#key
  }
}

export default function ConfigureShortcut(props: {
  value?: string
  onChange: (info: { codes: string; keys: string }) => void
  onCancel?: () => void
}) {
  const [, setCurrentKeys] = React.useState<string[]>([])
  const maxKeys = React.useRef<AcceleratorInfo>(new AcceleratorInfo())
  const commands = useCommands()
  const acceleratorLookup = React.useMemo(() => {
    return Object.values(commands)
      .flatMap((c) => c.accelerators.map((a) => [c.id, a.codes]))
      .reduce((prev, next) => ({ ...prev, [next[1]]: next[0] }), {})
  }, [commands])

  React.useEffect(() => {
    const onDown = (e: KeyboardEvent) => {
      const hasModifier = e.ctrlKey || e.metaKey || e.altKey || e.shiftKey

      // Enter on it's own is used to accept the current combination, if the
      // current keys are a valid shortcut.
      if (e.code === "Enter" && !hasModifier && maxKeys.current.isValid()) {
        props.onChange({
          codes: keysToString(maxKeys.current.codes),
          keys: keysToString(maxKeys.current.keys)
        })
        return;
      }

      // Escape on it's own cancels the change
      if (e.code === "Escape" && !hasModifier) {
        props.onCancel?.()
        return;
      }

      e.preventDefault()
      setCurrentKeys((keys) => {
        if (keys.length === 0) {
          maxKeys.current = new AcceleratorInfo()
        }

        const newKeys = Array.from(new Set([...keys, e.key]))
        maxKeys.current.add(e)
        return newKeys
      })
    }

    const onUp = (e: KeyboardEvent) => {
      e.preventDefault()
      setCurrentKeys((keys) => {
        const newKeys = keys.filter((k) => k !== e.key)
        return newKeys
      })
    }
    document.addEventListener('keyup', onUp)
    document.addEventListener('keydown', onDown)
    return () => {
      document.removeEventListener('keyup', onUp)
      document.removeEventListener('keydown', onDown)
    }
  })

  const keys = maxKeys.current.keys.length
    ? maxKeys.current.keys
    : stringToKeys(props.value ?? '')

  const shortcut = maxKeys.current.codes.join('+');
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
            setCurrentKeys([])
            maxKeys.current = new AcceleratorInfo()
            props.onCancel?.()
          }}
        >
          {getLocale('shortcutsPageCancelAddShortcut')}
        </Button>
        <Button
          size="large"
          kind="filled"
          isDisabled={!maxKeys.current.isValid() || unmodifiable}
          onClick={() => {
            props.onChange({
              codes: keysToString(maxKeys.current.codes),
              keys: keysToString(maxKeys.current.keys)
            })
          }}
        >
          {getLocale('shortcutsPageSaveAddShortcut')}
        </Button>
      </div>
    </StyledDialog>
  )
}
