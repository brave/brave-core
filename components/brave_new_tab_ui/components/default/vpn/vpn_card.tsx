/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Icon from '@brave/leo/react/icon'
import styles from './style.module.scss'
import widgetStyles from './widget_style.module.scss'

import Toggle from '@brave/leo/react/toggle'
import * as React from 'react'
import Flag from '../../../../brave_vpn/resources/panel/components/flag'

import { IconName } from '@brave/leo/icons/meta'
import { BraveVPNState } from 'components/brave_new_tab_ui/reducers/brave_vpn'
import { useDispatch } from 'react-redux'
import * as Actions from '../../../actions/brave_vpn_actions'
import * as BraveVPN from '../../../api/braveVpn'
import { PromoButton, PromoCard } from './styles'

const locales = {
  firewallVpn: 'Firewall + VPN',
  connected: 'Connected',
  disconnected: 'Disconnected',
  change: 'Change',

  promo: {
    heading: 'Extra privacy & security online',
    poweredBy: 'Powered by',
    cta: 'Start free trial',
    freeTrial: '7-day free trial'
  }
}

const WidgetWrapper = (props: React.PropsWithChildren) => (
  <div
    data-theme='dark'
    className={widgetStyles.wrapper}
  >
    {props.children}
  </div>
)

type WidgetHeaderProps = {
  icon: IconName
}
const WidgetHeader = (props: React.PropsWithChildren<WidgetHeaderProps>) => (
  <div className={widgetStyles.header}>
    <Icon name={props.icon} />
    <span className={widgetStyles.title}>{props.children}</span>
  </div>
)

type WidgetBodyProps = {
  className?: string
}
const WidgetBody = (props: React.PropsWithChildren<WidgetBodyProps>) => (
  <div className={`${styles.body} ${props.className}`}>{props.children}</div>
)

const Shield = () => (
  <svg
    width='64'
    height='64'
    fill='none'
    xmlns='http://www.w3.org/2000/svg'
  >
    <path
      className={styles.background}
      d='M34.309 1.49a5.685 5.685 0 0 0-4.618 0L9.793 10.334a5.685 5.685 0 0 0-3.376 5.195v13.36C6.417 44.666 17.332 59.42 32 63c14.668-3.582 25.583-18.334 25.583-34.11V15.528a5.685 5.685 0 0 0-3.376-5.195L34.309 1.49Z'
      stroke='#000'
      strokeWidth='1.5'
    />
    <path
      className={styles.stripes}
      d='M15.122 29.174c.865.941 2.24 1.033 3.174.185 7.927-7.084 19.437-7.084 27.38-.019.951.849 2.343.775 3.21-.166 1-1.088.933-2.896-.17-3.873-9.693-8.615-23.715-8.615-33.425 0-1.103.959-1.188 2.767-.17 3.873ZM28.294 43.49l2.496 2.71c.662.72 1.731.72 2.393 0l2.496-2.711c.797-.867.628-2.361-.39-2.933a6.754 6.754 0 0 0-6.638 0c-.968.572-1.154 2.066-.357 2.933Zm-6.331-6.88c.831.903 2.139.995 3.106.239 4.142-3.191 9.71-3.191 13.852 0 .967.738 2.275.664 3.106-.24l.017-.018c1.019-1.107.95-2.989-.22-3.892-5.84-4.593-13.801-4.593-19.658 0-1.17.922-1.239 2.785-.203 3.91Z'
    />
    <defs>
      <linearGradient
        id='shieldGradient'
        x1='54.925'
        y1='58.715'
        x2='-2.622'
        y2='12.177'
        gradientUnits='userSpaceOnUse'
      >
        <stop
          offset='.026'
          stopColor='#FA7250'
        />
        <stop
          offset='.401'
          stopColor='#FF1893'
        />
        <stop
          offset='.995'
          stopColor='#A78AFF'
        />
      </linearGradient>
    </defs>
  </svg>
)

export const VPNWidgetHeader = () => (
  <WidgetHeader icon='product-vpn'>{locales.firewallVpn}</WidgetHeader>
)

