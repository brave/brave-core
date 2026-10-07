// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Local Storage
import {
  useSyncedLocalStorage, //
} from '$wallet/common/hooks/use_local_storage'
import {
  LOCAL_STORAGE_KEYS, //
} from '$wallet/common/constants/local-storage-keys'

// Selectors
import {
  useSafeUISelector, //
} from '$wallet/common/hooks/use-safe-selector'
import { UISelectors } from '$wallet/common/selectors'

// Types
import { WalletRoutes } from '$wallet/constants/types'

// Utils
import { openWalletRouteTab } from '$wallet/utils/routes-utils'
import { getLocale } from '$web-common/locale'

// Components
import { WalletSettingsMenu } from '$wallet/page/components/wallet_menus/wallet_settings_menu'

// Styled Components
import {
  Button,
  ButtonIcon,
  SidePanelIcon,
  LeftRightContainer,
  SidePanelWrapper,
} from './shared_panel_headers.style'
import { HeaderTitle } from './shared_card_headers.style'
import { Row, Text, HorizontalDivider } from '$wallet/components/shared/style'
import { useCloseSidePanelUIMutation } from '$wallet/common/slices/api.slice'

interface Props {
  title?: string
  expandRoute?: WalletRoutes
  actionIconName?: string
  onClickActionButton?: () => void
}

export const DefaultPanelHeader = (props: Props) => {
  const { title, expandRoute, actionIconName, onClickActionButton } = props

  // UI Selectors (safe)
  const isMobile = useSafeUISelector(UISelectors.isMobile)
  const isSidePanel = useSafeUISelector(UISelectors.isSidePanel)

  // Local Storage
  const [_isNavOpen, setIsNavOpen] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.IS_NAVIGATION_OPEN,
    false,
  )

  // Mutations
  const [closeSidePanelUI] = useCloseSidePanelUIMutation()

  // Methods
  const onClickToggleNav = React.useCallback(() => {
    setIsNavOpen((prev) => !prev)
  }, [setIsNavOpen])

  const onClickExpand = React.useCallback(() => {
    if (expandRoute) {
      openWalletRouteTab(expandRoute)
      return
    }
    openWalletRouteTab(WalletRoutes.PortfolioAssets)
  }, [expandRoute])

  const onClickClose = React.useCallback(() => {
    closeSidePanelUI()
  }, [closeSidePanelUI])

  if (isSidePanel) {
    return (
      <SidePanelWrapper
        padding='16px'
        justifyContent='space-between'
      >
        <LeftRightContainer
          width='unset'
          justifyContent='flex-start'
          gap='12px'
        >
          <Button onClick={onClickToggleNav}>
            <SidePanelIcon name='hamburger-menu' />
          </Button>
          <Text
            variant='heading.h4'
            textColor='primary'
          >
            {getLocale(S.BRAVE_WALLET_TITLE)}
          </Text>
          {title && (
            <>
              <HorizontalDivider />
              <Text
                variant='heading.h4'
                textColor='secondary'
              >
                {title}
              </Text>
            </>
          )}
        </LeftRightContainer>
        <LeftRightContainer
          width='unset'
          justifyContent='flex-end'
          gap='12px'
        >
          {expandRoute && (
            <Button onClick={onClickExpand}>
              <SidePanelIcon name='expand' />
            </Button>
          )}
          {actionIconName && onClickActionButton && (
            <Button onClick={onClickActionButton}>
              <SidePanelIcon name={actionIconName} />
            </Button>
          )}
          <WalletSettingsMenu>
            <Button slot='anchor-content'>
              <SidePanelIcon name='more-vertical' />
            </Button>
          </WalletSettingsMenu>
          <HorizontalDivider />
          <Button onClick={onClickClose}>
            <SidePanelIcon name='close' />
          </Button>
        </LeftRightContainer>
      </SidePanelWrapper>
    )
  }

  return (
    <Row
      padding='16px'
      justifyContent='space-between'
    >
      <LeftRightContainer
        width='unset'
        justifyContent='flex-start'
      >
        {!isMobile && expandRoute && (
          <Button onClick={onClickExpand}>
            <ButtonIcon name='expand' />
          </Button>
        )}
      </LeftRightContainer>
      {title && (
        <HeaderTitle
          variant='large.semibold'
          textColor='primary'
        >
          {title}
        </HeaderTitle>
      )}
      <LeftRightContainer
        width='unset'
        justifyContent='flex-end'
      >
        {actionIconName && onClickActionButton && (
          <Button onClick={onClickActionButton}>
            <ButtonIcon name={actionIconName} />
          </Button>
        )}
        <WalletSettingsMenu>
          <Button slot='anchor-content'>
            <ButtonIcon name='more-vertical' />
          </Button>
        </WalletSettingsMenu>
      </LeftRightContainer>
    </Row>
  )
}
