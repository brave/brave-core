/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

function InfoIcon () {
  return (
    <svg className='icon' fill='none' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 28 27'>
      <path fillRule='evenodd' clipRule='evenodd' d='M13.9998 26.6667c-7.36376 0-13.333296-5.9696-13.333296-13.3334C.666504 5.96954 6.63604 0 13.9998 0c7.3638 0 13.3334 5.96954 13.3334 13.3333 0 7.3638-5.9696 13.3334-13.3334 13.3334Zm1.0257-7.8975c0 .5665-.4592 1.0257-1.0257 1.0257-.5664 0-1.0256-.4592-1.0256-1.0257v-6.4615c0-.5665.4592-1.0257 1.0256-1.0257.5665 0 1.0257.4592 1.0257 1.0257v6.4615Zm-1.0256-9.30254c.81 0 1.4666-.65665 1.4666-1.46667s-.6566-1.46667-1.4666-1.46667c-.81 0-1.4667.65665-1.4667 1.46667s.6567 1.46667 1.4667 1.46667Z' fill='currentColor'/>
    </svg>
  )
}

function WarnIcon () {
  return (
    <svg className='icon' fill='none' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 28 25'>
      <path fillRule='evenodd' clipRule='evenodd' d='M17.3451 1.9357c-1.486-2.575727-5.2035-2.575726-6.6895 0l-9.46613 16.408c-1.485176 2.5743.37273 5.7911 3.34473 5.7911h18.9323c2.9719 0 4.8299-3.2168 3.3447-5.7911l-9.4661-16.408Zm-3.3449 4.93657c.5665 0 1.0257.4592 1.0257 1.02564v5.12819c0 .5665-.4592 1.0257-1.0257 1.0257-.5664 0-1.0256-.4592-1.0256-1.0257V7.89791c0-.56644.4592-1.02564 1.0256-1.02564ZM15.467 17.4672c0 .81-.6566 1.4666-1.4666 1.4666-.8101 0-1.4667-.6566-1.4667-1.4666 0-.8101.6566-1.4667 1.4667-1.4667.81 0 1.4666.6566 1.4666 1.4667Z' fill='currentColor'/>
    </svg>
  )
}

function ErrorIcon () {
  return (
    <svg className='icon' fill='none' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 28 28'>
      <path fillRule='evenodd' clipRule='evenodd' d='M.666992 13.9993C.666992 6.63555 6.63653.666016 14.0003.666016c7.3638 0 13.3334 5.969534 13.3334 13.333284 0 7.3638-5.9696 13.3334-13.3334 13.3334-7.36377 0-13.333308-5.9696-13.333308-13.3334ZM15.0259 8.56344c0-.56645-.4592-1.02564-1.0256-1.02564-.5665 0-1.0257.45919-1.0257 1.02564v5.12816c0 .5665.4592 1.0257 1.0257 1.0257.5664 0 1.0256-.4592 1.0256-1.0257V8.56344ZM14.0004 19.5993c.81 0 1.4666-.6566 1.4666-1.4666 0-.81-.6566-1.4667-1.4666-1.4667-.8101 0-1.4667.6567-1.4667 1.4667 0 .81.6566 1.4666 1.4667 1.4666Z' fill='currentColor'/>
    </svg>
  )
}

interface Props {
  style: 'info' | 'warn' | 'error'
}

export function InfoBoxIcon (props: Props) {
  switch (props.style) {
    case 'error': return <ErrorIcon />
    case 'warn': return <WarnIcon />
    default: return <InfoIcon />
  }
}
