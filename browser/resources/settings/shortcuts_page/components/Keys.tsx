// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled, { css } from 'styled-components'
import { color, font, radius, spacing } from '@brave/leo/tokens/css'

const Kbd = styled.div<{ large?: boolean; square?: boolean }>`
  display: inline-block;

  font-family: ${font.components.label};
  min-width: 12px;

  border: 1px solid ${color.divider.subtle};
  border-radius: ${radius[12]};
  padding: 6px 10px;
  background: linear-gradient(180deg, #f6f7f9 0%, #ffffff 100%);
  box-shadow: 0px 1px 0px rgba(0, 0, 0, 0.05), inset 0px 1px 0px #ffffff;
  color: ${color.text.tertiary};
  text-transform: capitalize;
  text-shadow: 0px 1px 0px #ffffff;

  display: flex;
  align-items: center;
  justify-content: center;

  ${(props) =>
    props.square &&
    css`
      aspect-ratio: 1;
    `}

  ${(props) =>
    props.large &&
    css`
      min-width: 32px;
      border-radius: ${radius[12]};
      padding: ${spacing[16]};
      font: var(--leo-font-heading-h3);
      box-shadow: 0px 2px 0px rgba(0, 0, 0, 0.05), inset 0px 1px 0px #ffffff;
      background: linear-gradient(180deg, #f4f6f8 0%, #ffffff 100%);
    `}
`

const Padded = styled.span`
  margin-right: 4px;
`

const keySymbols = {
  'Shift': <Padded>⇧</Padded>,
  'Meta': <Padded>⌘</Padded>,
  'Alt': <Padded>⌥</Padded>,
  'AltGr': <Padded>⌥</Padded>,
  'Control': <Padded>⌃</Padded>
}

export default function Keys({
  keys,
  large
}: {
  keys: string[]
  large?: boolean
}) {
  return (
    <>
      {keys.map((k, i) => (
        <Kbd key={i} large={large} square={k.length <= 2}>
          {keySymbols[k]}
          {k}
        </Kbd>
      ))}
    </>
  )
}
