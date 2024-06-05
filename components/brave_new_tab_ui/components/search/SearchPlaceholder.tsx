// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import styled from 'styled-components';
import { useNewTabPref } from '../../hooks/usePref';
import SearchBox, { Backdrop } from './SearchBox';
import { SearchContext, useSearchContext } from './SearchContext';
import SearchDialog from './SearchDialog';
import { searchBoxRadius } from './config';
import Button from '@brave/leo/react/button';
import Icon from '@brave/leo/react/icon';

const MenuButton = styled(Button)`
  color: white;

  position: absolute;
  top: calc(50% - 12px);
  right: -28px;

  opacity: 0;

  transition: opacity 0.2s ease-in-out;
  transition-delay: 1s;
`

const PlaceholderContainer = styled.div`
  position: relative;
  
  border-radius: ${searchBoxRadius};
  
  box-shadow: 0px 4px 4px 0px rgba(0, 0, 0, 0.10);
  
  &:hover ${MenuButton} {
    transition-delay: 0s;
    opacity: 1;
  }
`

function Swapper() {
  const { open, setOpen } = useSearchContext()
  const [boxPos, setBoxPos] = React.useState(0)
  return <>
    {!open && <PlaceholderContainer onClick={e => {
      // If we were clicking a button inside the SearchBox, don't open the box.
      if (e.nativeEvent.composedPath().some(el => el['tagName'] === 'LEO-BUTTON')) {
        console.log(e.nativeEvent.composedPath())
        return
      }
      setOpen(true)
      setBoxPos(e.currentTarget.getBoundingClientRect().y)
    }}>
      <Backdrop />
      <SearchBox />
      <MenuButton fab kind='plain-faint'>
        <Icon name='more-vertical' />
      </MenuButton>
    </PlaceholderContainer>}
    {open && <SearchDialog offsetY={boxPos} onClose={() => setOpen(false)} />}
  </>
}

export default function SearchPlaceholder() {
  const [showSearchBox] = useNewTabPref('showSearchBox')
  if (!showSearchBox) return null
  return <SearchContext>
    <Swapper />
  </SearchContext>
}
