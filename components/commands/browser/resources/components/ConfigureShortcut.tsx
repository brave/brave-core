// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled from 'styled-components'
import Keys from './Keys'
import { keysToString, stringToKeys } from '../utils/accelerator'
import { color, effect, font, radius, spacing } from '@brave/leo/tokens/css'
import Button from '@brave/leo/react/button'
import Alert from '@brave/leo/react/alert'
import { useCommands } from '../commands'

const Dialog = styled.dialog`
  border: none;
  border-radius: ${radius[16]};

  ::backdrop {
    backdrop-filter: blur(2px);
  }
`

const Container = styled.div`
  background: ${color.white};
  width: 420px;
  min-height: 252px;
  margin: auto;

  display: flex;
  flex-direction: column;
  align-items: stretch;
  justify-content: center;

  box-shadow: ${effect.elevation[5]};
`

const KeysContainer = styled.div`
  align-self: stretch;
  flex: 1;

  display: flex;
  justify-content: center;
  align-items: center;

  gap: ${spacing[8]};
  margin-top: ${spacing[32]};
`

const ActionsContainer = styled.div`
  display: flex;
  flex-direction: row;
  justify-content: stretch;
  gap: ${spacing[8]};
  margin: 0 ${spacing[24]} ${spacing[32]} ${spacing[24]};

  > * {
    flex: 1;
  }
`

const HintText = styled.div`
  text-align: center;
  margin: ${spacing[40]};
  color: ${color.text.tertiary};
  font: ${font.desktop.primary.default.regular};
`

const InUseAlert = styled(Alert)`
  margin: ${spacing[24]};
`

const modifiers = ['Control', 'Alt', 'Shift', 'Meta']

class AcceleratorInfo {
  codes: string[] = []
  keys: string[] = []

  add(e: KeyboardEvent) {
    if (e.ctrlKey) {
      this.codes.push('Control')
      this.keys.push('Control')
    }
    if (e.altKey) {
      this.codes.push('Alt')
      this.keys.push('Alt')
    }

    if (e.shiftKey) {
      this.codes.push('Shift')
      this.keys.push('Shift')
    }

    if (e.metaKey) {
      this.codes.push('Meta')
      this.keys.push('Meta')
    }

    if (!modifiers.includes(e.key)) {
      this.codes.push(e.code)
      this.keys.push(e.key)
    }

    this.keys = Array.from(new Set(this.keys))
    this.codes = Array.from(new Set(this.codes))
  }

  isValid() {
    // There needs to be at least one non-modifier key
    return this.keys.some((k) => !modifiers.includes(k))
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

  const dialogRef = React.useRef<HTMLDialogElement>()
  React.useEffect(() => {
    dialogRef.current?.showModal()
  }, [])

  const conflict = acceleratorLookup[maxKeys.current.codes.join('+')]
  return (
    <Dialog ref={dialogRef as any}>
      <Container>
        <KeysContainer>
          {keys.length ? (
            <Keys keys={keys} large />
          ) : (
            <HintText>
              Create a new shortcut. Press the desired keys to create a new
              binding
            </HintText>
          )}
        </KeysContainer>
        {conflict && (
          <InUseAlert>
            This combination is being used for{' '}
            <b>"{commands[conflict].name}"</b>. Saving will override that
            shortcut.
          </InUseAlert>
        )}
        <ActionsContainer>
          <Button
            size="large"
            kind="plain-faint"
            onClick={() => {
              setCurrentKeys([])
              maxKeys.current = new AcceleratorInfo()
              props.onCancel?.()
            }}
          >
            Cancel
          </Button>
          <Button
            size="large"
            kind="filled"
            disabled={!maxKeys.current.isValid()}
            onClick={() => {
              props.onChange({
                codes: keysToString(maxKeys.current.codes),
                keys: keysToString(maxKeys.current.keys)
              })
            }}
          >
            Save
          </Button>
        </ActionsContainer>
      </Container>
    </Dialog>
  )
}
