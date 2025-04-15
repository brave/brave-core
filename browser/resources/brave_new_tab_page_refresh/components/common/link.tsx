/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { sanitizeExternalURL } from '../../lib/url_sanitizer'

interface Props {
  url: string
  className?: string
  onClick?: () => void
  openInNewTab?: boolean
  children: React.ReactNode
}

export function Link(props: Props) {
  const sanitizedURL = sanitizeExternalURL(props.url)
  if (!sanitizedURL) {
    return (
      <span className={props.className}>
        {props.children}
      </span>
    )
  }
  return (
    <a
      href={sanitizedURL}
      className={props.className}
      rel='noopener noreferrer'
      target={props.openInNewTab ? '_blank' : '_self'}
      onClick={props.onClick}
    >
      {props.children}
    </a>
  )
}

export function openLink(url: string) {
  const sanitizedURL = sanitizeExternalURL(url)
  if (sanitizedURL) {
    window.open(sanitizedURL, '_self', 'noopener noreferrer')
  }
}
