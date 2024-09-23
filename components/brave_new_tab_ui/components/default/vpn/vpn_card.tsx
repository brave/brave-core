/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Icon from "@brave/leo/react/icon";
import "@brave/leo/tokens/css/variables.css";
import widgetStyles from "./widget_style.module.scss";
import styles from "./style.module.scss";

import { IconName } from "@brave/leo/icons/meta";
import * as React from 'react';
import Button from "@brave/leo/react/button";
import Toggle from "@brave/leo/react/toggle";

import * as Actions from "../../../actions/brave_vpn_actions";
import { BraveVPNState } from "components/brave_new_tab_ui/reducers/brave_vpn";
import { useDispatch } from "react-redux";

const locales = {
  firewallVpn: "Firewall + VPN",
  connected: "Connected",
  disconnected: "Disconnected",
  change: "Change",
}

const WidgetWrapper = (props: React.PropsWithChildren) => (
  <div className={widgetStyles.wrapper}>
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
    <Button className={widgetStyles.menuButton} kind="plain-faint" fab><Icon name="more-vertical" /></Button>
  </div>
)

type WidgetBodyProps = {
  className: string;
}
const WidgetBody = (props: React.PropsWithChildren<WidgetBodyProps>) => (
  <div className={`${widgetStyles.body} ${props.className}`}>
    {props.children}
  </div>
)

const Shield = () => (
  <svg width="64" height="64" fill="none" xmlns="http://www.w3.org/2000/svg">
    <path className={styles.background} d="M34.309 1.49a5.685 5.685 0 0 0-4.618 0L9.793 10.334a5.685 5.685 0 0 0-3.376 5.195v13.36C6.417 44.666 17.332 59.42 32 63c14.668-3.582 25.583-18.334 25.583-34.11V15.528a5.685 5.685 0 0 0-3.376-5.195L34.309 1.49Z" stroke="#000" stroke-width="1.5" />
    <path className={styles.stripes} d="M15.122 29.174c.865.941 2.24 1.033 3.174.185 7.927-7.084 19.437-7.084 27.38-.019.951.849 2.343.775 3.21-.166 1-1.088.933-2.896-.17-3.873-9.693-8.615-23.715-8.615-33.425 0-1.103.959-1.188 2.767-.17 3.873ZM28.294 43.49l2.496 2.71c.662.72 1.731.72 2.393 0l2.496-2.711c.797-.867.628-2.361-.39-2.933a6.754 6.754 0 0 0-6.638 0c-.968.572-1.154 2.066-.357 2.933Zm-6.331-6.88c.831.903 2.139.995 3.106.239 4.142-3.191 9.71-3.191 13.852 0 .967.738 2.275.664 3.106-.24l.017-.018c1.019-1.107.95-2.989-.22-3.892-5.84-4.593-13.801-4.593-19.658 0-1.17.922-1.239 2.785-.203 3.91Z" />
    <defs>
      <linearGradient id="shieldGradient" x1="54.925" y1="58.715" x2="-2.622" y2="12.177" gradientUnits="userSpaceOnUse">
        <stop offset=".026" stop-color="#FA7250" />
        <stop offset=".401" stop-color="#FF1893" />
        <stop offset=".995" stop-color="#A78AFF" />
      </linearGradient>
    </defs>
  </svg>
)

export const VPNWidget = ({
  purchasedState,
  connectionState,
  selectedRegion,
}: BraveVPNState) => {
  const dispatch = useDispatch();
  return (
    <WidgetWrapper>
      <WidgetHeader icon="product-vpn">
        {locales.firewallVpn}
      </WidgetHeader>
      <WidgetBody className={`${styles.body} ${connectionState && styles.connected}`}>
        <div className={`${styles.shield}`}>
          <Icon>
            <Shield />
          </Icon>
        </div>
        <div className={styles.detailsAndActions}>
          <div className={styles.serverDetails}>
            <span className={`${styles.connectionStatus}`}>{connectionState ? locales.connected : locales.disconnected}</span>
            <div className={styles.detailLockup}>
              <span className={styles.region}>{selectedRegion.country}</span>
              <button className={styles.changeServerButton}>{locales.change}</button>
            </div>
            <div className={styles.detailLockup}>
              {selectedRegion.regionPrecision === "city" && <span className={styles.city}>{selectedRegion.namePretty}</span>}
            </div>
          </div>
          <Toggle checked={connectionState === 0} onChange={() => dispatch(Actions.toggleConnection())} />
        </div>
      </WidgetBody>
    </WidgetWrapper>
  )
}