export const VPNPromoWidget = () => {
  const dispatch = useDispatch()

  return (
    <PromoCard>
      <WidgetWrapper>
        <svg
          className={styles.shieldBg}
          width='108'
          height='145'
          fill='none'
          xmlns='http://www.w3.org/2000/svg'
        >
          <path
            fill-rule='evenodd'
            clip-rule='evenodd'
            d='m30.506 37.01 41.117-18.097a2.186 2.186 0 0 1 1.754 0l41.117 18.097c.772.34 1.204 1.06 1.204 1.783v27.341c0 27.325-18.662 52.655-43.198 59.802-24.537-7.147-43.198-32.478-43.198-59.802v-27.34c0-.725.432-1.445 1.204-1.784ZM77.27 10.065a11.852 11.852 0 0 0-9.542 0L26.612 28.162c-4.243 1.867-6.977 6.034-6.977 10.631v27.341c0 32.284 22.556 62.474 52.865 69.803 30.309-7.329 52.864-37.519 52.864-69.803v-27.34c0-4.598-2.734-8.765-6.976-10.632L77.271 10.065ZM47.86 65.398a42.25 42.25 0 0 1 24.64-7.874 42.251 42.251 0 0 1 24.638 7.874 5.067 5.067 0 1 0 5.888-8.25 52.385 52.385 0 0 0-30.527-9.76 52.386 52.386 0 0 0-30.527 9.76 5.068 5.068 0 1 0 5.888 8.25Zm9.581 16.624A23.328 23.328 0 0 1 72.5 76.548a23.328 23.328 0 0 1 15.057 5.474 5.068 5.068 0 1 0 6.515-7.764A33.463 33.463 0 0 0 72.5 66.412a33.463 33.463 0 0 0-21.572 7.846 5.068 5.068 0 1 0 6.514 7.764Zm24.632 7.18c1.308 1.161.977 3.185-.5 4.12a47.975 47.975 0 0 1-7.988 4.083 2.946 2.946 0 0 1-2.17 0 47.972 47.972 0 0 1-7.988-4.082c-1.478-.936-1.809-2.96-.5-4.121a14.37 14.37 0 0 1 9.573-3.635 14.37 14.37 0 0 1 9.573 3.635Z'
            fill='#fff'
            opacity='.1'
          />
        </svg>
        <VPNWidgetHeader />
        <WidgetBody className={styles.promo}>
          <h3 className={styles.title}>{locales.promo.heading}</h3>
          <div className={styles.poweredBy}>
            {locales.promo.poweredBy}
            <svg
              width='56'
              height='12'
              fill='none'
              xmlns='http://www.w3.org/2000/svg'
            >
              <path
                fill-rule='evenodd'
                clip-rule='evenodd'
                d='M3.611 9.574a10.755 10.755 0 0 0 1.885 1.597l.506.319c.141.086.38.232.536.299l.496.211 1.011-.585.004-.002a9.88 9.88 0 0 0 1.24-.868 9.917 9.917 0 0 0 2.825-3.796l.141-.409c.147-.459.445-1.4.464-1.487.158-.73.257-1.402.24-2.315l-.013-.682-.491-.31C8.296-1.225 3.46.352 1.564 1.486a55.33 55.33 0 0 0-.58.357l-.02.684C.878 5.796 2.239 8.105 3.612 9.574Zm4.997.137c.864-.705 1.898-1.804 2.57-3.38v-.144l-.05-.071c-1.71-1.49-3.275-1.705-4.322-1.613a4.554 4.554 0 0 0-.873.162l-.195.057.071.19a6.59 6.59 0 0 0 .372.802c.119.216.256.43.395.574l.069.072.099-.017a2.8 2.8 0 0 1 .794-.028 1.957 1.957 0 0 1 .758.248 1.82 1.82 0 0 1 .26.186c-.74 1.032-1.348 1.53-1.584 1.699-2.141-1.723-2.721-4.03-2.84-4.634 2.053-.735 3.516-.723 4.625-.43.762.202 1.363.537 1.887.873a14.832 14.832 0 0 1 .423.279l.31.206.2.17.09-.288a13.87 13.87 0 0 0 .092-.468 8.086 8.086 0 0 0 .123-1.598l-.002-.102-.086-.054C6.91-.678 2.65 2.074 2.191 2.37c-.014.01-.053.028-.06.032l-.087.053-.003.102c-.079 2.924 1.13 4.97 2.357 6.283a9.674 9.674 0 0 0 1.695 1.436c.234.151.735.467.868.524l.54-.313a8.97 8.97 0 0 0 1.107-.775ZM19.133 2.4c-2.208 0-3.772 1.493-3.772 3.602 0 2.169 1.645 3.598 3.757 3.598 1.823 0 3.443-1.305 3.443-3.528 0-.169-.01-.338-.036-.521h-3.402v1.28h1.726c-.197.819-.784 1.255-1.792 1.255-1.053 0-1.954-.749-1.954-2.084 0-1.21.815-2.044 1.949-2.044.658 0 1.16.238 1.514.61l1.205-1.16c-.618-.631-1.514-1.008-2.638-1.008Zm25.542.945c0 .456-.321.77-.776.77-.45 0-.767-.314-.767-.77s.312-.774.767-.774.776.313.776.774Zm0 1.455h-1.543v4.8h1.543V4.8Zm-9.291.718V4.8h-1.509v4.8h1.509V7.262c0-.85.604-1.16 1.209-1.16.255-.025.612.056.71.1V4.895c-.122-.079-.447-.096-.594-.095-.564 0-1.097.266-1.325.718ZM33.189 9.6V4.8h-1.505v.568c-.271-.321-.78-.568-1.446-.568-1.264 0-2.191 1.027-2.191 2.405 0 1.377.927 2.395 2.19 2.395.667 0 1.176-.143 1.447-.469V9.6h1.505Zm-2.58-3.456c.601.005 1.02.437 1.02 1.06 0 .625-.419 1.061-1.02 1.061-.603 0-1.023-.436-1.023-1.06 0-.624.42-1.061 1.022-1.061ZM50.331 4.8v4.8h-1.505v-.47c-.272.326-.78.469-1.446.469-1.264 0-2.192-1.018-2.192-2.395 0-1.378.928-2.405 2.192-2.405.666 0 1.174.247 1.446.568V4.8h1.505Zm-1.56 2.405c0-.624-.419-1.056-1.021-1.061-.602 0-1.022.437-1.022 1.06 0 .625.42 1.061 1.022 1.061.602 0 1.022-.436 1.022-1.06ZM42.448 9.6V2.57H40.94v2.86c-.271-.317-.78-.56-1.446-.56-1.263 0-2.191 1.013-2.191 2.37 0 1.358.928 2.36 2.191 2.36.666 0 1.175-.14 1.446-.461V9.6h1.506Zm-2.582-3.406c.602.004 1.022.43 1.022 1.045s-.42 1.046-1.022 1.046c-.602 0-1.021-.43-1.021-1.046 0-.615.42-1.045 1.021-1.045ZM26.025 7.42V4.8h1.507v2.738c0 1.195-.894 2.062-2.252 2.062-1.358 0-2.205-.867-2.205-2.033V4.8h1.507v2.62c0 .466.3.774.698.774.45 0 .746-.303.746-.774Zm29.45-.876V9.6h-1.477V6.984c0-.524-.47-.773-.84-.773-.384 0-.905.25-.905.773V9.6h-1.406V4.8h1.406v.593c.331-.422.944-.593 1.494-.593.881 0 1.728.622 1.728 1.744Z'
                fill='#fff'
              />
            </svg>
          </div>
          <PromoButton
            kind='plain-faint'
            onClick={() => dispatch(Actions.openVPNAccountPage())}
          >
            {locales.promo.cta}
          </PromoButton>
          <p className={styles.poweredBy}>{locales.promo.freeTrial}</p>
        </WidgetBody>
      </WidgetWrapper>
    </PromoCard>
  )
}

