const path = require('path')
const webpack = require('webpack')
const fs = require('fs')

const buildConfigs = ['Component', 'Static', 'Debug', 'Release']
const extraArchitectures = ['arm64', 'x86']

function getBuildOuptutPathList(buildOutputRelativePath) {
  return buildConfigs.flatMap((config) => [
    path.resolve(__dirname, `../../out/${config}/${buildOutputRelativePath}`),
    ...extraArchitectures.map((arch) =>
      path.resolve(
        __dirname,
        `../../out/${config}_${arch}/${buildOutputRelativePath}`
      )
    )
  ])
}

// TODO(petemill): This is not ideal since if the dev is using a Static
// build, but has previously made a Component build, then an outdated
// version of a module will be used. Instead, accept a cli argument
// or environment variable containing which build target to use.
process.env.ROOT_GEN_DIR = getBuildOuptutPathList('gen').find(p => fs.existsSync(p))
const basePathMap = require('../components/webpack/path-map')

// Override the path map we use in the browser with some additional mock
// directories, so that we can replace things in Storybook.
const pathMap = {
  ...basePathMap,
  gen: [
    // Mock chromium code where possible
    path.resolve(__dirname, 'gen-mock'),
    process.env.ROOT_GEN_DIR
  ],
  'chrome://resources': [
    // Mock chromium code where possible
    path.resolve(__dirname, 'chrome-resources-mock'),
    // TODO(petemill): This is not ideal since if the dev is using a Static
    // build, but has previously made a Component build, then an outdated
    // version of a module will be used. Instead, accept a cli argument
    // or environment variable containing which build target to use.
    ...getBuildOuptutPathList('gen/ui/webui/resources/tsc')
  ]
}

/**
 * Maps a prefix to a corresponding path. We need this as Webpack5 dropped
 * support for scheme prefixes (like chrome://)
 * @param {string} prefix The prefix
 * @param {string[]} replacements The real path options
 */
const prefixReplacer = (prefix, replacements) => {
  const regex = new RegExp(`^${prefix}/(.*)`)
  return new webpack.NormalModuleReplacementPlugin(regex, resource => {
    resource.request = resource.request.replace(regex, (_, subpath) => {
      if (!subpath) {
        throw new Error("Subpath is undefined")
      }

      const match = replacements.find((r) => fs.existsSync(require.resolve(path.join(r, subpath)))) ?? replacements[0]
      const result = path.join(match, subpath)
      return result
    })
  })
}

// Export a function. Accept the base config as the only param.
module.exports = async ({ config, mode }) => {
  const isDevMode = mode === 'development'
  // Make whatever fine-grained changes you need
  config.module.rules.push(
    {
      test: /\.scss$/,
      include: [/\.global\./],
      use: [{ loader: 'style-loader' }, { loader: 'css-loader' }]
    },
    {
      test: /\.scss$/,
      exclude: [/\.global\./, /node_modules/],
      use: [
        { loader: 'style-loader' },
        {
          loader: 'css-loader',
          options: {
            importLoaders: 3,
            sourceMap: false,
            modules: {
              localIdentName: isDevMode
                ? '[path][name]__[local]--[hash:base64:5]'
                : '[hash:base64]'
            }
          }
        },
        { loader: 'sass-loader' }
      ]
    },
    {
      test: /\.(ts|tsx)$/,
      loader: 'ts-loader',
      options: {
        // TODO(petemill): point to the tsconfig in gen/[target] that
        // is made during build-time, or generate a new one. For both those
        // options, use a cli arg or environment variable to obtain the correct
        // build target.
        configFile: path.resolve(__dirname, '..', 'tsconfig-storybook.json'),
        getCustomTransformers: path.join(
          __dirname,
          '../components/webpack/webpack-ts-transformers.js'
        )
      }
    },
    {
      test: /\.avif$/,
      loader: 'file-loader'
    }
  )

  config.resolve.alias = pathMap
  config.resolve.fallback = {
    stream: require.resolve("stream-browserify"),
    path: require.resolve("path-browserify"),
    querystring: require.resolve("querystring-es3"),
    crypto: require.resolve("crypto-browserify"),
    os: require.resolve("os-browserify/browser"),
    zlib: require.resolve("browserify-zlib"),
    fs: false,
    http: require.resolve("stream-http"),
    https: require.resolve("https-browserify"),
    timers: require.resolve('timers-browserify'),
    buffer: require.resolve('buffer'),
    process: require.resolve('process/browser'),
  }

  config.plugins.push(
    // Provide globals from NodeJs polyfills
    new webpack.ProvidePlugin({
      'Buffer': ['buffer', 'Buffer'],
      'process': 'process/browser.js'
    }),
    prefixReplacer('chrome://resources/brave/fonts', getBuildOuptutPathList('gen/brave/ui/webui/resources/fonts')),
    prefixReplacer('chrome://resources/brave', getBuildOuptutPathList('gen/brave/ui/webui/resources/tsc')),
    prefixReplacer('chrome://resources', getBuildOuptutPathList('gen/ui/webui/resources/tsc')),
  )
  config.resolve.extensions.push('.ts', '.tsx', '.scss')
  return config
}
