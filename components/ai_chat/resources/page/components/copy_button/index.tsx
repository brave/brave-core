/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import styles from './style.module.scss'
import Button from '@brave/leo/react/button'
import Tooltip from '@brave/leo/react/tooltip'
import classnames from '$web-common/classnames'

interface Props {
  onClick?: () => void
}

function CopyButton(props: Props) {
  const [isActive, setIsActive] = React.useState(false)

  const handleClick = () => {
    props.onClick?.()
    setIsActive(true)
    setTimeout(() => setIsActive(false), 500)
  }

  return (
    <Tooltip
      mode='mini'
      visible={isActive}
      text="Copied"
    >
      <Button
        className={classnames({
          [styles.copyButton]: true,
          [styles.copyButtonActive]: isActive
        })}
        fab
        size='tiny'
        kind='plain-faint'
        onClick={handleClick}
      >
        <Icon name='copy' />
      </Button>
    </Tooltip>
  )
}

export default CopyButton
