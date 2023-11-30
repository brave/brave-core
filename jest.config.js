/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// For a detailed explanation regarding each configuration property, visit:
// https://jestjs.io/docs/en/configuration.html

const buildConfigs = [ 'Component', 'Static', 'Debug', 'Release' ]
const extraArchitectures = [ 'arm64', 'x86' ]

function getBuildOutputPathList(buildOutputRelativePath) {
  return buildConfigs.flatMap(config => [
    `<rootDir>/../out/${config}/${buildOutputRelativePath}`,
    ...extraArchitectures.map(arch =>
      `<rootDir>/../out/${config}_${arch}/${buildOutputRelativePath}`,
    )
  ])
}

module.exports = {
  preset: "ts-jest/presets/default",
  testEnvironment: "<rootDir>/components/test/testEnvironment.js",
  moduleFileExtensions: [
    "js",
    "tsx",
    "ts",
    "json"
  ],
  globals: {
    "ts-jest": {
      "tsconfig": "tsconfig-jest.json",
      "isolatedModules": true
    }
  },
  transform: {
    ".(jsx|js|ts|tsx)": "ts-jest"
  },
  clearMocks: true,
  resetMocks: true,
  resetModules: true,
  coverageProvider: 'v8',
  collectCoverage: true,
  collectCoverageFrom: [
    "<rootDir>/build/commands/lib/*",
    "<rootDir>/components/**/**/*.ts",
    "<rootDir>/components/**/**/*.tsx",
    "!<rootDir>/components/definitions/*",
    "!<rootDir>/components/**/constants/*",
    "!<rootDir>/components/**/reducers/index.ts",
    "!<rootDir>/components/**/store.ts",
    "!<rootDir>/components/test/*",
    "!<rootDir>/build/commands/lib/start.js",
    "!<rootDir>/build/commands/lib/jsconfig.json"
  ],
  testURL: "http://localhost/",
  testMatch: [
    "<rootDir>/**/*.test.{js,ts,tsx}",
    "<rootDir>/components/test/**/*_test.{ts,tsx}"
  ],
  testPathIgnorePatterns: [
    "lib/test.js"
  ],
  testTimeout: 30000,
  transformIgnorePatterns: [
    "<rootDir>/node_modules/(?!(@brave/brave-ui|@brave/leo)/)"
  ],
  setupFilesAfterEnv: [
    "<rootDir>/components/test/testSetup.ts"
  ],
  setupFiles: [
    "<rootDir>/components/test/testPolyfills.ts"
  ],
  moduleNameMapper: {
    "\\.(jpg|jpeg|png|gif|eot|otf|svg|ttf|woff|woff2)$": "<rootDir>/components/test/fileMock.ts",
    "\\.(css|less|scss)$": "identity-obj-proxy",
    "^\\$web-common\\/(.*)": "<rootDir>/components/common/$1",
    "^\\$web-components\\/(.*)": "<rootDir>/components/web-components/$1",
    "^brave-ui$": "<rootDir>/node_modules/@brave/brave-ui",
    "^brave-ui\\/(.*)": "<rootDir>/node_modules/@brave/brave-ui/$1",
    // TODO(petemill): The ordering here can get problematic for devs
    // who have more than 1 build type at a time, since if the file exists
    // at the first path, it will be used for Type analysis instead of the second
    // path, even if it's more recent.
    // It can also break if CI or devs perform a build in a directory not known
    // by this list.
    // Instead, we should get the directory from config.js:outputDir.
    "^gen\\/(.*)": getBuildOutputPathList('gen/$1'),
    "chrome:\/\/resources\\/(.*)":
      getBuildOutputPathList('gen/ui/webui/resources/tsc/$1'),
    "chrome:\/\/interstitials\\/(.*)": getBuildOutputPathList(
      'gen/components/security_interstitials/core/$1'
    )
  }
}
