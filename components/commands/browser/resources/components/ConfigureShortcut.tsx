// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled from 'styled-components'
import Keys from './Keys'
import { keysToString, stringToKeys } from '../utils/accelerator'

const Container = styled.div`
  background: gray;
  border-radius: 50px;
  width: 200px;
  height: 200px;
  margin: auto;

  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

const modifiers = ['Control', 'Alt', 'Shift', 'Meta']

class AcceleratorInfo {
  codes: string[] = []
  keys: string[] = []

  add(e: KeyboardEvent) {
    if (modifiers.includes(e.key)) {
      this.codes.push(e.key)
    } else {
      this.codes.push(e.code)
    }

    this.keys.push(e.key)
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
  return (
    <dialog ref={dialogRef as any}>
      <Container>
        <div>
          <Keys keys={keys} />
        </div>
        <div>
          <button
            disabled={!maxKeys.current.isValid()}
            onClick={() => {
              props.onChange({
                codes: keysToString(maxKeys.current.codes),
                keys: keysToString(maxKeys.current.keys)
              })
            }}
          >
            Accept
          </button>
          <button
            onClick={() => {
              setCurrentKeys([])
              maxKeys.current = new AcceleratorInfo()
              props.onCancel?.()
            }}
          >
            Cancel
          </button>
        </div>
      </Container>
    </dialog>
  )
}
