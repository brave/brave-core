// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Tooltip from './styles'

interface WidgetInfo {
  title: string
  icon: string
  description: string
  bannerImage: string
}

interface Props {
  widgetInfo: WidgetInfo
}

export const NewWidgetTooltip = ({ widgetInfo }: Props) => {
  const { title, icon, description, bannerImage } = widgetInfo

  return (
      <Tooltip.Wrapper>
          <Tooltip.CornerLabel>New Card</Tooltip.CornerLabel>
          <Tooltip.Content>
              <Tooltip.Heading><Tooltip.Icon src={icon} /> {title}</Tooltip.Heading>
              <Tooltip.Body>{description}</Tooltip.Body>
              <Tooltip.Banner>
                  <img src={bannerImage} />
              </Tooltip.Banner>
              <Tooltip.Button>Add {title}</Tooltip.Button>
          </Tooltip.Content>
      </Tooltip.Wrapper>
  )
}
