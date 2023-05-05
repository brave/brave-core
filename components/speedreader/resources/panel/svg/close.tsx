// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from "react";
const SvgClose = (props: any) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    width="16"
    height="16"
    viewBox="0 0 24 24"
    fill="none"
    {...props}
  >
    <path
      fill="currentColor"
      fillRule="evenodd"
      d="M5.992 5.992a.85.85 0 0 0 0 1.202L10.798 12l-4.81 4.81a.85.85 0 1 0 1.202 1.202l4.81-4.81 4.806 4.806a.85.85 0 0 0 1.202-1.202L13.202 12l4.81-4.81a.85.85 0 1 0-1.202-1.202L12 10.798 7.194 5.992a.85.85 0 0 0-1.202 0Z"
      clipRule="evenodd"
    />
  </svg>
);
export default SvgClose;

