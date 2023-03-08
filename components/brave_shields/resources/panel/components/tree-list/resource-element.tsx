// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import classnames from '$web-common/classnames'
import style from './resource-element.module.scss'
import {
  PermissionButtonHandler
} from '../../state/component_types'

interface UrlElementProps {
  name: string
  permissionButtonTitle: string
  onTextExpand?: () => void
  onPermissionButtonClick: PermissionButtonHandler
  isHost: boolean
}

function ResourceElement (props: UrlElementProps) {
  const [isExpanded, setExpanded] = React.useState(false)

  const handleTextClick = () => {
    // TODO(nullhook): Trigger if the element overflows otherwise we're
    // recalculating positions.
    if (isExpanded) return
    setExpanded(true)
    props.onTextExpand?.()
  }

  const handlePermissionButtonClick = () => {
    props.onPermissionButtonClick?.(props.name)
  }

  const urlTextClass = classnames({
    [style.textUrl]: true,
    [style.textMultiline]: isExpanded,
    [style.textHost]: props.isHost
  })

  const containerClass = classnames({
    [style.container]: true,
    [style.containerAlignTop]: isExpanded
  })

  return (
    <div className={containerClass}>
      <div className={urlTextClass} onClick={handleTextClick}>
        {props.name}
      </div>
      {props.onPermissionButtonClick && (
        <button onClick={handlePermissionButtonClick}
                className={style.permissionButton}>
          {props.permissionButtonTitle}
        </button>
      )}
    </div>
  )
}

export default ResourceElement
