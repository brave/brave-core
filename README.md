# bat-native-ads

## Command-line switches

Use production Ads Serve as defined by `PRODUCTION_SERVER` in `static_values.h`. Default for Official Builds

```
--brave-ads-production
```

Use staging Ads Serve as defined by `STAGING_SERVER` in `static_values.h`. Default for non Office Builds, i.e. Debug

```
--brave-ads-staging
```

Collect initial activity after 25 seconds instead of 1 hour as defined by `kDebugOneHourInSeconds` in `static_values.h`

```
--brave-ads-debug
```

Enable testing of notifications while viewing `www.iab.com` so that page refreshes force a notification to show after 30 seconds as defined by `kNextEasterEggStartsInSeconds` in `static_values.h`

```
--brave-ads-testing
```

Enable diagnostic logging, where `#` should set to a minimum log level. Valid values are from 0 to 3 where INFO = 0, WARNING = 1, ERROR = 2 and FATAL = 3. So if you want INFO, WARNING and ERROR you would choose 2

```
--enable-logging --log-level=#
```

i.e. to launch using Staging Server, Easter Egg and logging for INFO, WARNING and ERROR on macOS open the `Terminal` application and enter the below commands,:

```
cd /Applications

cd Brave\ Browser\ Beta.app/

cd Contents

./Brave\ Browser\ Development --brave-ads-staging --brave-ads-testing --enable-logging --log-level=2
```

## Initial Limitations

- Catalog is downloaded twice upon startup
- Clicking Ads in the Notification Center do not open the Ad
- Brave Ads notifications disappear without user input
- For Linux:
    - In "debug" mode, notifications cause crashes
    - Foreground/Background notifications aren't implemented on Brave Core
- `EventLog`'s are persisted in the Console Log, whereas in Muon they were persisted in a JSON file
- Implement DemoAPI logs
- Additional unit test coverage
- Migrate Mocks and Tests to Brave Core

All of these will be fixed _very soon!_

## Unscheduled future features

- `IsNotificationsAvailable` always returns `true` from Brave Core irrespective if they are enabled or disabled on the operating system

## Isolated development on macOS

- Deprecated as building and testing of the library will be moved to Brave Core

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

You must be in the `build` directory of BAT Native Ads before running the following commands. If any project changes are made the `Makefile` may need to
be re-generated

```
cmake ..
```

#### Build executable

You must be in the `build` directory of BAT Native Ads before running the following commands

```
make
```

#### Debugging

You can attach a debugger to the `batnativeads` executable which can be found in the `build` folder

#### Isolated Google Tests

You must be in the `build` directory of BAT Native Ads before running the following commands

```
make test
```

You can also add the `ARGS="-V"` command line argument for verbose output

```
make test ARGS="-V"
```
