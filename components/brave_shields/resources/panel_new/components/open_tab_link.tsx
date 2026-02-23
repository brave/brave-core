/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useShieldsApi } from '../api/shields_api_context'

interface Props {
  url: string
  className?: string
  children: React.ReactNode
}

export function OpenTabLink(props: Props) {
  const api = useShieldsApi()
  return (
    <a
      href={sanitize(props.url)}
      className={props.className}
      rel='noopener noreferrer'
      target='_blank'
      onClick={(event) => {
        event.preventDefault()
        api.openTab(sanitize(props.url))
      }}
    >
      {props.children}
    </a>
  )
}

function isAllowed(url: string) {
  try {
    return ['https:', 'chrome:'].includes(new URL(url).protocol)
  } catch {
    return false
  }
}

function sanitize(url: string) {
  return isAllowed(url) ? url : ''
}
