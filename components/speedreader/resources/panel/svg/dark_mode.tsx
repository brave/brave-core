// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from "react";
const SvgDarkMode = (props: any) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    width="18"
    height="18"
    viewBox="0 0 18 18"
    fill="none"
    {...props}
  >
    <path fill="#000" d="M9 12.6a3.6 3.6 0 0 0 0-7.2v7.2Z" />
    <path
      fill="#000"
      fillRule="evenodd"
      d="M9 0a9 9 0 1 0 0 18A9 9 0 0 0 9 0Zm0 1.8v3.6a3.6 3.6 0 0 0 0 7.2v3.6A7.2 7.2 0 0 0 9 1.8Z"
      clipRule="evenodd"
    />
  </svg>
);
export default SvgDarkMode;

