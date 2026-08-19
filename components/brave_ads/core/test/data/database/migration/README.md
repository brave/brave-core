# Database Migration Test Data

> [!IMPORTANT]
> The brave_ads schema does not declare `FOREIGN KEY` constraints, so referential integrity between tables is not enforced by SQLite. `mock_database.py` maintains cross-table consistency for UUID columns (e.g. `campaign_id`, `creative_instance_id`) itself, via an entity pool that dependent tables draw from.

To mock a database with test data, create a corresponding SQLite database file by launching the development build of the browser. Then, copy this file from the default profile in `Default/ads_service` to this directory and execute `mock_database.py`. For example:

    python3 mock_database.py database_schema_1.sqlite

Use `--days`, `--rows`, `--min-rows`, and `--max-rows` to control how many rows are generated and how their timestamps are spread over time. `--rows` sets both `--min-rows` and `--max-rows` to the same value.

This approach tests how database migrations handle data integrity, schema flexibility, null handling, and migration robustness.

Please add to it!
