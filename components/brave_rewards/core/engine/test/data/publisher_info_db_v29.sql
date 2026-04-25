BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "meta" (
	"key"	LONGVARCHAR NOT NULL UNIQUE,
	"value"	LONGVARCHAR,
	PRIMARY KEY("key")
);
CREATE TABLE IF NOT EXISTS "publisher_info" (
	"publisher_id"	LONGVARCHAR NOT NULL UNIQUE,
	"excluded"	INTEGER NOT NULL DEFAULT 0,
	"name"	TEXT NOT NULL,
	"favIcon"	TEXT NOT NULL,
	"url"	TEXT NOT NULL,
	"provider"	TEXT NOT NULL,
	PRIMARY KEY("publisher_id")
);
CREATE TABLE IF NOT EXISTS "promotion" (
	"promotion_id"	TEXT NOT NULL,
	"version"	INTEGER NOT NULL,
	"type"	INTEGER NOT NULL,
	"public_keys"	TEXT NOT NULL,
	"suggestions"	INTEGER NOT NULL DEFAULT 0,
	"approximate_value"	DOUBLE NOT NULL DEFAULT 0,
	"status"	INTEGER NOT NULL DEFAULT 0,
	"expires_at"	TIMESTAMP NOT NULL,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	"claimed_at"	TIMESTAMP,
	"claim_id"	TEXT,
	"legacy"	BOOLEAN NOT NULL DEFAULT 0,
	PRIMARY KEY("promotion_id")
);
CREATE TABLE IF NOT EXISTS "contribution_info" (
	"contribution_id"	TEXT NOT NULL,
	"amount"	DOUBLE NOT NULL,
	"type"	INTEGER NOT NULL,
	"step"	INTEGER NOT NULL DEFAULT -1,
	"retry_count"	INTEGER NOT NULL DEFAULT -1,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	"processor"	INTEGER NOT NULL DEFAULT 1,
	PRIMARY KEY("contribution_id")
);
CREATE TABLE IF NOT EXISTS "activity_info" (
	"publisher_id"	LONGVARCHAR NOT NULL,
	"duration"	INTEGER NOT NULL DEFAULT 0,
	"visits"	INTEGER NOT NULL DEFAULT 0,
	"score"	DOUBLE NOT NULL DEFAULT 0,
	"percent"	INTEGER NOT NULL DEFAULT 0,
	"weight"	DOUBLE NOT NULL DEFAULT 0,
	"reconcile_stamp"	INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT "activity_unique" UNIQUE("publisher_id","reconcile_stamp")
);
CREATE TABLE IF NOT EXISTS "media_publisher_info" (
	"media_key"	TEXT NOT NULL UNIQUE,
	"publisher_id"	LONGVARCHAR NOT NULL,
	PRIMARY KEY("media_key")
);
CREATE TABLE IF NOT EXISTS "pending_contribution" (
	"pending_contribution_id"	INTEGER NOT NULL,
	"publisher_id"	LONGVARCHAR NOT NULL,
	"amount"	DOUBLE NOT NULL DEFAULT 0,
	"added_date"	INTEGER NOT NULL DEFAULT 0,
	"viewing_id"	LONGVARCHAR NOT NULL,
	"type"	INTEGER NOT NULL,
	PRIMARY KEY("pending_contribution_id" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "recurring_donation" (
	"publisher_id"	LONGVARCHAR NOT NULL UNIQUE,
	"amount"	DOUBLE NOT NULL DEFAULT 0,
	"added_date"	INTEGER NOT NULL DEFAULT 0,
	PRIMARY KEY("publisher_id")
);
CREATE TABLE IF NOT EXISTS "server_publisher_banner" (
	"publisher_key"	LONGVARCHAR NOT NULL UNIQUE,
	"title"	TEXT,
	"description"	TEXT,
	"background"	TEXT,
	"logo"	TEXT,
	PRIMARY KEY("publisher_key")
);
CREATE TABLE IF NOT EXISTS "server_publisher_links" (
	"publisher_key"	LONGVARCHAR NOT NULL,
	"provider"	TEXT,
	"link"	TEXT,
	CONSTRAINT "server_publisher_links_unique" UNIQUE("publisher_key","provider")
);
CREATE TABLE IF NOT EXISTS "server_publisher_amounts" (
	"publisher_key"	LONGVARCHAR NOT NULL,
	"amount"	DOUBLE NOT NULL DEFAULT 0,
	CONSTRAINT "server_publisher_amounts_unique" UNIQUE("publisher_key","amount")
);
CREATE TABLE IF NOT EXISTS "creds_batch" (
	"creds_id"	TEXT NOT NULL,
	"trigger_id"	TEXT NOT NULL,
	"trigger_type"	INT NOT NULL,
	"creds"	TEXT NOT NULL,
	"blinded_creds"	TEXT NOT NULL,
	"signed_creds"	TEXT,
	"public_key"	TEXT,
	"batch_proof"	TEXT,
	"status"	INT NOT NULL DEFAULT 0,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY("creds_id"),
	CONSTRAINT "creds_batch_unique" UNIQUE("trigger_id","trigger_type")
);
CREATE TABLE IF NOT EXISTS "sku_order" (
	"order_id"	TEXT NOT NULL,
	"total_amount"	DOUBLE,
	"merchant_id"	TEXT,
	"location"	TEXT,
	"status"	INTEGER NOT NULL DEFAULT 0,
	"contribution_id"	TEXT,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY("order_id")
);
CREATE TABLE IF NOT EXISTS "sku_order_items" (
	"order_item_id"	TEXT NOT NULL,
	"order_id"	TEXT NOT NULL,
	"sku"	TEXT,
	"quantity"	INTEGER,
	"price"	DOUBLE,
	"name"	TEXT,
	"description"	TEXT,
	"type"	INTEGER,
	"expires_at"	TIMESTAMP,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	CONSTRAINT "sku_order_items_unique" UNIQUE("order_item_id","order_id")
);
CREATE TABLE IF NOT EXISTS "sku_transaction" (
	"transaction_id"	TEXT NOT NULL,
	"order_id"	TEXT NOT NULL,
	"external_transaction_id"	TEXT NOT NULL,
	"type"	INTEGER NOT NULL,
	"amount"	DOUBLE NOT NULL,
	"status"	INTEGER NOT NULL,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY("transaction_id")
);
CREATE TABLE IF NOT EXISTS "contribution_info_publishers" (
	"contribution_id"	TEXT NOT NULL,
	"publisher_key"	TEXT NOT NULL,
	"total_amount"	DOUBLE NOT NULL,
	"contributed_amount"	DOUBLE,
	CONSTRAINT "contribution_info_publishers_unique" UNIQUE("contribution_id","publisher_key")
);
CREATE TABLE IF NOT EXISTS "balance_report_info" (
	"balance_report_id"	LONGVARCHAR NOT NULL,
	"grants_ugp"	DOUBLE NOT NULL DEFAULT 0,
	"grants_ads"	DOUBLE NOT NULL DEFAULT 0,
	"auto_contribute"	DOUBLE NOT NULL DEFAULT 0,
	"tip_recurring"	DOUBLE NOT NULL DEFAULT 0,
	"tip"	DOUBLE NOT NULL DEFAULT 0,
	PRIMARY KEY("balance_report_id")
);
CREATE TABLE IF NOT EXISTS "processed_publisher" (
	"publisher_key"	TEXT NOT NULL,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY("publisher_key")
);
CREATE TABLE IF NOT EXISTS "contribution_queue" (
	"contribution_queue_id"	TEXT NOT NULL,
	"type"	INTEGER NOT NULL,
	"amount"	DOUBLE NOT NULL,
	"partial"	INTEGER NOT NULL DEFAULT 0,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	"completed_at"	TIMESTAMP NOT NULL DEFAULT 0,
	PRIMARY KEY("contribution_queue_id")
);
CREATE TABLE IF NOT EXISTS "contribution_queue_publishers" (
	"contribution_queue_id"	TEXT NOT NULL,
	"publisher_key"	TEXT NOT NULL,
	"amount_percent"	DOUBLE NOT NULL
);
CREATE TABLE IF NOT EXISTS "unblinded_tokens" (
	"token_id"	INTEGER NOT NULL,
	"token_value"	TEXT,
	"public_key"	TEXT,
	"value"	DOUBLE NOT NULL DEFAULT 0,
	"creds_id"	TEXT,
	"expires_at"	TIMESTAMP NOT NULL DEFAULT 0,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	"redeemed_at"	TIMESTAMP NOT NULL DEFAULT 0,
	"redeem_id"	TEXT,
	"redeem_type"	INTEGER NOT NULL DEFAULT 0,
	"reserved_at"	TIMESTAMP NOT NULL DEFAULT 0,
	PRIMARY KEY("token_id" AUTOINCREMENT),
	CONSTRAINT "unblinded_tokens_unique" UNIQUE("token_value","public_key")
);
CREATE TABLE IF NOT EXISTS "server_publisher_info" (
	"publisher_key"	LONGVARCHAR NOT NULL,
	"status"	INTEGER NOT NULL DEFAULT 0,
	"address"	TEXT NOT NULL,
	"updated_at"	TIMESTAMP NOT NULL,
	PRIMARY KEY("publisher_key")
);
CREATE TABLE IF NOT EXISTS "publisher_prefix_list" (
	"hash_prefix"	BLOB NOT NULL,
	PRIMARY KEY("hash_prefix")
);
CREATE TABLE IF NOT EXISTS "event_log" (
	"event_log_id"	LONGVARCHAR NOT NULL,
	"key"	TEXT NOT NULL,
	"value"	TEXT NOT NULL,
	"created_at"	TIMESTAMP NOT NULL,
	PRIMARY KEY("event_log_id")
);
INSERT INTO "meta" VALUES ('mmap_status','-1'),
 ('version','29'),
 ('last_compatible_version','1');
INSERT INTO "unblinded_tokens" VALUES (1,'123','456',30.0,'789',1640995200,'2020-05-29 15:58:14',0,NULL,0,0);
CREATE INDEX IF NOT EXISTS "promotion_promotion_id_index" ON "promotion" (
	"promotion_id"
);
CREATE INDEX IF NOT EXISTS "activity_info_publisher_id_index" ON "activity_info" (
	"publisher_id"
);
CREATE INDEX IF NOT EXISTS "media_publisher_info_media_key_index" ON "media_publisher_info" (
	"media_key"
);
CREATE INDEX IF NOT EXISTS "media_publisher_info_publisher_id_index" ON "media_publisher_info" (
	"publisher_id"
);
CREATE INDEX IF NOT EXISTS "pending_contribution_publisher_id_index" ON "pending_contribution" (
	"publisher_id"
);
CREATE INDEX IF NOT EXISTS "recurring_donation_publisher_id_index" ON "recurring_donation" (
	"publisher_id"
);
CREATE INDEX IF NOT EXISTS "server_publisher_banner_publisher_key_index" ON "server_publisher_banner" (
	"publisher_key"
);
CREATE INDEX IF NOT EXISTS "server_publisher_links_publisher_key_index" ON "server_publisher_links" (
	"publisher_key"
);
CREATE INDEX IF NOT EXISTS "server_publisher_amounts_publisher_key_index" ON "server_publisher_amounts" (
	"publisher_key"
);
CREATE INDEX IF NOT EXISTS "creds_batch_trigger_id_index" ON "creds_batch" (
	"trigger_id"
);
CREATE INDEX IF NOT EXISTS "creds_batch_trigger_type_index" ON "creds_batch" (
	"trigger_type"
);
CREATE INDEX IF NOT EXISTS "sku_order_items_order_id_index" ON "sku_order_items" (
	"order_id"
);
CREATE INDEX IF NOT EXISTS "sku_order_items_order_item_id_index" ON "sku_order_items" (
	"order_item_id"
);
CREATE INDEX IF NOT EXISTS "sku_transaction_order_id_index" ON "sku_transaction" (
	"order_id"
);
CREATE INDEX IF NOT EXISTS "contribution_info_publishers_contribution_id_index" ON "contribution_info_publishers" (
	"contribution_id"
);
CREATE INDEX IF NOT EXISTS "contribution_info_publishers_publisher_key_index" ON "contribution_info_publishers" (
	"publisher_key"
);
CREATE INDEX IF NOT EXISTS "balance_report_info_balance_report_id_index" ON "balance_report_info" (
	"balance_report_id"
);
CREATE INDEX IF NOT EXISTS "contribution_queue_publishers_contribution_queue_id_index" ON "contribution_queue_publishers" (
	"contribution_queue_id"
);
CREATE INDEX IF NOT EXISTS "contribution_queue_publishers_publisher_key_index" ON "contribution_queue_publishers" (
	"publisher_key"
);
CREATE INDEX IF NOT EXISTS "unblinded_tokens_creds_id_index" ON "unblinded_tokens" (
	"creds_id"
);
CREATE INDEX IF NOT EXISTS "unblinded_tokens_redeem_id_index" ON "unblinded_tokens" (
	"redeem_id"
);
COMMIT;
