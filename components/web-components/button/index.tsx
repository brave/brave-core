// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import classnames from '$web-common/classnames'
import { LoaderIcon } from 'brave-ui/components/icons'
import '../app.global.scss'
import style from './button.module.scss'

type Scale = 'tiny' | 'small' | 'regular' | 'large' | 'jumbo'

export interface ButtonProps {
  scale?: Scale
  isPrimary?: boolean
  isTertiary?: boolean
  isLoading?: boolean
  isDisabled?: boolean
  isCallToAction?: boolean
  ariaLabel?: string
  type?: React.ButtonHTMLAttributes<HTMLButtonElement>['type']
  children?: React.ReactNode | React.ReactNode[]
  onClick: () => unknown
}

function scaleToClass (scale: Scale) {
  switch (scale) {
    case 'tiny':
      return style.isTiny
    case 'small':
      return style.isSmall
    case 'large':
      return style.isLarge
    case 'jumbo':
      return style.isJumbo
  }
  return null
}

export function ButtonIconContainer (props: React.PropsWithChildren<{}>) {
  return (
    <div className={style.iconContainer}>{props.children}</div>
  )
}

export default function Button (props: ButtonProps) {
  const { scale = 'regular' } = props
  return (
    <button
      className={classnames(
        style.button,
        scaleToClass(scale),
        {
          [style.isPrimary]: props.isPrimary,
          [style.isTertiary]: props.isTertiary && !props.isPrimary,
          [style.isLoading]: props.isLoading,
          [style.isCallToAction]: props.isCallToAction
        }
      )}
      disabled={props.isDisabled}
      aria-label={props.ariaLabel}
      onClick={props.onClick}
    >
      <div className={style.content}>{props.children}</div>
      { props.isLoading && <div aria-label={'Loading'} className={style.loadingIcon}><LoaderIcon /></div>}
    </button>
  )
}
