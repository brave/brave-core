/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Container, LoadIcon, Text } from './style'

interface LoadingPanelProps {
  text?: string
}

export function LoadingPanel (props: LoadingPanelProps) {
  return (
    <Container>
      <LoadIcon />
      {props.text ? <Text>{props.text}</Text> : null}
    </Container>
  )
}
