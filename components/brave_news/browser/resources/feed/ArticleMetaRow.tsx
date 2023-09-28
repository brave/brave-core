// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { color, font, spacing } from "@brave/leo/tokens/css"
import { FeedItemMetadata } from "../../../../brave_new_tab_ui/api/brave_news"
import { channelIcons } from "../../../../brave_new_tab_ui/components/default/braveNews/customize/Icons"
import styled from "styled-components";
import * as React from "react";
import Flex from "../../../../brave_new_tab_ui/components/Flex";
import ButtonMenu from "@brave/leo/react/buttonMenu";
import Button from "@brave/leo/react/button";
import Icon from "@brave/leo/react/icon";

const MenuButton = styled(Button)`
  flex-grow: 0;
  --leo-button-padding: ${spacing.s};
`

export const MetaInfoContainer = styled.h4` 
  margin: 0;

  font: ${font.primary.xSmall.regular};
  color: ${color.text.secondary};

  opacity: 0.5;

  display: flex;
  align-items: center;
  gap: ${spacing.s};

  --leo-icon-size: 12px;
`

export const getOrigin = (article: FeedItemMetadata) => {
  const host = new URL(article.url.url).host
  return host.startsWith('www.') ? host.substring(4) : host
}

export function MetaInfo(props: { article: FeedItemMetadata, hideChannel?: boolean }) {

  const maybeChannel = !props.hideChannel && <>
    • {channelIcons[props.article.categoryName] ?? channelIcons.default} {props.article.categoryName}
  </>
  return <MetaInfoContainer>
    {getOrigin(props.article)} {maybeChannel} • {props.article.relativeTimeDescription}
  </MetaInfoContainer>
}

export default function ArticleMetaRow(props: { article: FeedItemMetadata, hideChannel?: boolean }) {
  return <Flex direction="row" justify="space-between" align="center">
    <MetaInfo {...props} />

    <ButtonMenu>
      <MenuButton slot='anchor-content' kind='plain-faint' onClick={e => {
        e.preventDefault()
        e.stopPropagation()
      }}>
        <Icon name='more-horizontal' />
      </MenuButton>
      <leo-menu-item onClick={e => {
        e.preventDefault()
        e.stopPropagation()
      }}>Hide content from {getOrigin(props.article)}</leo-menu-item>
    </ButtonMenu>
  </Flex>
}
