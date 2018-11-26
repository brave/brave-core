# bat-native-ads

## Isolated development on macOS

### Pre-requisite

- Xcode command line tools must be installed
- You must be in the working directory of BAT Native Ads, i.e. `bat-native-ads`

#### Install homebrew

```
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" < /dev/null 2> /dev/null
```

#### Install cmake

```
brew install cmake
```

#### Create build folder

```
mkdir build
```

#### Generate Makefile

Which should be called for any project changes, i.e. adding new files

```
cd build

cmake ..
```

#### Build executable

```
cd build

make
```

#### Debugging

You can attach a debugger to the `batnativeads` executable which can be found in the `build` folder

#### Isolated Google Tests

```
cd build

make test
```

You can also add the `ARGS="-V"` command line argument for verbose output

```
cd build

make test ARGS="-V"
```
