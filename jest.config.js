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
    // "^chrome:\/\/resources\\/(.*)": "<rootDir>/.storybook/chrome-resources-mock/$1",
    // "^gen\\/(.*)": "<rootDir>/.storybook/gen-mock/$1",
    "^gen\\/(.*)": [
      // TODO(petemill): The ordering here can get problematic for devs
      // who have more than 1 build type at a time, since if the file exists
      // at the first path, it will be used for Type analysis instead of the second
      // path, even if it's more recent. We should investigate a test-time
      // command / environment variable which would use the correct path here.
      "<rootDir>/../out/Component/gen/$1",
      "<rootDir>/../out/Static/gen/$1",
      "<rootDir>/../out/Release/gen/$1",
      "<rootDir>/../out/Debug/gen/$1",
    ],
    "chrome:\/\/resources\\/(.*)": [
      "<rootDir>/../out/Component/gen/ui/webui/resources/preprocessed/$1",
      "<rootDir>/../out/Static/gen/ui/webui/resources/preprocessed/$1",
      "<rootDir>/../out/Release/gen/ui/webui/resources/preprocessed/$1",
      "<rootDir>/../out/Debug/gen/ui/webui/resources/preprocessed/$1",
    ]
  }
}
