# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# This script is used to mock a SQLite database.

from datetime import datetime, timedelta
import argparse
import os
import secrets
import sqlite3
import string
import sys
import uuid

DEFAULT_TEXT_COLUMN_LENGTH = 32
DEFAULT_LONGVARCHAR_COLUMN_LENGTH = 64

MILLISECONDS_IN_SECOND = 1000

DEFAULT_DATE_RANGE_DAYS = 90
DEFAULT_MIN_ROW_COUNT = 1
DEFAULT_MAX_ROW_COUNT = 1

# Cycling patterns: (should_mock_column_nulls, should_use_random_test_data)
_ROW_PATTERNS = [
    (True, True),
    (True, False),
    (False, True),
    (False, False),
]

# Valid values for columns that have a constrained set of values parsed via
# strict enum converters (i.e. those that trigger NOTREACHED on unknown input).
# Keys are (table_name, column_name); use None as the table_name to match the
# column in any table.
COLUMN_VALID_VALUES = {
    # ConfirmationType: parsed via ToMojomConfirmationType which NOTREACHes on
    # unknown values.
    (None, 'confirmation_type'): [
        'click', 'dismiss', 'view', 'served', 'landed', 'bookmark', 'flag',
        'upvote', 'downvote', 'conversion', 'interaction', 'media_play',
        'media_25', 'media_100'
    ],
    # confirmation_queue.type is a ConfirmationType despite the generic name.
    ('confirmation_queue', 'type'): [
        'click', 'dismiss', 'view', 'served', 'landed', 'bookmark', 'flag',
        'upvote', 'downvote', 'conversion', 'interaction', 'media_play',
        'media_25', 'media_100'
    ],
    # AdType: parsed via ToMojomAdType which NOTREACHes on unknown values.
    (None, 'ad_type'): [
        'ad_notification', 'new_tab_page_ad', 'search_result_ad'
    ],
    # ad_events.type and ad_history.type are AdType despite the generic name.
    ('ad_events', 'type'): [
        'ad_notification', 'new_tab_page_ad', 'search_result_ad'
    ],
    ('ad_history', 'type'): [
        'ad_notification', 'new_tab_page_ad', 'search_result_ad'
    ],
    # Segment taxonomy strings used in segments, ad_events, ad_history, and
    # transactions tables.
    (None, 'segment'): [
        'arts & entertainment',
        'automotive',
        'business',
        'careers',
        'cell phones',
        'crypto',
        'education',
        'family & parenting',
        'fashion',
        'food & drink',
        'gaming',
        'health & fitness',
        'home',
        'personal finance',
        'pets',
        'science',
        'shopping',
        'sports',
        'technology & computing',
        'travel',
    ],
    # ISO 3166-1 alpha-2 country codes used in geo_targets.
    (None, 'geo_target'): [
        'AU',
        'CA',
        'DE',
        'FR',
        'GB',
        'JP',
        'NL',
        'NZ',
        'US',
    ],
    # campaigns.metric_type: only 'confirmation' is a valid value.
    ('campaigns', 'metric_type'): ['confirmation'],
}

# Column names that store UUIDs (parsed via base::Uuid::ParseLowercase in
# production code). Applies to any table unless overridden by
# COLUMN_UUID_TABLE_NAMES. Random alphanumeric strings would fail the parse,
# so UUID columns always use uuid.uuid4() regardless of the row pattern.
COLUMN_UUID_NAMES = {
    'placement_id',
    'campaign_id',
    'creative_set_id',
    'creative_instance_id',
    'advertiser_id',
    'transaction_id',
    'confirmation_id',
}

# (table_name, column_name) pairs whose `id` column is a UUID primary key.
COLUMN_UUID_TABLE_NAMES = {
    ('campaigns', 'id'),
    ('transactions', 'id'),
}

# Maps column_name → entity pool key for FK columns that must draw from a pool.
# Columns listed here will use an already-populated entity pool instead of
# generating a new random UUID, ensuring cross-table UUID consistency.
COLUMN_ENTITY_KEY = {
    'campaign_id': 'campaign',
    'creative_set_id': 'creative_set',
    'creative_instance_id': 'creative_instance',
    'advertiser_id': 'advertiser',
    'transaction_id': 'transaction',
    'placement_id': 'placement',
    'confirmation_id': 'confirmation',
}

