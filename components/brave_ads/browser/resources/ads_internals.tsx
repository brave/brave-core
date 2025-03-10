// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as AdsInternalsMojo from 'gen/brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.m.js'
import * as React from 'react'
import styled from 'styled-components'
import { render } from 'react-dom'
import 'react-json-view-lite/dist/index.css';

const Container = styled.div`
  margin: 4px;
  display: flex;
  flex-direction: column;
  gap: 10px;
`

const ButtonContainer = styled.div`
  display: flex;
  flex-direction: row;
  gap: 4px;
`

const StateContainer = styled.div`
  padding: 4px;
`

const Table = styled.table`
  table-layout: auto;
  width: 100%;
  border-collapse: collapse;

  th, td {
    border: 1px solid #ddd;
    padding: 8px;
    text-align: left;
  }

  th {
    background-color: #f2f2f2;
  }
`

const API = AdsInternalsMojo.AdsInternals.getRemote();

const pageCallbackRouter = new AdsInternalsMojo.AdsInternalsPageCallbackRouter();

const CONVERSION_URL_PATTERN_TABLE_COLUMN = 'URL Pattern';
const CONVERSION_EXPIRES_AT_TABLE_COLUMN = 'Expires At';
interface AdsInternalConversionUrlPattern {
  CONVERSION_URL_PATTERN_TABLE_COLUMN: string
  CONVERSION_EXPIRES_AT_TABLE_COLUMN: number
}

const AD_EVENT_PLACEMENT_ID_TABLE_COLUMN = 'Placement Id';
const AD_EVENT_TYPE_TABLE_COLUMN = 'Ad Type';
const AD_EVENT_CONFIRMATION_TYPE_TABLE_COLUMN = 'Confirmation Type';
const AD_EVENT_EXPIRES_AT_TABLE_COLUMN = 'Expires At';
interface AdsInternalAdEvent {
  AD_EVENT_PLACEMENT_ID_TABLE_COLUMN: string
  AD_EVENT_TYPE_TABLE_COLUMN: string
  AD_EVENT_CONFIRMATION_TYPE_TABLE_COLUMN: string
  AD_EVENT_EXPIRES_AT_TABLE_COLUMN: number
}

const App: React.FC = () => {
  const [adsInternalConversionUrlPatterns, setAdsInternalConversionUrlPatterns] = React.useState<AdsInternalConversionUrlPattern[]>([]);
  const [adsInternalAdEvents, setAdsInternalAdEvents] = React.useState<AdsInternalAdEvent[]>([]);
  const [rewardsEnabled, setRewardsEnabled] = React.useState<boolean>(false);

  pageCallbackRouter.onBraveRewardsEnabledChanged.addListener((enabled: boolean) => {
    setRewardsEnabled(enabled);
  })
  API.createAdsInternalsPageHandler(pageCallbackRouter.$.bindNewPipeAndPassRemote());

  const getAdsInternals = React.useCallback(async () => {
    try {
      const adsInternals = await API.getAdsInternals()
      const adsInternalsData = JSON.parse(adsInternals.response);
      setAdsInternalConversionUrlPatterns(adsInternalsData.creativeSetConversions || []);
      setAdsInternalAdEvents(adsInternalsData.adEvents || []);
    } catch (error) {
      console.error('Error getting ads internals', error);
    }
  }, []);

  const clearAdsData = React.useCallback(async () => {
    try {
      const result = await API.clearAdsData();
      if (result.success) {
        getAdsInternals();
      } else {
        console.warn('Failed to clear ads data');
      }
    } catch (error) {
      console.error('Error clearing ads data', error);
    }
  }, [getAdsInternals]);

  React.useEffect(() => {
    getAdsInternals();
  }, [getAdsInternals]);

  return (
    <Container>
      <h2>Ads internals</h2>

      <ButtonContainer>
        <button onClick={getAdsInternals}>Refresh</button>
      </ButtonContainer>

      <StateContainer>
        <b>Active Brave Ads conversion URL patterns:</b><br /><br />
        <ConversionUrlPatternsTable data={adsInternalConversionUrlPatterns} /><br /><br />
      </StateContainer>

      <StateContainer>
        <b>Active Brave Ads events:</b><br /><br />
        <AdEventsTable data={adsInternalAdEvents} /><br /><br />
      </StateContainer>

      {!rewardsEnabled && (
        <ButtonContainer>
          <button onClick={clearAdsData}>Clear Ads Data</button>
        </ButtonContainer>
      )}
    </Container>
  );
}

const formatUnixEpochToLocalTime = (epoch: number) => {
  const date = new Date(epoch * 1000);  // Convert seconds to milliseconds.
  return date.toLocaleString();
}

const conversionUrlPatternsTableRow = (header: string, row: any) => {
  return header === CONVERSION_EXPIRES_AT_TABLE_COLUMN ? formatUnixEpochToLocalTime(row[header]) : row[header];
}

const ConversionUrlPatternsTable: React.FC<{ data: AdsInternalConversionUrlPattern[] }> = React.memo(({ data }) => {
  if (data.length === 0) {
    return (
      <p>No Brave Ads conversion URL patterns are currently being matched.</p>
    );
  }

  const tableColumnOrder = [CONVERSION_URL_PATTERN_TABLE_COLUMN, CONVERSION_EXPIRES_AT_TABLE_COLUMN];

  const uniqueTableRows = () => {
    return Array.from(new Set(data.map(item => JSON.stringify(item))))
      .map(item => JSON.parse(item));
  };

  return (
    <Table>
      <thead>
        <tr>
          {tableColumnOrder.map((header) => (
            <th key={header}>{header}</th>
          ))}
        </tr>
      </thead>
      <tbody>
        {uniqueTableRows().map((row, index) => (
          <tr key={index}>
            {tableColumnOrder.map((header) => (
              <td key={header}>
                {conversionUrlPatternsTableRow(header, row)}
              </td>
            ))}
          </tr>
        ))}
      </tbody>
    </Table>
  );
});


const adEventsTableRow = (header: string, row: any) => {
  return header === AD_EVENT_EXPIRES_AT_TABLE_COLUMN ? formatUnixEpochToLocalTime(row[header]) : row[header];
}

const AdEventsTable: React.FC<{ data: AdsInternalAdEvent[] }> = React.memo(({ data }) => {
  if (data.length === 0) {
    return (
      <p>No Brave Ads events.</p>
    );
  }

  const tableColumnOrder = [AD_EVENT_PLACEMENT_ID_TABLE_COLUMN, AD_EVENT_TYPE_TABLE_COLUMN, AD_EVENT_CONFIRMATION_TYPE_TABLE_COLUMN, AD_EVENT_EXPIRES_AT_TABLE_COLUMN];

  const uniqueTableRows = () => {
    return Array.from(new Set(data.map(item => JSON.stringify(item))))
      .map(item => JSON.parse(item));
  };

  return (
    <Table>
      <thead>
        <tr>
          {tableColumnOrder.map((header) => (
            <th key={header}>{header}</th>
          ))}
        </tr>
      </thead>
      <tbody>
        {uniqueTableRows().map((row, index) => (
          <tr key={index}>
            {tableColumnOrder.map((header) => (
              <td key={header}>
                {adEventsTableRow(header, row)}
              </td>
            ))}
          </tr>
        ))}
      </tbody>
    </Table>
  );
});

document.addEventListener('DOMContentLoaded', () => {
  render(<App />, document.getElementById('root'));
});
