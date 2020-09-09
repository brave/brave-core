/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../common/locale'

import { Section, Title } from '../style'

export class UninstalledView extends React.Component {
  render () {
    return (
      <Section>
        <Title>
          {getLocale('daemonStatusTitle')}
        </Title>
        <div>
          {getLocale('not_installed')}
        </div>
      </Section>
    )
  }
}
