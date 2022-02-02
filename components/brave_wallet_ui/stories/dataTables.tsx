/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import { withKnobs, object } from '@storybook/addon-knobs'

// Components
import Table, { Header, Row } from '../components/shared/datatable'

storiesOf('Wallet/Desktop/Components', module)
  .addDecorator(withKnobs)
  .add('DataTable', () => {
    const rowTheme = {

    }

    const rows: Row[] = [
      {
        content: [
          {
            content: 'Baker'
          },
          {
            content: '40%',
            customStyle: rowTheme
          },
          {
            content: '4',
            customStyle: rowTheme
          }
        ]
      },
      {
        content: [
          {
            content: 'duckduckgo.com'
          },
          {
            content: '20%',
            customStyle: rowTheme
          },
          {
            content: '2',
            customStyle: rowTheme
          }
        ]
      }
    ]

    const header: Header[] = [
      {
        id: 'site',
        content: 'Site visited',
        sortable: true,
        sortOrder: 'desc'
      },
      {
        id: 'Attention',
        content: 'Attention',
        sortOrder: 'desc'
      },
      {
        id: 'tokens',
        content: 'Tokens',
        sortable: true
      }
    ]

    return (
      <div>
        <Table
          headers={object('Header', header)}
          rows={object('Rows', rows)}
        >
          404: Publishers not found :)
        </Table>
        <br />
      </div>
    )
  })
