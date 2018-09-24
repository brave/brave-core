/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as CSS from 'csstype'

interface Content {
  display: CSS.DisplayProperty
  flexDirection: CSS.FlexDirectionProperty
  justifyContent: CSS.JustifyContentProperty
  alignItems: CSS.AlignItemsProperty
  flex: CSS.FlexProperty<1>
  marginBottom: CSS.MarginBottomProperty<1>
}

export const theme = {
  panel: {
    backgroundColor: 'rgba(255,255,255,0.95)',
    maxWidth: '600px',
    height: '660px',
    display: 'flex',
    flexDirection: 'column',
    justifyContent: 'space-between'
  },
  braveLogo: {
    width: '120px'
  },
  paymentsImage: {
    width: '230px'
  },
  importImage: {
    width: '215px'
  },
  shieldsImage: {
    width: '170px'
  },
  featuresImage: {
    width: '300px'
  },
  title: {
    fontFamily: '"Poppins", sans-serif',
    fontSize: '32px',
    color: '#212121',
    margin: '40px 0 0'
  },
  text: {
    fontFamily: '"Muli", sans-serif',
    fontSize: '18px',
    color: '#76777A',
    lineHeight: '34px',
    textAlign: 'center',
    margin: '20px 0'
  },
  mainButton: {
    textTransform: 'uppercase'
  },
  sideButton: {
    textTransform: 'uppercase'
  },
  arrow: {
    display: 'inline-block',
    verticalAlign: 'middle'
  },
  content: {
    display: 'flex',
    flexDirection: 'column',
    justifyContent: 'center',
    alignItems: 'center',
    flex: '1',
    marginBottom: '40px'
  } as Content,
  skip: {
    color: '#76777A',
    textDecoration: 'underline'
  },
  footer: {
    gridGap: '0',
    padding: '0'
  },
  footerColumnLeft: {
    alignItems: 'center',
    justifyContent: 'flex-start'
  },
  footerColumnCenter: {
    alignItems: 'center',
    justifyContent: 'center'
  },
  footerColumnRight: {
    alignItems: 'center',
    justifyContent: 'flex-end'
  },
  bulletActive: {
    color: '#FB542B',
    hoverColor: '#FB542B',
    padding: '0 7px',
    fontSize: '40px'
  },
  bullet: {
    color: '#7D7D7D',
    hoverColor: '#FB542B',
    padding: '0 7px',
    fontSize: '40px'
  }
}