# Maps (table_name, column_name) → entity pool key for columns that are the
# source of an entity pool. Values inserted here populate the pool so that
# dependent tables can draw consistent UUIDs from it.
COLUMN_ENTITY_SOURCE = {
    ('campaigns', 'id'): 'campaign',
    ('campaigns', 'advertiser_id'): 'advertiser',
    ('creative_ads', 'creative_instance_id'): 'creative_instance',
    ('creative_ads', 'creative_set_id'): 'creative_set',
    ('transactions', 'id'): 'transaction',
}

# Insertion order ensures entity-source tables are populated before tables that
# draw from their pools. Tables not listed here are appended at the end.
_TABLE_INSERTION_ORDER = [
    'campaigns',
    'creative_ads',
    'confirmation_tokens',
    'payment_tokens',
    'transactions',
    'confirmation_queue',
    'dayparts',
    'geo_targets',
    'segments',
    'deposits',
    'creative_set_conversions',
    'creative_ad_notifications',
    'creative_new_tab_page_ads',
    'ad_events',
    'ad_history',
]


def get_column_is_uuid(table_name, column_name):
    return (column_name in COLUMN_UUID_NAMES
            or (table_name, column_name) in COLUMN_UUID_TABLE_NAMES)


def _sort_tables_for_insertion(table_names):
    ordered_set = {t: i for i, t in enumerate(_TABLE_INSERTION_ORDER)}
    known = sorted([t for t in table_names if t in ordered_set],
                   key=lambda t: ordered_set[t])
    unknown = [t for t in table_names if t not in ordered_set]
    return known + unknown


def generate_mock_string(length):
    characters = string.ascii_letters + string.digits
    random_string = ''.join(secrets.choice(characters) for _ in range(length))
    return random_string


def generate_mock_blob():
    return secrets.token_bytes(128)


def generate_mock_chrome_webkit_timestamp(days_ago=0):
    now = datetime.utcnow()
    day_start = now - timedelta(days=days_ago + 1)
    random_seconds = secrets.randbelow(86400)
    random_date = day_start + timedelta(seconds=random_seconds)

    chrome_webkit_epoch = datetime(
        1601, 1, 1)  # To be, or not to be, that is the question.
    chrome_webkit_delta = random_date - chrome_webkit_epoch
    chrome_webkit_delta_in_seconds = int(chrome_webkit_delta.total_seconds())

    return chrome_webkit_delta_in_seconds * MILLISECONDS_IN_SECOND


def get_column_valid_values(table_name, column_name):
    # Check for a table-specific override first, then fall back to a
    # column-name-only match (None key).
    return (COLUMN_VALID_VALUES.get((table_name, column_name))
            or COLUMN_VALID_VALUES.get((None, column_name)))


def generate_mock_column_with_random_test_data(connection,
                                               table_name,
                                               column_name,
                                               column_type,
                                               days_ago=0):
    valid_values = get_column_valid_values(table_name, column_name)
    if valid_values:
        return secrets.choice(valid_values)

    if column_type in ('INTEGER', 'INT', 'NUMERIC'):
        mock_column = secrets.randbelow(100)
    elif column_type == 'TEXT':
        if get_column_is_uuid(table_name, column_name):
            mock_column = str(uuid.uuid4())
        else:
            mock_column = generate_mock_string(DEFAULT_TEXT_COLUMN_LENGTH)
    elif column_type == 'LONGVARCHAR':
        longvarchar_length = get_longvarchar_length(connection, table_name,
                                                    column_name)
        string_length = longvarchar_length[0][
            0] if longvarchar_length else DEFAULT_LONGVARCHAR_COLUMN_LENGTH
        mock_column = generate_mock_string(string_length)
    elif column_type in ('REAL', 'DOUBLE'):
        mock_column = round(secrets.SystemRandom().uniform(0.0, 1.0), 1)
    elif column_type == 'BLOB':
        mock_column = generate_mock_blob()
    elif column_type in ('DATE', 'TIMESTAMP'):
        mock_column = generate_mock_chrome_webkit_timestamp(days_ago)
    else:
        sys.exit(f"ERROR: Unsupported column type {column_type}")

    return mock_column


