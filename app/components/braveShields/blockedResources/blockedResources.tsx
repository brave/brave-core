/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Label,
  ResourcesGrid,
  ResourcesSiteInfoGrid,
  CloseButton,
  CloseIcon,
  EmptyButton,
  StatFlex,
  ShowLessIcon,
  ResourcesStatusTitle,
  ResourcesSiteInfoFlex,
  ResourcesStatusGrid
} from 'brave-ui/features/shields'

// Utils
import { getFavicon } from '../../../helpers/shieldsUtils'

interface Props {
  hostname: string
  stats: number
  title: string
  url: string
  onToggle: (event: React.MouseEvent<HTMLButtonElement>) => void
  dynamic?: boolean
}

export default class BlockedResources extends React.PureComponent<Props, {}> {
  renderHeader = (stats: number | undefined, title: string, onToggle: (event: React.MouseEvent<HTMLButtonElement>) => void) => {
    return (
      <ResourcesStatusGrid withStats={stats !== undefined} onClick={onToggle}>
        <EmptyButton><ShowLessIcon /></EmptyButton>
        {stats !== undefined ? <StatFlex>{stats}</StatFlex> : null}
        <ResourcesStatusTitle>{title}</ResourcesStatusTitle>
      </ResourcesStatusGrid>
    )
  }

  render () {
    const { stats, hostname, title, url, onToggle, children } = this.props
    return (
      <ResourcesGrid>
        <ResourcesSiteInfoFlex>
          <ResourcesSiteInfoGrid>
            <img src={getFavicon(url)} />
            <Label size='large'>{hostname}</Label>
          </ResourcesSiteInfoGrid>
          <CloseButton onClick={onToggle}><CloseIcon /></CloseButton>
        </ResourcesSiteInfoFlex>
        {this.renderHeader(stats, title, onToggle)}
        {children}
      </ResourcesGrid>
    )
  }
}
