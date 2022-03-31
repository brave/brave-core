// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import PopupMenu, { EllipsisTrigger } from '../../../../components/popupMenu'

interface Props {
  onRemove: Function
  key: string
}

export default function DirectFeedItemMenu (props: Props) {
  const [isOpen, setIsOpen] = React.useState(false)
  return (
    <EllipsisTrigger isOpen={isOpen} onTrigger={setIsOpen.bind(null, !isOpen)}>
      <PopupMenu
        isOpen={isOpen}
        onClose={setIsOpen.bind(null, false)}
        menuItems={[{
          key: props.key,
          onClick: props.onRemove,
          child: <span>Remove</span>
        }]}
      />
    </EllipsisTrigger>
  )
}
