# bat-native-ads

## Isolated development on macOS

### Pre-requisite

- Xcode command line tools must be installed
- You must be in the working directory of BAT Native Ads, i.e. `bat-native-ads`

#### Install homebrew

```
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" < /dev/null 2> /dev/null
```

#### Install cmake (3.12 or above required)

```
brew install cmake
```

or

```
brew upgrade cmake
```

#### Create build folder

```
mkdir build
```

#### Generate Makefile

You must be in the `build` directory of BAT Native Ads before running the
following commands. If any project changes are made the `Makefile` may need to
be re-generated

```
cmake ..
```

#### Build executable

You must be in the `build` directory of BAT Native Ads before running the
following commands

```
make
```

#### Debugging

You can attach a debugger to the `batnativeads` executable which can be
found in the `build` folder

#### Isolated Google Tests

You must be in the `build` directory of BAT Native Ads before running the
following commands

```
make test
```

You can also add the `ARGS="-V"` command line argument for verbose output

```
make test ARGS="-V"
```
