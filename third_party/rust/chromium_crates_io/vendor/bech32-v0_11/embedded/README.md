## Embedded tests

These no-std tests are a bit peculiar.
They are cross compiled to ARM and run in an emulator.
Here's why:

 - We want to build for an exotic platform to help make sure `std` doesn't sneak in by accident.

 - We use an emulator and build something runnable,
   rather than merely testing whether a library builds,
   because we want to actually run our integration test.
