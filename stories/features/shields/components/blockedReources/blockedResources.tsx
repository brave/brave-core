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
} from '../../../../../src/features/shields'

interface Props {
  sitename: string
  data: string | number
  title: string
  favicon: string
  onToggle: (event: React.MouseEvent<HTMLButtonElement>) => void
  dynamic?: boolean
}

const dynamicHeader = (
  title: string,
  onToggle: (event: React.MouseEvent<HTMLButtonElement>) => void
) => {
  return (
    <ResourcesStatusGrid withStats={false} onClick={onToggle}>
      <EmptyButton><ShowLessIcon /></EmptyButton>
      <ResourcesStatusTitle>{title}</ResourcesStatusTitle>
    </ResourcesStatusGrid>
  )
}

const staticHeader = (
  data: string | number,
  title: string,
  onToggle: (event: React.MouseEvent<HTMLButtonElement>) => void
) => {
  return (
    <ResourcesStatusGrid withStats={true} onClick={onToggle}>
      <EmptyButton><ShowLessIcon /></EmptyButton>
      <StatFlex>{data}</StatFlex>
      <ResourcesStatusTitle>{title}</ResourcesStatusTitle>
    </ResourcesStatusGrid>
  )
}

export default class BlockedResources extends React.PureComponent<Props, {}> {
  render () {
    const { sitename, data, title, favicon, onToggle, dynamic, children } = this.props
    return (
      <ResourcesGrid>
        <ResourcesSiteInfoFlex>
          <ResourcesSiteInfoGrid>
            <img src={favicon} />
            <Label size='large'>{sitename}</Label>
          </ResourcesSiteInfoGrid>
          <CloseButton onClick={onToggle}><CloseIcon /></CloseButton>
        </ResourcesSiteInfoFlex>
        {dynamic ? dynamicHeader(title, onToggle) : staticHeader(data, title, onToggle)}
        {children}
      </ResourcesGrid>
    )
  }
}
