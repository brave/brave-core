// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'path'
import { forkTsChecker } from './options'
import { StorybookConfig } from '@storybook/react-webpack5'
import type { Indexer } from '@storybook/types';
import { loadCsf } from '@storybook/csf-tools';
import { readFile } from 'fs/promises';

const slashStoriesIndexer: Indexer = {
  test: /stories\/.*\.tsx$/,
  createIndex: async (fileName, opts) => {
    const code = (await readFile(fileName, 'utf-8')).toString();
    const result = loadCsf(code, { ...opts, fileName }).parse()
    return result.indexInputs;
  }
}

const config: StorybookConfig = {
  stories: process.env.STORYBOOK_STORYPATH
    ? [`../${process.env.STORYBOOK_STORYPATH}`]
    : [
      '../components/**/stories/*.tsx',
      '../components/**/*.stories.tsx',
      '../browser/resources/**/stories/*.tsx'
    ],
  typescript: {
    check: false,
    reactDocgen: false,
    checkOptions: {
      async: forkTsChecker,
      typescript: {
        configFile: path.resolve(__dirname, '..', 'tsconfig-storybook.json')
      }
    }
  },
  addons: [ '@storybook/addon-knobs', '@storybook/addon-essentials' ],
  framework: '@storybook/react-webpack5',
  staticDirs: [
    { from: '../node_modules/@brave/leo/icons', to: 'icons/' },
    {
      from: '../components/playlist/browser/resources/stories/assets',
      to: 'playlist/'
    }
  ],
  core: {
    disableTelemetry: true
  },
  env: (config) => ({
    ...config,
    NODE_ENV: 'test'
  }),
  experimental_indexers: async (existing) => [...existing, slashStoriesIndexer]
}

export default config
