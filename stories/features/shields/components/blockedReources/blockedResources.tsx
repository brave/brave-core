/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Label,
  Resources,
  ResourcesHeader,
  ResourcesSiteInfo,
  ResourcesList,
  CloseButton,
  CloseIcon,
  ResourcesLabelGrid,
  EmptyButton,
  Stat,
  ResourcesLabel,
  ShowLessIcon
} from '../../../../../src/features/shields'

interface Props {
  sitename: string
  data: string | number
  title: string
  favicon: string
  onToggle: (event: React.MouseEvent<HTMLButtonElement>) => void
}

export default class BlockedResources extends React.PureComponent<Props, {}> {
  render () {
    const { sitename, data, title, favicon, onToggle, children } = this.props
    return (
      <Resources>
        <ResourcesHeader>
          <ResourcesSiteInfo>
            <img src={favicon} />
            <Label size='large'>{sitename}</Label>
          </ResourcesSiteInfo>
          <CloseButton onClick={onToggle}><CloseIcon /></CloseButton>
        </ResourcesHeader>
        <ResourcesLabelGrid>
          <EmptyButton onClick={onToggle}><ShowLessIcon /></EmptyButton>
          <Stat>{data}</Stat>
          <ResourcesLabel>{title}</ResourcesLabel>
        </ResourcesLabelGrid>
        <ResourcesList>{children}</ResourcesList>
      </Resources>
    )
  }
}
