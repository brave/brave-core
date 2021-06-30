// For a detailed explanation regarding each configuration property, visit:
// https://jestjs.io/docs/en/configuration.html

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
      "tsConfig": "tsconfig-jest.json",
      "isolatedModules": true
    }
  },
  transform: {
    ".(jsx|js|ts|tsx)": "ts-jest"
  },
  clearMocks: true,
  resetMocks: true,
  resetModules: true,
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
  transformIgnorePatterns: [
    "<rootDir>/node_modules/(?!(brave-ui)/)"
  ],
  setupFilesAfterEnv: [
    "<rootDir>/components/test/testSetup.ts"
  ],
  setupFiles: [
    "<rootDir>/components/test/testPolyfills.ts"
  ],
  moduleNameMapper: {
    "\\.(jpg|jpeg|png|gif|eot|otf|svg|ttf|woff|woff2)$": "<rootDir>/components/test/fileMock.ts",
    "\\.(css|less)$": "identity-obj-proxy",
    "^brave-ui$": "<rootDir>/node_modules/brave-ui/src",
    "^brave-ui\\/(.*)": "<rootDir>/node_modules/brave-ui/src/$1",
    "^chrome:\/\/resources\\/(.*)": "<rootDir>/.storybook/chrome-resources-mock/$1"
  }
}
