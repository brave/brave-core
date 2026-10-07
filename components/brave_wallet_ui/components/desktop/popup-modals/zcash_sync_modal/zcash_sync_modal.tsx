// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import getAPIProxy from '../../../../common/async/bridge'

// Selectors
import { useSafeUISelector } from '../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../common/selectors'

// Page API Proxy
import getWalletPageApiProxy from '../../../../page/wallet_page_api_proxy'

// Hooks
import {
  useStartShieldSyncMutation,
  useStopShieldSyncMutation,
} from '../../../../common/slices/api.slice'

// Utils
import { openWalletRouteTab } from '../../../../utils/routes-utils'
import { getLocale } from '../../../../../common/locale'

// Types
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// Components
import PopupModal from '..'

// Styled Components
import { ProgressRing } from './zcash_sync_modal.style'
import { Text, Column, Row } from '../../../shared/style'

const blocksPerRange = 1024

interface Props {
  account: BraveWallet.AccountInfo
  onClose: () => void
}

export const ZCashSyncModal = (props: Props) => {
  const { account, onClose } = props

  // Selector
  const isIOS = useSafeUISelector(UISelectors.isIOS)

  // Mutations
  const [startShieldSync] = useStartShieldSyncMutation()
  const [stopShieldSync] = useStopShieldSyncMutation()

  // State
  const [syncStatus, setSyncStatus] = React.useState<
    'started' | 'stopped' | undefined
  >(undefined)
  const [syncError, setSyncError] = React.useState<string>('')
  const [startBlock, setStartBlock] = React.useState<number>(0)
  const [endBlock, setEndBlock] = React.useState<number>(0)
  const [scannedRanges, setScannedRanges] = React.useState<number>(0)
  const [totalRanges, setTotalRanges] = React.useState<number>(0)

  // Methods
  const onStartShieldSync = React.useCallback(async () => {
    await startShieldSync(account.accountId)
  }, [startShieldSync, account])

  const onStopShieldSync = React.useCallback(async () => {
    await stopShieldSync(account.accountId)
  }, [stopShieldSync, account])

  // effects
  React.useEffect(() => {
    const zcashWalletServiceObserver =
      new BraveWallet.ZCashWalletServiceObserverReceiver({
        onSyncStart: (accountId: BraveWallet.AccountId) => {
          if (account.accountId.uniqueKey === accountId.uniqueKey) {
            setSyncStatus('started')
          }
        },
        onSyncStop: (accountId: BraveWallet.AccountId) => {
          if (account.accountId.uniqueKey === accountId.uniqueKey) {
            setSyncStatus('stopped')
          }
        },
        onSyncStatusUpdate: (
          accountId: BraveWallet.AccountId,
          status: BraveWallet.ZCashShieldSyncStatus,
        ) => {
          if (account.accountId.uniqueKey === accountId.uniqueKey) {
            setSyncStatus('started')
            setStartBlock(Number(status.startBlock))
            setEndBlock(Number(status.endBlock))
            setScannedRanges(Number(status.scannedRanges))
            setTotalRanges(Number(status.totalRanges))
          }
        },
        onSyncError: (accountId: BraveWallet.AccountId, error: string) => {
          if (account.accountId.uniqueKey === accountId.uniqueKey) {
            setSyncError(error)
          }
        },
      })

    getAPIProxy().zcashWalletService.addObserver(
      zcashWalletServiceObserver.$.bindNewPipeAndPassRemote(),
    )

    return () => zcashWalletServiceObserver.$.close()
  }, [account])

  // Computed
  const percent =
    scannedRanges === 0 && totalRanges === 0
      ? 0
      : (scannedRanges / totalRanges) * 100
  const calculatedBlock =
    scannedRanges === 0
      ? startBlock
      : scannedRanges === totalRanges
        ? endBlock
        : scannedRanges * blocksPerRange + startBlock
  const syncComplete = syncStatus === 'stopped' && scannedRanges === totalRanges

  return (
    <PopupModal
      onClose={onClose}
      title={getLocale(S.BRAVE_WALLET_SYNC_ACCOUNT_NAME).replace(
        '$1',
        account.name,
      )}
      width='600px'
    >
      <Column
        padding='40px'
        width='100%'
        justifyContent='flex-start'
      >
        <ProgressRing
          mode={syncStatus === undefined ? 'indeterminate' : 'determinate'}
          progress={syncStatus === undefined ? 0.25 : percent * 0.01}
        >
          {syncStatus !== undefined && (
            <Text
              textSize='22px'
              textColor='primary'
              isBold={true}
            >
              {Math.trunc(percent)}%
            </Text>
          )}
        </ProgressRing>
        <Column
          gap='24px'
          width='100%'
        >
          {syncStatus === undefined ? (
            <Text
              textSize='14px'
              textColor='primary'
              isBold={false}
            >
              {getLocale(S.BRAVE_WALLET_INITIALIZING)}
            </Text>
          ) : (
            <>
              <Column gap='8px'>
                <Text
                  textSize='14px'
                  textColor='primary'
                  isBold={false}
                >
                  {getLocale(S.BRAVE_WALLET_PROCESSING_BLOCK)}
                </Text>
                <Text
                  textSize='16px'
                  textColor='primary'
                  isBold={true}
                >
                  {getLocale(S.BRAVE_WALLET_BLOCKS_LEFT).replace(
                    '$1',
                    (endBlock - calculatedBlock).toLocaleString(),
                  )}
                </Text>
                <Text
                  textSize='12px'
                  textColor='tertiary'
                  isBold={false}
                >
                  {getLocale(S.BRAVE_WALLET_BLOCKS_OF_BLOCKS)
                    .replace('$1', calculatedBlock.toLocaleString())
                    .replace('$2', endBlock.toLocaleString())}
                </Text>
              </Column>
              <Column gap='8px'>
                <Text
                  textSize='14px'
                  textColor='primary'
                  isBold={false}
                >
                  {getLocale(S.BRAVE_WALLET_RANGES)}
                </Text>
                <Text
                  textSize='16px'
                  textColor='primary'
                  isBold={true}
                >
                  {getLocale(S.BRAVE_WALLET_BLOCKS_OF_BLOCKS)
                    .replace('$1', scannedRanges.toLocaleString())
                    .replace('$2', totalRanges.toLocaleString())}
                </Text>
              </Column>
              <Button
                onClick={
                  syncComplete
                    ? onClose
                    : syncStatus === 'stopped'
                      ? onStartShieldSync
                      : onStopShieldSync
                }
                kind={syncComplete ? 'filled' : 'outline'}
              >
                {!syncComplete && (
                  <Icon
                    name={
                      syncStatus === 'stopped' ? 'play-filled' : 'pause-filled'
                    }
                    slot='icon-before'
                  />
                )}
                {syncComplete
                  ? getLocale(S.BRAVE_WALLET_BUTTON_CLOSE)
                  : syncStatus === 'stopped'
                    ? getLocale(S.BRAVE_WALLET_BUTTON_CONTINUE)
                    : getLocale(S.BRAVE_WALLET_PAUSE)}
              </Button>
            </>
          )}
          {syncComplete && !syncError && (
            <Alert type='success'>
              {getLocale(S.BRAVE_WALLET_SYNC_COMPLETE_MESSAGE)}
            </Alert>
          )}
          {!syncComplete && !syncError && (
            <Alert type='warning'>
              {getLocale(S.BRAVE_WALLET_SYNC_STARTED_MESSAGE)}
            </Alert>
          )}
          {syncError && <Alert type='error'>{syncError}</Alert>}
          {!syncComplete && (
            <Row justifyContent='center'>
              <div>
                <Button
                  onClick={() => {
                    if (isIOS) {
                      getWalletPageApiProxy().pageHandler.openWalletHome()
                    } else {
                      openWalletRouteTab(WalletRoutes.PortfolioAssets)
                    }
                  }}
                >
                  <Icon
                    name='launch'
                    slot='icon-before'
                  />
                  {getLocale(S.BRAVE_WALLET_CONTINUE_USING_WALLET)}
                </Button>
              </div>
            </Row>
          )}
        </Column>
      </Column>
    </PopupModal>
  )
}
