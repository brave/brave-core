-- Create Tables

CREATE TABLE activity_info(publisher_id LONGVARCHAR NOT NULL,duration INTEGER DEFAULT 0 NOT NULL,visits INTEGER DEFAULT 0 NOT NULL,score DOUBLE DEFAULT 0 NOT NULL,percent INTEGER DEFAULT 0 NOT NULL,weight DOUBLE DEFAULT 0 NOT NULL,reconcile_stamp INTEGER DEFAULT 0 NOT NULL,CONSTRAINT activity_unique UNIQUE (publisher_id, reconcile_stamp) CONSTRAINT fk_activity_info_publisher_id    FOREIGN KEY (publisher_id)    REFERENCES publisher_info (publisher_id)    ON DELETE CASCADE);
CREATE TABLE contribution_info(publisher_id LONGVARCHAR,probi TEXT "0"  NOT NULL,date INTEGER NOT NULL,type INTEGER NOT NULL,month INTEGER NOT NULL,year INTEGER NOT NULL,CONSTRAINT fk_contribution_info_publisher_id    FOREIGN KEY (publisher_id)    REFERENCES publisher_info (publisher_id)    ON DELETE CASCADE);
CREATE TABLE contribution_queue (contribution_queue_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,type INTEGER NOT NULL,amount DOUBLE NOT NULL,partial INTEGER NOT NULL DEFAULT 0,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL);
CREATE TABLE contribution_queue_publishers (contribution_queue_id INTEGER NOT NULL,publisher_key TEXT NOT NULL,amount_percent DOUBLE NOT NULL,CONSTRAINT fk_contribution_queue_publishers_publisher_key     FOREIGN KEY (publisher_key)     REFERENCES publisher_info (publisher_id),CONSTRAINT fk_contribution_queue_publishers_id     FOREIGN KEY (contribution_queue_id)     REFERENCES contribution_queue (contribution_queue_id)     ON DELETE CASCADE);
CREATE TABLE media_publisher_info(media_key TEXT NOT NULL PRIMARY KEY UNIQUE,publisher_id LONGVARCHAR NOT NULL,CONSTRAINT fk_media_publisher_info_publisher_id    FOREIGN KEY (publisher_id)    REFERENCES publisher_info (publisher_id)    ON DELETE CASCADE);
CREATE TABLE pending_contribution(publisher_id LONGVARCHAR NOT NULL,amount DOUBLE DEFAULT 0 NOT NULL,added_date INTEGER DEFAULT 0 NOT NULL,viewing_id LONGVARCHAR NOT NULL,type INTEGER NOT NULL,CONSTRAINT fk_pending_contribution_publisher_id    FOREIGN KEY (publisher_id)    REFERENCES publisher_info (publisher_id)    ON DELETE CASCADE);
CREATE TABLE promotion (promotion_id TEXT NOT NULL,version INTEGER NOT NULL,type INTEGER NOT NULL,public_keys TEXT NOT NULL,suggestions INTEGER NOT NULL DEFAULT 0,approximate_value DOUBLE NOT NULL DEFAULT 0,status INTEGER NOT NULL DEFAULT 0,expires_at TIMESTAMP NOT NULL,created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,PRIMARY KEY (promotion_id));
CREATE TABLE promotion_creds (promotion_id TEXT UNIQUE NOT NULL,tokens TEXT NOT NULL,blinded_creds TEXT NOT NULL,signed_creds TEXT,public_key TEXT,batch_proof TEXT,claim_id TEXT,CONSTRAINT fk_promotion_creds_promotion_id FOREIGN KEY (promotion_id) REFERENCES promotion (promotion_id) ON DELETE CASCADE);
CREATE TABLE publisher_info(publisher_id LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,excluded INTEGER DEFAULT 0 NOT NULL,name TEXT NOT NULL,favIcon TEXT NOT NULL,url TEXT NOT NULL,provider TEXT NOT NULL);
CREATE TABLE recurring_donation(publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,amount DOUBLE DEFAULT 0 NOT NULL,added_date INTEGER DEFAULT 0 NOT NULL,CONSTRAINT fk_recurring_donation_publisher_id    FOREIGN KEY (publisher_id)    REFERENCES publisher_info (publisher_id)    ON DELETE CASCADE);
CREATE TABLE server_publisher_amounts (publisher_key LONGVARCHAR NOT NULL,amount DOUBLE DEFAULT 0 NOT NULL,CONSTRAINT server_publisher_amounts_unique     UNIQUE (publisher_key, amount) CONSTRAINT fk_server_publisher_amounts_publisher_key    FOREIGN KEY (publisher_key)    REFERENCES server_publisher_info (publisher_key)    ON DELETE CASCADE);
CREATE TABLE server_publisher_banner (publisher_key LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,title TEXT,description TEXT,background TEXT,logo TEXT,CONSTRAINT fk_server_publisher_banner_publisher_key    FOREIGN KEY (publisher_key)    REFERENCES server_publisher_info (publisher_key)    ON DELETE CASCADE);
CREATE TABLE server_publisher_info (publisher_key LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,status INTEGER DEFAULT 0 NOT NULL,excluded INTEGER DEFAULT 0 NOT NULL,address TEXT NOT NULL);
CREATE TABLE server_publisher_links (publisher_key LONGVARCHAR NOT NULL,provider TEXT,link TEXT,CONSTRAINT server_publisher_links_unique     UNIQUE (publisher_key, provider) CONSTRAINT fk_server_publisher_links_publisher_key    FOREIGN KEY (publisher_key)    REFERENCES server_publisher_info (publisher_key)    ON DELETE CASCADE);
CREATE TABLE unblinded_tokens (token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,token_value TEXT,public_key TEXT,value DOUBLE NOT NULL DEFAULT 0,promotion_id TEXT,created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,CONSTRAINT fk_unblinded_tokens_promotion_id FOREIGN KEY (promotion_id) REFERENCES promotion (promotion_id) ON DELETE CASCADE);

-- Create Indexes

CREATE INDEX activity_info_publisher_id_index ON activity_info (publisher_id);
CREATE INDEX contribution_info_publisher_id_index ON contribution_info (publisher_id);
CREATE INDEX pending_contribution_publisher_id_index ON pending_contribution (publisher_id);
CREATE INDEX promotion_creds_promotion_id_index ON promotion_creds (promotion_id);
CREATE INDEX promotion_promotion_id_index ON promotion (promotion_id);
CREATE INDEX recurring_donation_publisher_id_index ON recurring_donation (publisher_id);
CREATE INDEX server_publisher_amounts_publisher_key_index ON server_publisher_amounts (publisher_key);
CREATE INDEX server_publisher_banner_publisher_key_index ON server_publisher_banner (publisher_key);
CREATE INDEX server_publisher_info_publisher_key_index ON server_publisher_info (publisher_key);
CREATE INDEX server_publisher_links_publisher_key_index ON server_publisher_links (publisher_key);
CREATE INDEX unblinded_tokens_token_id_index ON unblinded_tokens (token_id);

-- Migrate data into tables

# {statements}
