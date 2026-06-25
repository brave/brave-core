// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// brave://wallet/snap/:snapId — renders the home page UI for any installed snap.

import * as React from 'react'
import { useParams, useHistory } from 'react-router-dom'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'

import { WalletRoutes } from '../../../constants/types'
import getWalletPageApiProxy from '../../wallet_page_api_proxy'
import {
  SnapHomePageRenderer,
  type SnapUserInputEvent,
} from '../../../common/snap/snap_ui_renderer'
import {
  BackLink,
  ContentCard,
  EmptyState,
  EmptyStateText,
  ErrorAlert,
  Header,
  HeaderActions,
  LoadingState,
  LoadingText,
  Page,
  SnapIdHeading,
  TitleRow,
  UnloadButton,
} from './snap_page.style'

export function SnapPage() {
  const { snapId: encodedSnapId } = useParams<{ snapId: string }>()
  const snapId = decodeURIComponent(encodedSnapId)
  const history = useHistory()

  const [homePageData, setHomePageData] = React.useState<object | null>(null)
  const [interfaceId, setInterfaceId] = React.useState<string | null>(null)
  const [loading, setLoading] = React.useState(false)
  const [error, setError] = React.useState<string | null>(null)

  const loadHomePage = React.useCallback(async () => {
    setLoading(true)
    setError(null)
    try {
      const { snapBridge } = getWalletPageApiProxy()
      const result = await snapBridge.getHomePage(snapId)
      if (result.error) {
        setError(result.error)
      } else {
        setHomePageData(result.content as object | null)
        setInterfaceId(result.interfaceId ?? null)
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : String(err))
    } finally {
      setLoading(false)
    }
  }, [snapId])

  // Auto-load on mount.
  React.useEffect(() => {
    loadHomePage()
  }, [loadHomePage])

  const handleUserInput = React.useCallback(
    async (event: SnapUserInputEvent) => {
      if (!interfaceId) {
        return
      }
      const { snapBridge } = getWalletPageApiProxy()
      const result = await snapBridge.sendUserInput(snapId, interfaceId, event)
      if (result.error) {
        setError(result.error)
        return
      }
      if (result.content) {
        setHomePageData(result.content as object)
      }
    },
    [snapId, interfaceId],
  )

  const handleUnload = React.useCallback(() => {
    getWalletPageApiProxy().snapBridge.unloadSnap(snapId)
    setHomePageData(null)
    setInterfaceId(null)
    setError(null)
  }, [snapId])

  return (
    <Page>
      <Header>
        <BackLink onClick={() => history.push(WalletRoutes.SnapsStore)}>
          <Icon name='arrow-left' />
          Snaps Store
        </BackLink>
        <TitleRow>
          <SnapIdHeading>{snapId}</SnapIdHeading>
          <HeaderActions>
            <Button
              onClick={loadHomePage}
              isDisabled={loading}
            >
              {loading ? 'Loading…' : 'Reload'}
            </Button>
            <UnloadButton
              kind='outline'
              onClick={handleUnload}
            >
              Unload
            </UnloadButton>
          </HeaderActions>
        </TitleRow>
      </Header>

      {error && <ErrorAlert type='error'>{error}</ErrorAlert>}

      {loading && !homePageData && (
        <LoadingState>
          <ProgressRing mode='indeterminate' />
          <LoadingText>Loading snap UI…</LoadingText>
        </LoadingState>
      )}

      {homePageData && (
        <ContentCard>
          <SnapHomePageRenderer
            data={homePageData}
            onUserInput={handleUserInput}
          />
        </ContentCard>
      )}

      {!loading && !homePageData && !error && (
        <EmptyState>
          <EmptyStateText>
            This snap does not provide a home page UI.
          </EmptyStateText>
        </EmptyState>
      )}
    </Page>
  )
}

export default SnapPage
