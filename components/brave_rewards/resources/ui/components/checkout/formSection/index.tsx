/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Box, Header } from './style'

interface FormSectionProps {
  title: React.ReactNode
  children: React.ReactNode
}

export function FormSection (props: FormSectionProps) {
  return (
    <Box>
      <Header>{props.title}</Header>
      {props.children}
    </Box>
  )
}
