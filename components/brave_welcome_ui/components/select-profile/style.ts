// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const MainBox = styled.div`
  background: rgba(255, 255, 255, 0.1);
  backdrop-filter: blur(15px);
  border-radius: 30px;
  max-width: 800px;
  color: white;
  font-family: ${(p) => p.theme.fontFamily.heading};
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;

  .view-header-box {
    display: grid;
    grid-template-columns: 0.2fr 1fr 0.2fr;
    padding: 40px;
    margin-bottom: 50px;
    text-align: center;
  }

  .view-details {
    grid-column: 2;
  }

  .view-title {
    font-weight: 600;
    font-size: 36px;
    margin: 0 0 18px 0;
  }

  .view-desc {
    font-weight: 400;
    font-size: 16px;
    margin: 0;
  }

  .view-header-actions {
    grid-column: 1;
    padding-top: 10px;
  }

  .back-button {
    background: transparent;
    font-size: 16px;
    line-height: 20px;
    border: none;
    display: flex;
    align-items: center;
    gap: 15px;

    svg {
      width: 22px;
      height: auto;
    }
  }
`

export const ProfileListBox = styled.div`
  color: #212529;
  font-size: 16px;
  font-weight: 400;
  margin-bottom: 55px;

  p {
    margin: 0;
  }

  .profile-list {
    background: white;
    min-width: 520px;
    max-height: 420px;
    padding-bottom: 20px;
    border-radius: 16px;
    overflow-y: auto;
  }

  .list-actions {
    padding: 20px 32px;
    display: flex;
    justify-content: flex-end;
    background: white;
    position: sticky;
    top: 0;

    button {
      box-shadow: none;
      color: #4C54D2;
      background: transparent;
      border: 0;
      padding: 0;
    }
  }

  .item-box {
    background: transparent;

    &:hover {
      background: #F8F9FA;
    }
  }

  .item-grid {
    display: grid;
    grid-template-columns: 1fr 25px;
    padding: 10px 32px;
    align-items: center;
  }

  .item-info {
    display: flex;
    align-items: center;
    grid-gap: 15px;
  }

  .item-action {
    text-align: right;
  }

  .avatar {
    width: 48px;
    height: 48px;
    border-radius: 48px;

    svg {
      width: 100%;
      height: auto;
    }
  }

  input[type="checkbox"] {
    width: 22px;
    height: 22px;
    border-radius: 4px;
    border: 1px solid #AEB1C2;

    &:checked {
      accent-color: #4C54D2;
    }
  }
`

export const ActionBox = styled.div`
  display: flex;
  grid-gap: 10px;
  margin-bottom: 40px;
`
