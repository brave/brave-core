# Database Migration

Migrate legacy database state. You **MUST** increment `kVersion` and `kCompatibleVersion` in [database_constants.h](../../legacy_migration/database/database_constants.h) and add corresponding `database_schema_{#}.sqlite` and `invalid_database_schema_{#}.sqlite` files to `brave/components/brave_ads/test/data/database_migration`.

See [database migration test data](../../../test/data/database_migration/README.md) on how to mock test data.

Please add to it!