def generate_mock_column_with_fixed_test_data(column_type):
    if column_type in ('INTEGER', 'INT', 'NUMERIC'):
        mock_column = 0
    elif column_type in ('TEXT', 'LONGVARCHAR'):
        mock_column = ""
    elif column_type in ('REAL', 'DOUBLE'):
        mock_column = 0.0
    elif column_type == 'BLOB':
        mock_column = b""
    elif column_type in ('DATE', 'TIMESTAMP'):
        mock_column = 0
    else:
        sys.exit(f"ERROR: Unsupported column type {column_type}")

    return mock_column


def generate_mock_columns(connection,
                          table_name,
                          should_mock_column_nulls,
                          should_use_random_test_data,
                          days_ago=0,
                          entity_registry=None):
    auto_increment_column_names = get_auto_increment_column_names(
        connection, table_name)

    mock_columns = []

    table_info = get_table_info(connection, table_name)
    for column_name, column_type, column_notnull, column_pk in zip(
            column_names(table_info), column_types(table_info),
            column_notnulls(table_info), column_pks(table_info)):
        if column_name in auto_increment_column_names:
            # Do not mock auto-increment columns.
            continue

        mock_column = None

        is_uuid = get_column_is_uuid(table_name, column_name)
        entity_fk_key = COLUMN_ENTITY_KEY.get(column_name)
        entity_source_key = COLUMN_ENTITY_SOURCE.get((table_name, column_name))

        if (entity_fk_key and not entity_source_key and entity_registry
                and entity_registry.get(entity_fk_key)):
            pool = entity_registry[entity_fk_key]
            if column_pk == 1:
                # PK column: draw without replacement to avoid ON CONFLICT
                # REPLACE silently overwriting rows with the same key.
                iter_key = f"_iter_{table_name}_{column_name}"
                if iter_key not in entity_registry:
                    pool_copy = list(pool)
                    secrets.SystemRandom().shuffle(pool_copy)
                    entity_registry[iter_key] = pool_copy
                iterator = entity_registry[iter_key]
                mock_column = (iterator.pop()
                               if iterator else str(uuid.uuid4()))
            else:
                mock_column = secrets.choice(pool)
        elif column_pk == 1:
            # Always mock the column if the column is a primary key.
            mock_column = (
                generate_mock_column_with_random_test_data(
                    connection, table_name, column_name, column_type, days_ago)
                if is_column_unique(connection, table_name, column_name)
                or should_use_random_test_data or is_uuid else
                generate_mock_column_with_fixed_test_data(column_type))
        elif column_notnull == 1:
            # Always mock the column if the column has a NOT NULL constraint.
            mock_column = (
                generate_mock_column_with_random_test_data(
                    connection, table_name, column_name, column_type, days_ago)
                if is_column_unique(connection, table_name, column_name)
                or should_use_random_test_data or is_uuid else
                generate_mock_column_with_fixed_test_data(column_type))
        else:
            # Only mock the column if we should mock NULL columns.
            if should_mock_column_nulls:
                mock_column = (
                    generate_mock_column_with_random_test_data(
                        connection, table_name, column_name, column_type,
                        days_ago)
                    if should_use_random_test_data or is_uuid else
                    generate_mock_column_with_fixed_test_data(column_type))

        if entity_source_key is not None and mock_column is not None:
            entity_registry.setdefault(entity_source_key,
                                       []).append(mock_column)

        mock_columns.append(mock_column)

    return mock_columns


def create_connection(database):
    connection = None

    try:
        connection = sqlite3.connect(database)
    except sqlite3.Error as e:
        print(f"ERROR: Failed to connect to database {e}")
        raise

    return connection


def close_connection(connection):
    if connection:
        connection.close()


def vacuum(connection):
    connection.execute("VACUUM")


