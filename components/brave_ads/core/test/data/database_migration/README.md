# Database Migration Test Data

> [!IMPORTANT]
> The script currently disables foreign keys while mocking the database. This could potentially mask issues related to foreign key constraints during testing. See https://github.com/brave/brave-browser/issues/40017.

To mock a database with test data, create a corresponding SQLite database file by launching the development build of the browser. Then, copy this file from the default profile in `Default/ads_service` to this directory and execute `mock_database.py`. For example:

    python3 mock_database.py database_schema_1.sqlite

This approach tests how database migrations handle data integrity, schema flexibility, null handling, and migration robustness.

Please add to it!
