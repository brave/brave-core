/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { style } from './text_skeleton.style'

interface Props {
  length: number
}

export function TextSkeleton(props: Props) {
  return (
    <span {...style}>{'\u2002'.repeat(props.length)}</span>
  )
}