def get_table_names(connection):
    connection_cursor = connection.cursor()

    connection_cursor.execute('''
        SELECT
          name
        FROM
          sqlite_master
        WHERE type = 'table'
          AND name NOT LIKE 'sqlite_%'
          AND name NOT LIKE 'meta';
        ''')
    return [row[0] for row in connection_cursor.fetchall()]


def get_table_info(connection, table_name):
    connection_cursor = connection.cursor()
    connection_cursor.execute(f"PRAGMA table_info({table_name});")
    return connection_cursor.fetchall()


# Extract column ids.
def column_cids(table_info):
    return [column[0] for column in table_info]


# Extract column names.
def column_names(table_info):
    return [column[1] for column in table_info]


# Extract column types.
def column_types(table_info):
    return [column[2] for column in table_info]


# Extract column NOT NULL constraints.
def column_notnulls(table_info):
    return [column[3] for column in table_info]


# Extract column DEFAULT values.
def column_dflt_values(table_info):
    return [column[4] for column in table_info]


# Extract column PRIMARY KEY constraints.
def column_pks(table_info):
    return [column[5] for column in table_info]


def is_column_unique(connection, table_name, column_name):
    connection_cursor = connection.cursor()

    connection_cursor.execute(f"PRAGMA index_list({table_name});")
    indexes = connection_cursor.fetchall()

    for index in indexes:
        index_name = index[1]
        is_unique = index[2]

        if is_unique:
            connection_cursor.execute(f"PRAGMA index_info({index_name});")
            index_info = connection_cursor.fetchall()

            for column in index_info:
                if column[2] == column_name:
                    return True

    return False


def get_auto_increment_column_names(connection, table_name):
    connection_cursor = connection.cursor()

    auto_increment_column_names = []

    table_info = get_table_info(connection, table_name)
    for column_name, column_pk in zip(column_names(table_info),
                                      column_pks(table_info)):
        if column_pk == 0:
            continue

        connection_cursor.execute(f"""
            SELECT
              sql
            FROM
              sqlite_master
            WHERE type='table'
              AND name='{table_name}';
            """)
        create_table_sql = connection_cursor.fetchone()[0]
        if 'AUTOINCREMENT' in create_table_sql:
            auto_increment_column_names.append(column_name)

    return auto_increment_column_names


def get_longvarchar_length(connection, table_name, column_name):
    connection_cursor = connection.cursor()
    connection_cursor.execute(
        f"SELECT LENGTH({column_name}) FROM {table_name};")
    return connection_cursor.fetchall()


def delete_table_rows(connection, table_name):
    connection_cursor = connection.cursor()

    try:
        connection_cursor.execute(f"DELETE FROM {table_name}")
    except Exception as e:
        print(f"ERROR: Failed to delete {table_name} table rows {e}")
        raise


def delete_table_auto_increment_counter(connection, table_name):
    connection_cursor = connection.cursor()

    try:
        connection_cursor.execute(
            f"DELETE FROM sqlite_sequence WHERE name='{table_name}'")
    except Exception:
        # The `sqlite_sequence` table will not exist if the table lacks an
        # auto-increment column.
        pass


def insert_mock_table_row(connection, table_name, columns):
    auto_increment_column_names = get_auto_increment_column_names(
        connection, table_name)

    table_info = get_table_info(connection, table_name)
    filtered_column_names = [
        column_name for column_name in column_names(table_info)
        if column_name not in auto_increment_column_names
    ]

    comma_separated_filtered_column_names = ', '.join(filtered_column_names)

    bind_columns = ", ".join(["?"] * len(columns))

    connection_cursor = connection.cursor()
    try:
        connection_cursor.execute(
            f'''
            INSERT INTO {table_name} (
              {comma_separated_filtered_column_names}
            ) VALUES ({bind_columns})
            ''', columns)
    except sqlite3.IntegrityError as e:
        print(f"ERROR: Failed to insert {columns} into {table_name}: {e}")
        raise


def _generate_row_distribution(days, min_row_count, max_row_count):
    row_range = max_row_count - min_row_count + 1
    return [secrets.randbelow(row_range) + min_row_count for _ in range(days)]


