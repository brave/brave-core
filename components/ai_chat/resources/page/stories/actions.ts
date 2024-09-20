// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as mojom from '../api/'

const ACTIONS_LIST: mojom.ActionGroup[] = [
  {
    category: 'Quick actions',
    entries: [
      {
        subheading: undefined,
        details: { label: 'Explain', type: mojom.ActionType.EXPLAIN }
      }
    ]
  },
  {
    category: 'Rewrite',
    entries: [
      {
        subheading: undefined,
        details: { label: 'Paraphrase', type: mojom.ActionType.PARAPHRASE }
      },
      { subheading: 'Change tone', details: undefined },
      {
        subheading: undefined,
        details: {
          label: 'Change tone / Academic',
          type: mojom.ActionType.ACADEMICIZE
        }
      },
      {
        subheading: undefined,
        details: {
          label: 'Change tone / Professional',
          type: mojom.ActionType.PROFESSIONALIZE
        }
      }
    ]
  }
]

export default ACTIONS_LIST
