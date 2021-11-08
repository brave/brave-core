/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { ExternalWalletProvider, getExternalWalletProviderName } from '../../shared/lib/external_wallet'
import { SmallMenuIcon } from './icons/small_menu_icon'
import { InfoIcon } from './icons/info_icon'
import { UnlinkIcon } from './icons/unlink_icon'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { ConfirmationBox } from './confirmation_box'

import * as style from './linked_devices_view.style'

const contactSupportURL = 'https://community.brave.com'
const deviceLimitLearnMoreURL = 'https://support.brave.com/hc/en-us/articles/360056508071-How-many-Brave-Rewards-wallets-can-be-linked-to-my-crypto-custodian-account-'

const daysFormatter = new Intl.NumberFormat(undefined, {
  style: 'unit',
  unit: 'day',
  unitDisplay: 'long',
  maximumFractionDigits: 0
})

function formatDays (interval: number) {
  const days = Math.ceil(interval / 24 / 60 / 60 / 1000)
  return daysFormatter.format(days)
}

export interface LinkedDevice {
  paymentID: string
  updating: boolean
}

interface Props {
  paymentID: string
  externalWalletProvider: ExternalWalletProvider | null
  linkedDevices: LinkedDevice[]
  openDeviceSlots: number
  unlinkingAvailableAt: number
  unlinkingInterval: number
  onVerifyWallet: () => void
  onUnlinkDevice: (paymentID: string) => void
}

export function LinkedDevicesView (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [bubblePaymentID, setBubblePaymentID] = React.useState('')
  const [confirmUnlink, setConfirmUnlink] = React.useState(false)

  const devices = [...props.linkedDevices].sort((a) => {
    return a.paymentID === props.paymentID ? -1 : 0
  })

  const unlinkingTimeLeft = props.unlinkingAvailableAt - Date.now()
  const maxSlots = devices.length + props.openDeviceSlots

  function renderRow (device: LinkedDevice) {
    const { paymentID } = device
    const isCurrent = paymentID === props.paymentID
    const showActionBubble = paymentID === bubblePaymentID
    const canUnlink = unlinkingTimeLeft <= 0

    const onActionClick = () => {
      setBubblePaymentID(paymentID)
    }

    const hideActionBubble = () => {
      setBubblePaymentID('')
    }

    const onUnlinkClick = () => {
      setConfirmUnlink(true)
      hideActionBubble()
    }

    const onUnlinkConfirm = () => {
      props.onUnlinkDevice(paymentID)
    }

    const onUnlinkCancel = () => {
      setConfirmUnlink(false)
    }

    return (
      <tr key={paymentID}>
        <td>
          {
            isCurrent
              ? <a href='chrome://rewards-internals'>{paymentID}</a>
              : paymentID
          }
        </td>
        <td className='current'>
          {isCurrent && getString('manageWalletCurrentDevice')}
        </td>
        <td>
          <style.actionBubbleAnchor>
            <style.actionMenu>
              <button onClick={onActionClick}>
                <SmallMenuIcon />
              </button>
            </style.actionMenu>
            {
              showActionBubble &&
                <>
                  <style.actionBubble>
                    <button
                      onClick={onUnlinkClick}
                      disabled={!canUnlink}
                    >
                      <UnlinkIcon />{getString('manageWalletUnlink')}
                    </button>
                  </style.actionBubble>
                  <style.bubbleBackdrop onClick={hideActionBubble} />
                </>
            }
          </style.actionBubbleAnchor>
          {
            confirmUnlink &&
              <ConfirmationBox
                header={getString('manageWalletUnlinkConfirmHeader')}
                text={
                  formatMessage(getString('manageWalletUnlinkConfirmText'), [
                    formatDays(props.unlinkingInterval)
                  ])
                }
                buttonText={getString('manageWalletUnlink')}
                onConfirm={onUnlinkConfirm}
                onCancel={onUnlinkCancel}
              />
          }
        </td>
      </tr>
    )
  }

  if (!props.externalWalletProvider) {
    return (
      <style.root>
        <style.notLinked>
          {
            formatMessage(getString('manageWalletNoLinkedDevices'), {
              tags: {
                $1: (content) => (
                  <a key='verify' onClick={props.onVerifyWallet}>
                    {content}
                  </a>
                ),
                $3: (content) => (
                  <style.contactSupport>
                    <NewTabLink key='contact' href={contactSupportURL}>
                      {content}
                    </NewTabLink>
                  </style.contactSupport>
                )
              }
            })
          }
        </style.notLinked>
      </style.root>
    )
  }

  return (
    <style.root>
      <style.description>
        {
          formatMessage(getString('manageWalletDevicesText'), {
            placeholders: {
              $1: String(maxSlots),
              $2: getExternalWalletProviderName(props.externalWalletProvider)
            },
            tags: {
              $3: {
                end: '$4',
                replace: (content) => <strong key='bold'>{content}</strong>
              },
              $5: (content) => (
                <NewTabLink key='learn-more' href={deviceLimitLearnMoreURL}>
                  {content}
                </NewTabLink>
              )
            }
          })
        }
      </style.description>
      <style.devices>
        <table>
          <thead>
            <tr>
              <th>
                {getString('manageWalletPaymentID')}
                <style.idTooltipAnchor>
                  <InfoIcon />
                  <style.idTooltip>
                    <style.idTooltipBody>
                      {
                        formatMessage(getString('manageWalletIdTooltip'), [
                          maxSlots
                        ])
                      }
                    </style.idTooltipBody>
                  </style.idTooltip>
                </style.idTooltipAnchor>
              </th>
            </tr>
          </thead>
          <tbody>
            {devices.map(renderRow)}
          </tbody>
        </table>
      </style.devices>
      {
        unlinkingTimeLeft > 0 &&
          <style.nextUnlinking>
            {
              formatMessage(getString('manageWalletUnlinkingTime'), [
                formatDays(unlinkingTimeLeft)
              ])
            }
          </style.nextUnlinking>
      }
    </style.root>
  )
}
