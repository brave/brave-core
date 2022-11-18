// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { Speedometer } from './components/speedometer/speedometer'
import {
  ContentWrapper,
  IpfsDetailsModalWrapper,
  IpfsDetailsText,
  NetworkSection,
  PeerSection,
  SectionRow,
  SectionValue,
  SectionValueHighlighted,
  TitleText
} from './ipfs-details-modal.style'

export const IpfsDetailsModal = () => {
  return (
    <IpfsDetailsModalWrapper>
      <ContentWrapper>
        <TitleText>Connected to IPFS</TitleText>
        <IpfsDetailsText>Hosting 2.2 GiB of data - Discovered 49 peers</IpfsDetailsText>
        <PeerSection>
          <SectionRow>
            <TitleText>Peer ID</TitleText>
            <SectionValue>12D3KooWLUtacX7FkD9f1hKcGZKnaYBm4Ff8kVwb1vxmCJMX4YRk</SectionValue>
          </SectionRow>
          <SectionRow>
            <TitleText>Agent</TitleText>
            <SectionValue>kubo v0.16.0 brave</SectionValue>
          </SectionRow>
          <SectionRow>
            <TitleText>UI</TitleText>
            <SectionValueHighlighted style={{ marginBottom: 0 }}>v2.15.1</SectionValueHighlighted>
          </SectionRow>
        </PeerSection>
        <TitleText>Network traffic</TitleText>
        <NetworkSection>
          <Speedometer title="Incoming" filled={0} total={125000} />
        </NetworkSection>
      </ContentWrapper>
    </IpfsDetailsModalWrapper>
  )
}
