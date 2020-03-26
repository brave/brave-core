/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Container, SadIcon, Text, Details } from './style'

interface ErrorPanelProps {
  text: string
  details: string
}

export function ErrorPanel (props: ErrorPanelProps) {
  return (
    <Container>
      <SadIcon />
      <Text>{props.text}</Text>
      <Details>{props.details}</Details>
    </Container>
  )
}
