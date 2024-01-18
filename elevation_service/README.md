## Elevation service

[Please see the documentation in upstream Chromium](https://source.chromium.org/chromium/chromium/src/+/main:chrome/elevation_service/README.md).

### When does this get registered

This Windows-specific service is registered for system level installs. There are two ways a system level install can be done:
- UAC prompt raised during Brave install (ex: person has admin privs) and person clicks `Yes` to accept escalation.
- Brave installer executable is invoked with `--system-level` argument in command line which has admin privs.


### What is the service used for?

Currently, the elevation service is only used to install the `Brave VPN` services once the browser detects the product was purchased.


### Debugging

The service executable is inside the versioned folder of Brave. It's possible to debug the service interactively by using the following procedure:

1. Open `regedit.exe`
2. Navigate to `[HKEY_CLASSES_ROOT\AppID\{5693E62D-00D6-4421-AFE8-58F3C947436A}]` (or if building another channel, [check here to find the right AppID](https://github.com/brave/brave-core/blob/master/chromium_src/chrome/install_static/chromium_install_modes.cc)).
3. Delete the `LocalService` value (should be set to something like `BraveDevelopmentElevationService`) and create a new String value called `RunAs` with the data value set to `Interactive User`.
4. Open an admin `cmd.exe` instance.
5. Navigate to the path where you built the elevation service executable (ex: `C:\brave\src\out\Component`).
6. Launch the exe via `elevation_service.exe --console`.
7. After debugging, don't forget to undo step 3.

These steps will let you run a locally compiled version of elevation service. This is very helpful for debugging or tracing the logic there.
