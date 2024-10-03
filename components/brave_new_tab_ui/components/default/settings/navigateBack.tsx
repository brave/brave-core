// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

type Props = {
  onBack: () => any
  title?: string
}

export default function NavigateBack(props: Props) {
  return <Button onClick={props.onBack} kind='plain-faint' fab size='tiny'>
    <Icon name='arrow-left' slot='icon-before' />
    <span>{props.title ? props.title : getLocale('settingsNavigateBack')}</span>
  </Button>
}
