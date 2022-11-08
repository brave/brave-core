// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// style
import {
  ArticleLinkIcons,
  Bubble, BubbleIcon,
  BubbleIconBackground,
  BubbleText,
  IconBubbleColors
} from './article-link-bubble.style'

export interface ArticleLinkBubbleProps {
  icon: ArticleLinkIcons
  iconBackgroundColor: IconBubbleColors
  text: string
  url: string
}

export const ArticleLinkBubble: React.FC<ArticleLinkBubbleProps> = ({
  icon,
  iconBackgroundColor,
  text,
  url
}) => {
  return (
    <Bubble
      href={url}
      target='_blank'
      rel='noreferrer'
    >
      <BubbleIconBackground
        backgroundColor={iconBackgroundColor}
      >
        <BubbleIcon
          icon={icon}
        />
      </BubbleIconBackground>
      <BubbleText>{text}</BubbleText>
    </Bubble>
  )
}

export default ArticleLinkBubble
