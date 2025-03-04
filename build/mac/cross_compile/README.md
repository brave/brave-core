# Cross-compiling Brave

Out of the box, it is possible to compile unsigned Brave for macOS in Linux.
The code in this folder additionally makes it possible to codesign Brave, and
build DMG and PKG installers.

## Initialization

Use the following commands to prepare the build:

```
npm install
npm run init -- --target_os=mac --target_arch=x64
```

## Compiling Brave

After the above, you can compile Brave for macOS with the following command:

```
npm run build -- --target_os=mac --target_arch=x64
```

`Static` builds also work:

```
npm run build -- Static --target_os=mac --target_arch=x64
```

The current implementation does not yet support dSYM generation and symbol
stripping, so you need to disable them for `Release` builds:

```
npm run build -- Release --target_os=mac --target_arch=x64 \
    --gn enable_dsyms:false --gn enable_stripping:false
```

## Creating a DMG

Creating a DMG in Linux requires a tool called `libdmg-hfsplus` that is not
automatically checked out by default. To obtain it, set
`"checkout_dmg_tool": True` in the `custom_vars` section of the `src/brave`
solution in your `.gclient` file. Then, execute:

    npm run sync

Now you can create a DMG via the usual GN target:

```
npm run build -- Static --target_os=mac --target_arch=x64 \
    --target=brave/build/mac:create_dmg
```

## For PKGs and code signing: Get a macOS host

Not all parts of the build have been ported to run on Linux. The solution is to
invoke those commands that are currently only available on macOS via SSH on a
macOS host that has the `src/` directory mounted inside it.

Let's assume that you can SSH into your macOS machine via the command
`ssh mymac`. Let us further assume that, inside that machine, your local `src/`
directory is mounted at `/ChromiumSrc`. Set the following environment variables:

```
export MACOS_HOST=mymac
export MACOS_SRC_DIR_MOUNT=/ChromiumSrc
```

You also need to create a temporary directory for your build type. Let's say you
want to create a `Static` build. Then create the directory
`src/out/mac_Static/tmp` and set:

```
export TMPDIR=.../path/to/src/out/mac_Static/tmp
```

As of this writing, you do need to update `TMPDIR` when you switch to a
different build type. So if you want to build `Release`, then create
`src/out/mac_Release/tmp` and set `TMPDIR` to `.../src/out/mac_Release/tmp`.

The build produces temporary files. The above step makes sure that also those
temporary files are accessible to macOS under `/ChromiumSrc`.

## Create a PKG

After the above steps, you should be able to create a PKG installer for Brave
via the command:

```
npm run build -- Static --target_os=mac --target_arch=x64 \
    --target=brave/build/mac:create_pkg
```

## Code signing

Let's assume that you have a code signing certificate called `MyCert`. Set the
following additional environment variable to the password of the keychain where
the certificate is stored:

```
export KEYCHAIN_PASSWORD=...
```

If the certificate is stored in the login keychain, then no further environment
variables are required. Otherwise, set `KEYCHAIN_PATH` to the path of the
keychain. For example:

```
export KEYCHAIN_PATH=/Users/michael/Library/Keychains/signing.keychain-db
```

Now you can codesign Brave via:

```
npm run build -- Static --target_os=mac --target_arch=x64 \
    --mac_signing_identifier=MyCert --target=brave/build/mac:sign_app
```

## Implementation

The trick that makes the build work on Linux even when some commands run on
macOS is that the implementation adds stub implementations of common macOS
command-line tools such as `codesign` to the `PATH`. See `run-on-macos.py` and
the `path/` folder in this directory.
