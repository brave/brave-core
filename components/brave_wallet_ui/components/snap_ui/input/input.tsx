// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Input from '@brave/leo/react/input'

import type { SnapUiInputProps } from '../snap_ui_sdk_props'
import type { SnapUiOnInput } from '../snap_ui_wallet_renderer.types'

function snapInputHtmlType(
  props: SnapUiInputProps,
): 'text' | 'password' | 'number' {
  if ('type' in props && props.type === 'password') {
    return 'password'
  }
  if ('type' in props && props.type === 'number') {
    return 'number'
  }
  return 'text'
}

function numberFieldAttrs(
  props: SnapUiInputProps,
): { min?: number; max?: number; step?: number } | undefined {
  if ('type' in props && props.type === 'number') {
    return {
      min: props.min,
      max: props.max,
      step: props.step,
    }
  }
  return undefined
}

const SnapUiLeoInput: React.FC<{
  snapProps: SnapUiInputProps
  onSnapInput?: SnapUiOnInput
}> = ({ snapProps, onSnapInput }) => {
  const initial = snapProps.value ?? ''
  const [value, setValue] = React.useState(() => String(initial))

  React.useEffect(() => {
    setValue(String(snapProps.value ?? ''))
  }, [snapProps.value])

  const htmlType = snapInputHtmlType(snapProps)
  const nums = numberFieldAttrs(snapProps)
  const label = snapProps.placeholder ?? snapProps.name

  return (
    <Input
      type={htmlType}
      value={value}
      placeholder={snapProps.placeholder}
      disabled={snapProps.disabled === true}
      min={nums?.min}
      max={nums?.max}
      step={nums?.step}
      onInput={(detail) => {
        const v = detail.value
        setValue(String(v))
        onSnapInput?.({ name: snapProps.name, value: String(v) })
      }}
    >
      {label}
    </Input>
  )
}

export function renderSnapUiInput(
  props: SnapUiInputProps,
  reactKey: string,
  onSnapInput?: SnapUiOnInput,
): React.ReactNode {
  return (
    <SnapUiLeoInput
      key={reactKey}
      snapProps={props}
      onSnapInput={onSnapInput}
    />
  )
}
