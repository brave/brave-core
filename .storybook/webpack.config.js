const path = require('path')
const webpack = require('webpack')
const fs = require('fs')
const {
  fallback,
  provideNodeGlobals
} = require('../components/webpack/polyfill')

const buildConfigs = ['Component', 'Static', 'Debug', 'Release']
const extraArchitectures = ['arm64', 'x86']

// Choose which build path to use
function getActiveBuildPath() {
  /**
   * @typedef PathWithStat
   * @type {object}
   * @property {string} path
   * @property {fs.Stats} stat
   */
  /**
   * @type {PathWithStat}
   */
  let latestExisting
  function checkPath(path) {
    const stat = fs.statSync(path, {throwIfNoEntry: false})
    if (stat?.isDirectory) {
      if (
        !latestExisting ||
        stat.mtime.getTime() > latestExisting.stat.mtime.getTime()
      ) {
        latestExisting = { path, stat }
      }
    }
  }
  for (const config of buildConfigs) {
    const base = path.resolve(__dirname, '..', '..', 'out')
    checkPath(path.join(base, config))
    for (const arch of extraArchitectures) {
      checkPath(path.join(base, `${config}_${arch}`))
    }
  }
  return latestExisting?.path
}

const buildPath = getActiveBuildPath()
if (!buildPath) {
  throw new Error('Could not establish active build path')
}
console.log('Using build path at: ', buildPath)
process.env.ROOT_GEN_DIR = path.join(buildPath, 'gen')
const pathMap = require('../components/webpack/path-map')

// As we mock some chrome://resources, insert our mock directory as the first
// place to look.
pathMap['gen'] = [
  path.resolve(__dirname, 'gen-mock'),
  pathMap['gen']
]
pathMap['chrome://resources'] = [
  path.resolve(__dirname, 'chrome-resources-mock'),
  pathMap['chrome://resources']
]

/**
 * Maps a prefix to a corresponding path. We need this as Webpack5 dropped
 * support for scheme prefixes (like chrome://)
 *
 * Note: This prefixReplacer is slightly different from the one we use in proper
 * builds, as it takes the first match from any build folder, rather than
 * specifying one - we don't know what the user built last, so we just see what
 * we can find.
 *
 * This isn't perfect, and in future it'd be good to pass the build folder in
 * via an environment variable. For now though, this works well.
 * @param {string} prefix The prefix
 * @param {string[]} replacement The real path
 */
const prefixReplacer = (prefix, replacement) => {
  const regex = new RegExp(`^${prefix}/(.*)`)
  return new webpack.NormalModuleReplacementPlugin(regex, (resource) => {
    resource.request = resource.request.replace(regex, (_, subpath) => {
      if (!subpath) {
        throw new Error('Subpath is undefined')
      }
      let match = replacement
      if (Array.isArray(replacement)) {
        match = replacement.find((r) => fs.existsSync(require.resolve(path.join(r, subpath))))
        if (!match) {
          throw new Error('Could not find path that exists with first entry', replacement[0])
        }
      }

      return path.join(match, subpath)
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
  config.resolve.fallback = fallback
  config.plugins.push(
    provideNodeGlobals,
    ...Object.keys(pathMap)
      .filter(prefix => prefix.startsWith('chrome://'))
      .map((prefix) =>
        prefixReplacer(prefix, pathMap[prefix]))
  )
  config.resolve.extensions.push('.ts', '.tsx', '.scss')
  return config
}