def insert_mock_table_rows(connection, table_name, distribution,
                           entity_registry):
    row_index = 0
    for days_ago, row_count in enumerate(distribution):
        for _ in range(row_count):
            should_mock_column_nulls, should_use_random_test_data = (
                _ROW_PATTERNS[row_index % len(_ROW_PATTERNS)])
            mock_columns = generate_mock_columns(
                connection,
                table_name,
                should_mock_column_nulls=should_mock_column_nulls,
                should_use_random_test_data=should_use_random_test_data,
                days_ago=days_ago,
                entity_registry=entity_registry)
            insert_mock_table_row(connection, table_name, mock_columns)
            row_index += 1


def mock_table(connection, table_name, distribution, entity_registry):
    print(f"Deleting {table_name} table rows")
    delete_table_rows(connection, table_name)

    print(f"Deleting {table_name} table auto-increment counter")
    delete_table_auto_increment_counter(connection, table_name)

    print(f"Mocking {table_name} table test data")
    insert_mock_table_rows(connection, table_name, distribution,
                           entity_registry)


def mock_tables(connection, distribution):
    table_names = _sort_tables_for_insertion(get_table_names(connection))
    entity_registry = {}
    for table_name in table_names:
        mock_table(connection, table_name, distribution, entity_registry)

    connection.commit()


def verify_mock_tables(connection, expected_row_count):
    table_names = get_table_names(connection)
    for table_name in table_names:
        connection_cursor = connection.cursor()
        connection_cursor.execute(f"SELECT count(*) FROM {table_name};")
        actual_count = connection_cursor.fetchone()[0]
        if actual_count == 0:
            sys.exit(f"ERROR: {table_name} table is empty after mocking")
        if actual_count != expected_row_count:
            # Tables with composite PKs or ON CONFLICT REPLACE semantics may
            # have fewer rows than expected due to duplicate key collisions.
            print(f"WARNING: {table_name} table has {actual_count} rows, "
                  f"expected {expected_row_count}")


def mock_database(database,
                  days=DEFAULT_DATE_RANGE_DAYS,
                  min_row_count=DEFAULT_MIN_ROW_COUNT,
                  max_row_count=DEFAULT_MAX_ROW_COUNT):
    if not os.path.exists(database):
        sys.exit(f"ERROR: {database} does not exist")

    connection = create_connection(database)
    if not connection:
        sys.exit("ERROR: Failed to connect to database")

    distribution = _generate_row_distribution(days, min_row_count,
                                              max_row_count)
    total_row_count = sum(distribution)
    print(f"Mocking database migration test data "
          f"({total_row_count} rows per table over {days} days, "
          f"{min_row_count}–{max_row_count} per day)")

    mock_tables(connection, distribution)
    verify_mock_tables(connection, total_row_count)
    vacuum(connection)
    close_connection(connection)

    print("Mocked database migration test data")


def main():
    argument_parser = argparse.ArgumentParser(
        description="Mock database migration test data.")
    argument_parser.add_argument('database', help="The SQLite database")
    argument_parser.add_argument(
        '--days',
        type=int,
        default=DEFAULT_DATE_RANGE_DAYS,
        help=f"Spread timestamps over the past N days (default: "
        f"{DEFAULT_DATE_RANGE_DAYS})")
    argument_parser.add_argument(
        '--rows',
        type=int,
        default=None,
        help="Set both --min-rows and --max-rows to N (exact rows per day)")
    argument_parser.add_argument(
        '--min-rows',
        type=int,
        default=DEFAULT_MIN_ROW_COUNT,
        help=f"Minimum rows per day (default: {DEFAULT_MIN_ROW_COUNT})")
    argument_parser.add_argument(
        '--max-rows',
        type=int,
        default=None,
        help="Maximum rows per day (default: same as --min-rows)")
    args = argument_parser.parse_args()

    min_row_count = args.rows if args.rows is not None else args.min_rows
    max_row_count = (args.rows if args.rows is not None else (
        args.max_rows if args.max_rows is not None else min_row_count))

    if min_row_count > max_row_count:
        sys.exit(f"ERROR: --min-rows ({min_row_count}) must be <= "
                 f"--max-rows ({max_row_count})")

    mock_database(args.database, args.days, min_row_count, max_row_count)


if __name__ == "__main__":
    main()
