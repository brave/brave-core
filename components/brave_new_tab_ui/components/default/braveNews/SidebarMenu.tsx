// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react';
import styled from 'styled-components';
import FeedNavigation from '../../../../brave_news/browser/resources/FeedNavigation';
import SettingsButton from '../../../../brave_news/browser/resources/SettingsButton';
import Icon from '@brave/leo/react/icon';

const Container = styled.dialog`
  top: var(--bn-top-bar-height);
  height: calc(100vh - var(--bn-top-bar-height));

  overflow: hidden;

  margin-left: 0;
  margin-top: 0;
  padding: 0;

  position: fixed;
  display: flex;

  transition: transform 0.2s ease-in-out;
  transform: translateX(-100%);

  background: var(--bn-glass-card);
  backdrop-filter: blur(64px);
  border: none;
  outline: none;

  &[open] {
    transform: translateX(0);
  }

  &> div {
    background: unset;
  }
`

const MenuButton = styled(SettingsButton)`
  margin-right: auto;
`

export default function SidebarMenu() {
  const dialogRef = React.useRef<HTMLDialogElement>()
  return <MenuButton onClick={() => dialogRef.current?.showModal()}>
    <Icon name="hamburger-menu" />
    <Container ref={dialogRef as any} onClick={e => {
      // Close the menu on click outside.
      const bounds = e.currentTarget.getBoundingClientRect()
      if (e.clientX < bounds.x || e.clientY < bounds.y || e.clientX > bounds.right || e.clientY > bounds.bottom) {
        e.currentTarget.close()
      }
    }}>
      <FeedNavigation />
    </Container>
  </MenuButton>
}