export const VPNWidget = ({
  connectionState,
  selectedRegion
}: BraveVPNState) => {
  const dispatch = useDispatch()
  return (
    <>
      <VPNWidgetHeader />
      <WidgetWrapper>
        <WidgetBody
          className={
            connectionState === BraveVPN.ConnectionState.CONNECTED &&
            styles.connected
          }
        >
          <div className={`${styles.shield}`}>
            <Icon>
              <Shield />
            </Icon>
          </div>
          <div className={styles.detailsAndActions}>
            <div className={styles.serverDetails}>
              <span className={`${styles.connectionStatus}`}>
                {connectionState === BraveVPN.ConnectionState.CONNECTED
                  ? locales.connected
                  : locales.disconnected}
              </span>
              <div className={styles.detailLockup}>
                <Flag countryCode={selectedRegion.countryIsoCode} />
                <span className={styles.region}>{selectedRegion.country}</span>
                <button
                  className={styles.changeServerButton}
                  onClick={() => dispatch(Actions.launchVPNPanel())}
                >
                  {locales.change}
                </button>
              </div>
              <div className={styles.detailLockup}>
                {selectedRegion.regionPrecision === 'city' && (
                  <span className={styles.city}>
                    {selectedRegion.namePretty}
                  </span>
                )}
              </div>
            </div>
            <Toggle
              checked={connectionState === BraveVPN.ConnectionState.CONNECTED}
              onChange={() => dispatch(Actions.toggleConnection())}
            />
          </div>
        </WidgetBody>
      </WidgetWrapper>
    </>
  )
}
