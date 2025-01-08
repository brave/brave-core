// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import styled from 'styled-components';
import { useNewTabPref } from '../../hooks/usePref';
import SearchBox from './SearchBox';
import { SearchContext, useSearchContext } from './SearchContext';
import SearchDialog from './SearchDialog';
import { searchBoxRadius } from './config';
import Button from '@brave/leo/react/button';
import Icon from '@brave/leo/react/icon';
import ButtonMenu from '@brave/leo/react/buttonMenu';
import { color, spacing } from '@brave/leo/tokens/css/variables';
import { getLocale } from '$web-common/locale';
import getNTPBrowserAPI from '../../api/background';

const MenuContainer = styled(ButtonMenu).attrs({
  'data-theme': 'light'
})`
  position: absolute;
  top: calc(50% - 12px);
  right: -28px;

  opacity: 0;

  transition: opacity 0.2s ease-in-out;
  transition-delay: 1s;

  & > leo-menu-item {
    display: flex;
    align-items: center;
    gap: ${spacing.m};

    padding: ${spacing.m};
  }
`

const MenuButton = styled(Button)`
  color: ${color.container.background};
`

const PlaceholderContainer = styled.div`
  position: relative;
  
  border-radius: ${searchBoxRadius};
  
  box-shadow: 0px 4px 4px 0px rgba(0, 0, 0, 0.05);
  
  &:hover ${MenuContainer} {
    transition-delay: 0s;
    opacity: 1;
  }
`

function Swapper() {
  const { open, setOpen } = useSearchContext()
  const [boxPos, setBoxPos] = React.useState(0)
  const [, setShowSearchBox] = useNewTabPref('showSearchBox')

  return <>
    {!open && <PlaceholderContainer onClick={e => {
      // If we were clicking a button inside the SearchBox, don't open the box.
      if (e.nativeEvent.composedPath().some(el => (el as HTMLElement).tagName === 'LEO-BUTTON')) {
        e.preventDefault()
        return
      }
      setOpen(true)
      setBoxPos(e.currentTarget.getBoundingClientRect().y)
    }}>
      <SearchBox />
      <div data-theme="light">
        <MenuContainer>
          <MenuButton fab kind='plain-faint' slot='anchor-content'>
            <Icon name='more-vertical' />
          </MenuButton>
          <leo-menu-item onClick={e => {
            e.stopPropagation()
            setShowSearchBox(false)
          }}>
            <Icon name='eye-off' /> {getLocale('searchHide')}
          </leo-menu-item>
        </MenuContainer>
      </div>
    </PlaceholderContainer>}
    {open && <SearchDialog offsetY={boxPos} onClose={() => setOpen(false)} />}
  </>
}

export default function SearchPlaceholder() {
  const [showSearchBox] = useNewTabPref('showSearchBox')

  React.useEffect(() => {
    if (!showSearchBox) {
      getNTPBrowserAPI().newTabMetrics.reportNTPSearchDefaultEngine(null)
    }
  }, [showSearchBox]);

  if (!showSearchBox) return null
  return <SearchContext>
    <Swapper />
  </SearchContext>
}
