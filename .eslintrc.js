// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

module.exports = {
  'root': true /* Don't read chromium's eslint config (https://eslint.org/docs/user-guide/configuring/configuration-files#cascading-and-hierarchy) */,
  'extends': ['standard-with-typescript', 'prettier'],
  'ignorePatterns': [
    '.storybook/*',
    'build/*',
    'browser/*',
    'ui/webui/resources/*',
    '*.js',
    '*.d.ts',
    '!components/playlist/resources/media_detector/*.js' /* allow js scripts which will be bundled into playlist */
  ],
  'env': {
    'browser': false,
    'node': true,
    'es6': true,
    'jest/globals': true
  },
  'plugins': ['jest', 'licenses'],
  'globals': {
    'chrome': 'readonly'
  },
  'parserOptions': {
    'project': './tsconfig-lint.json'
  },
  'rules': {
    'licenses/header': [
      2,
      {
        'tryUseCreatedYear': true,
        'comment': {
          'allow': 'both',
          'prefer': 'line'
        },
        'header': [
          'Copyright (c) {YEAR} The Brave Authors. All rights reserved.',
          'This Source Code Form is subject to the terms of the Mozilla Public',
          'License, v. 2.0. If a copy of the MPL was not distributed with this file,',
          'You can obtain one at https://mozilla.org/MPL/2.0/.'
        ],
        'altHeaders': [
          [
            'Copyright (c) {YEAR} The Brave Authors. All rights reserved.',
            '* This Source Code Form is subject to the terms of the Mozilla Public',
            '* License, v. 2.0. If a copy of the MPL was not distributed with this file,',
            '* You can obtain one at https://mozilla.org/MPL/2.0/.'
          ],
          [
            'This Source Code Form is subject to the terms of the Mozilla Public',
            '* License, v. 2.0. If a copy of the MPL was not distributed with this file,',
            '* You can obtain one at https://mozilla.org/MPL/2.0/.'
          ],
          [
            'Copyright (c) {YEAR} The Brave Authors. All rights reserved.',
            'This Source Code Form is subject to the terms of the Mozilla Public',
            'License, v. 2.0. If a copy of the MPL was not distributed with this file,',
            'you can obtain one at https://mozilla.org/MPL/2.0/.'
          ]
        ]
      }
    ],
    'object-shorthand': 0,
    'n/no-callback-literal': 0,
    '@typescript-eslint/await-thenable': 0,
    '@typescript-eslint/consistent-generic-constructors': 0,
    '@typescript-eslint/ban-ts-comment': 0,
    '@typescript-eslint/consistent-indexed-object-style': 0,
    '@typescript-eslint/no-confusing-void-expression': 0,
    '@typescript-eslint/ban-types': 0,
    '@typescript-eslint/consistent-type-imports': 0,
    '@typescript-eslint/consistent-type-exports': 0,
    '@typescript-eslint/indent': 0,
    '@typescript-eslint/no-useless-constructor': 0,
    '@typescript-eslint/explicit-function-return-type': 0,
    'import/first': 0,
    '@typescript-eslint/no-var-requires': 0,
    'consistent-type-definitions': 0,
    '@typescript-eslint/strict-boolean-expressions': 0,
    '@typescript-eslint/restrict-template-expressions': 0,
    '@typescript-eslint/restrict-plus-operands': 0,
    '@typescript-eslint/prefer-optional-chain': 0,
    '@typescript-eslint/prefer-nullish-coalescing': 0,
    '@typescript-eslint/no-misused-promises': 0,
    'no-mixed-operators': 0,
    'no-prototype-builtins': 0,
    '@typescript-eslint/promise-function-async': 0,
    'no-case-declarations': 0,
    '@typescript-eslint/no-dynamic-delete': 0,
    '@typescript-eslint/no-empty-interface': 0,
    'no-useless-escape': 0,
    'no-return-assign': 0,
    'no-async-promise-executor': 0,
    'no-fallthrough': 0,
    'array-callback-return': 0,
    'prefer-promise-reject-errors': 0,
    '@typescript-eslint/no-floating-promises': 0,
    '@typescript-eslint/no-base-to-string': 0,
    'prefer-regex-literals': 0,
    '@typescript-eslint/no-implied-eval': 0,
    '@typescript-eslint/no-namespace': 0,
    '@typescript-eslint/require-array-sort-compare': 0,
    'no-loss-of-precision': 0,
    '@typescript-eslint/no-this-alias': 0,
    '@typescript-eslint/no-redeclare': 0,
    'no-unsafe-negation': 0,
    'promise/param-names': 0,
    'node/no-callback-literal': 0,
    '@typescript-eslint/consistent-type-definitions': 0,
    'multiline-ternary': 0,
    '@typescript-eslint/prefer-readonly': 0,
    /* TODO(nullhook): ENABLE the below rules in the future */
    'no-useless-call': 0,
    '@typescript-eslint/consistent-type-assertions': 0,
    '@typescript-eslint/no-non-null-assertion': 0,
    '@typescript-eslint/no-invalid-void-type': 0,
    'prefer-const': 0,
    '@typescript-eslint/return-await': 0
  }
}
