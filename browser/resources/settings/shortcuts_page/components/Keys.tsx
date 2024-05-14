// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled, { css } from 'styled-components'
import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'

const Kbd = styled.div<{ large?: boolean; square?: boolean }>`
  display: inline-block;

  font: ${font.components.label};
  font-size: 12px;
  line-height: 18px;

  border: 1px solid ${color.divider.subtle};
  border-radius: ${radius.m};
  padding: 4px 10px;
  background: linear-gradient(180deg, #F6F7F9 0%, #FFFFFF 100%);
  box-shadow: 0px 1px 0px rgba(0, 0, 0, 0.05), inset 0px 1px 0px #FFFFFF;
  color: ${color.text.tertiary};
  text-transform: capitalize;
  text-shadow: 0px 1px 0px ${color.white};

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
      border-radius: ${radius.l};
      padding: ${spacing.xl};
      font: ${font.heading.h3};
      box-shadow: 0px 2px 0px rgba(0, 0, 0, 0.05), inset 0px 1px 0px #ffffff;
      background: linear-gradient(180deg, #f4f6f8 0%, #ffffff 100%);
    `}

    @media (prefers-color-scheme: dark) {
      text-shadow: 0px 1px 0px ${color.black};
      background: linear-gradient(180deg, #0A0D10 0%, #1A1C20 100%);
      box-shadow: 0px 1px 0px rgba(0, 0, 0, 0.05), inset 0px 1px 0px rgba(255, 255, 255, 0.2);
    }
`

const Padded = styled.span`
  margin-right: 4px;
`

const keySymbols = {
  'Shift': <Padded>⇧</Padded>,
  'Meta': <Padded>⌘</Padded>,
  'Alt': <Padded>⌥</Padded>,
  'AltGr': <Padded>⌥</Padded>,
  'Control': <Padded>⌃</Padded>,
  ' ': 'Space'
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
        <Kbd key={i} large={large} square={k.length <= 2 && k !== ' '}>
          {keySymbols[k]}
          {k}
        </Kbd>
      ))}
    </>
  )
}
