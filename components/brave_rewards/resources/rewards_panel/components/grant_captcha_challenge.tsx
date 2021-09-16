/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { GrantCaptchaInfo } from '../lib/interfaces'
import { LoadingIcon } from './icons/loading_icon'
import { ArrowDownIcon } from './icons/arrow_down_icon'

import * as style from './grant_captcha_challenge.style'

import dragIconURL from '../assets/grant_captcha_icon.png'

interface Point {
  x: number
  y: number
}

function applyDevicePixelTransform (point: Point): Point {
  if (window.navigator.platform !== 'Win32') {
    return point
  }

  const { devicePixelRatio } = window
  if (devicePixelRatio <= 1) {
    return point
  }

  // TODO(zenparsing): It's unclear how this works - if the ratio is 2 on
  // Windows, why would we offset the answer by 10 pixels left and down? Revisit
  // this logic and add comments explaining the strategy.
  const offset = 5 * devicePixelRatio

  return {
    x: point.x + offset,
    y: point.y + offset
  }
}

interface Props {
  grantCaptchaInfo: GrantCaptchaInfo
  onSolve: (solution: Point) => void
}

export function GrantCaptchaChallenge (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [centerOffset, setCenterOffset] = React.useState<Point | null>(null)

  const { grantCaptchaInfo } = props

  function onDragStart (event: React.DragEvent) {
    const target = event.target as HTMLElement
    const rect = target.getBoundingClientRect()

    // Record the offset from the user's mouse position to the center of the
    // draggable object.
    setCenterOffset({
      x: (rect.width / 2) - (event.clientX - rect.left),
      y: (rect.height / 2) - (event.clientY - rect.top)
    })
  }

  function onDragOver (event: React.DragEvent) {
    event.preventDefault()
  }

  function onDrop (event: React.DragEvent) {
    event.preventDefault()

    if (!centerOffset) {
      return
    }

    const target = event.target as HTMLElement
    const rect = target.getBoundingClientRect()

    // The solution is the center point of the draggable object within the
    // space coordinates of the captcha image.
    props.onSolve(applyDevicePixelTransform({
      x: event.clientX - rect.left + centerOffset.x,
      y: event.clientY - rect.top + centerOffset.y
    }))
  }

  if (!grantCaptchaInfo.imageURL) {
    return (
      <style.root>
        <style.loading>
          <LoadingIcon />
        </style.loading>
      </style.root>
    )
  }

  return (
    <style.root>
      <style.hint>
          {
            formatMessage(getString('grantCaptchaHint'), [
              <strong key='hint'>{grantCaptchaInfo.hint}</strong>
            ])
          }
      </style.hint>
      <style.dragObject>
        <style.dragObjectCircle>
          <img
            src={dragIconURL}
            draggable={true}
            onDragStart={onDragStart}
            data-test-id='grant-captcha-object'
          />
        </style.dragObjectCircle>
      </style.dragObject>
      <style.arrow>
        <ArrowDownIcon />
      </style.arrow>
      <style.dragTarget>
        <img
          src={grantCaptchaInfo.imageURL}
          onDragOver={onDragOver}
          onDrop={onDrop}
          data-test-id='grant-captcha-target'
        />
      </style.dragTarget>
    </style.root>
  )
}
