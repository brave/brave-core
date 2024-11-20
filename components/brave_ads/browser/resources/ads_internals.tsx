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

const URL_PATTERN_TABLE_COLUMN = 'URL Pattern';
const EXPIRES_AT_TABLE_COLUMN = 'Expires At';

interface AdsInternal {
  URL_PATTERN_TABLE_COLUMN: string
  EXPIRES_AT_TABLE_COLUMN: number
}

const App: React.FC = () => {
  const [adsInternals, setAdsInternals] = React.useState<AdsInternal[]>([]);
  const [rewardsEnabled, setRewardsEnabled] = React.useState<boolean>(false);

  pageCallbackRouter.onBraveRewardsEnabledChanged.addListener((enabled: boolean) => {
    setRewardsEnabled(enabled);
  })
  API.createAdsInternalsPageHandler(pageCallbackRouter.$.bindNewPipeAndPassRemote());

  const getAdsInternals = React.useCallback(async () => {
    try {
      const response = await API.getAdsInternals()
      setAdsInternals(JSON.parse(response.response) || []);
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
        <ConversionUrlPatternsTable data={adsInternals} /><br /><br />
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
  return header === EXPIRES_AT_TABLE_COLUMN ? formatUnixEpochToLocalTime(row[header]) : row[header];
}

const ConversionUrlPatternsTable: React.FC<{ data: AdsInternal[] }> = React.memo(({ data }) => {
  if (data.length === 0) {
    return (
      <p>No Brave Ads conversion URL patterns are currently being matched.</p>
    );
  }

  const tableColumnOrder = [URL_PATTERN_TABLE_COLUMN, EXPIRES_AT_TABLE_COLUMN];

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

document.addEventListener('DOMContentLoaded', () => {
  render(<App />, document.getElementById('root'));
});
