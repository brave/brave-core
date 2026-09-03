// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Input from '@brave/leo/react/input'

// Leo Input keeps the native <input> in shadow DOM, so page-level CSS cannot
// hide number spinners. Inject the rules into each host's shadow root instead.
const HIDE_SPINNERS_STYLE_ID = 'hide-number-spinners'
const HIDE_SPINNERS_CSS = `
  .leo-input[type='number'] {
    -moz-appearance: textfield;
    appearance: textfield;
  }
  .leo-input::-webkit-inner-spin-button,
  .leo-input::-webkit-outer-spin-button {
    -webkit-appearance: none;
    appearance: none;
    margin: 0;
    display: none;
  }
`

function hideNumberSpinners(host: HTMLElement | null) {
  if (
    !host?.shadowRoot
    || host.shadowRoot.querySelector(`#${HIDE_SPINNERS_STYLE_ID}`)
  ) {
    return
  }
  const style = document.createElement('style')
  style.id = HIDE_SPINNERS_STYLE_ID
  style.textContent = HIDE_SPINNERS_CSS
  host.shadowRoot.appendChild(style)
}

/**
 * Props for {@link NumberInput}.
 *
 * Note: `type` is fixed to `'number'` internally. Consumers should not pass `type`.
 */
export type NumberInputProps = Omit<React.ComponentProps<typeof Input>, 'type'>

// Wrap rather than use styled(Input).attrs(): styled-components v6's type
// machinery explodes over leo Input's large prop type (TS2590). Cast on
// createElement for the same reason (ref + spread also trips TS2590/TS2769).
/**
 * Leo `<input type="number">` without native spinner controls.
 */
export const NumberInput = (props: NumberInputProps) => {
  const setRef = React.useCallback((node: HTMLElement | null) => {
    hideNumberSpinners(node)
  }, [])

  return React.createElement(Input, {
    ...props,
    type: 'number',
    ref: setRef,
  } as any)
}

export default NumberInput
