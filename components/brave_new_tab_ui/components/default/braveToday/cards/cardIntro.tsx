// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import * as Card from '../cardIntro'
import BraveTodayLogo from '../braveTodayLogo.svg'
import { getLocale } from '../../../../../common/locale'

class HeaderBlock extends React.PureComponent<{}, {}> {
  onClickHideWidget = () => {
    return undefined
  }
  render () {
    return (
      <Card.Intro>
        <Card.Image src={BraveTodayLogo} />
        <Card.Heading>{getLocale('braveTodayTitle')}</Card.Heading>
        <Card.Text>{getLocale('braveTodayDescription')}</Card.Text>
      </Card.Intro>
    )
  }
}

export default HeaderBlock
