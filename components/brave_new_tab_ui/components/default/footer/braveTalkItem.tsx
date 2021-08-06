// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../../common/locale'
import VisibilityTimer from '../../../helpers/visibilityTimer'
import { IconLink } from '..'
import BraveTalkTooltip from './braveTalkTooltip'
import BraveTalkIcon from './braveTalkTooltip/braveTalkIcon'
import { OnDismissBraveTalkPrompt, Props } from './footer'

function BraveTalkTooltipItem (props: Props) {
  const tooltipRef = React.useRef<HTMLDivElement>(null)
  // Make callback a ref so that timer callback has access to latest value
  const handleDismissPromptRef = React.useRef<null | OnDismissBraveTalkPrompt>(null)
  React.useEffect(() => {
    handleDismissPromptRef.current = props.onDismissBraveTalkPrompt
  }, [props.onDismissBraveTalkPrompt])
  // Set up timer to auto dismiss
  React.useEffect(() => {
    if (!tooltipRef.current) {
      return
    }
    const timer = new VisibilityTimer(() => {
      // Sanity check
      if (!handleDismissPromptRef.current) {
        return
      }
      handleDismissPromptRef.current({ isAutomatic: true })
    }, 4000, tooltipRef.current)
    timer.startTracking()
    return () => timer.stopTracking()
  }, [tooltipRef.current])
  // Regular 'close' handler
  const handleClose = () => {
    props.onDismissBraveTalkPrompt({ isAutomatic: false })
  }
  return (
    <BraveTalkTooltip ref={tooltipRef} onClose={handleClose}>
      <IconLink title={getLocale('braveTalkPromptTitle')} href='https://talk.brave.com/widget'>
        <BraveTalkIcon />
      </IconLink>
    </BraveTalkTooltip>
  )
}

export default function BraveTalkItem (props: Props) {
  if (props.showBraveTalkPrompt) {
    return <BraveTalkTooltipItem {...props} />
  }
  return (
    <IconLink title={getLocale('braveTalkPromptTitle')} href='https://talk.brave.com/widget'>
      <BraveTalkIcon />
    </IconLink>
  )
}
