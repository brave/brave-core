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
CREATE TABLE IF NOT EXISTS "server_publisher_info" (
	"publisher_key"	LONGVARCHAR NOT NULL UNIQUE,
	"status"	INTEGER NOT NULL DEFAULT 0,
	"excluded"	INTEGER NOT NULL DEFAULT 0,
	"address"	TEXT NOT NULL,
	PRIMARY KEY("publisher_key")
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
	CONSTRAINT "creds_batch_unique" UNIQUE("trigger_id","trigger_type"),
	PRIMARY KEY("creds_id")
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
	PRIMARY KEY("token_id" AUTOINCREMENT)
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
INSERT INTO "meta" VALUES ('mmap_status','-1');
INSERT INTO "meta" VALUES ('version','25');
INSERT INTO "meta" VALUES ('last_compatible_version','1');
INSERT INTO "server_publisher_info" VALUES ('3zsistemi.si',2,0,'0fd15856-ada1-45a3-9ec7-74ac4db76eec');
INSERT INTO "server_publisher_info" VALUES ('laurenwags.github.io',2,0,'abf1ff79-a239-42af-abff-20eb121edd1c');
INSERT INTO "server_publisher_banner" VALUES ('laurenwags.github.io','Staging Banner Test','Lorem ipsum dolor sit amet, sale homero neglegentur ei vix, quo no tacimates vituperatoribus. Per elit luptatum temporibus ad, cibo minimum quaerendum no nec, atqui corpora complectitur te sed. Per ne vulputate neglegentur, id nec alia affert aperiri. Ea melius deserunt pro. Officiis sadipscing at nam, adhuc populo atomorum est.','chrome://rewards-image/https://rewards-stg.bravesoftware.com/xrEJASVGN9nQ5zJUnmoCxjEE','chrome://rewards-image/https://rewards-stg.bravesoftware.com/8eT9LXcpK3D795YHxvDdhrmg');
INSERT INTO "server_publisher_links" VALUES ('laurenwags.github.io','twitch','https://www.twitch.tv/laurenwags');
INSERT INTO "server_publisher_links" VALUES ('laurenwags.github.io','twitter','https://twitter.com/bravelaurenwags');
INSERT INTO "server_publisher_links" VALUES ('laurenwags.github.io','youtube','https://www.youtube.com/channel/UCCs7AQEDwrHEc86r0NNXE_A/videos');
INSERT INTO "server_publisher_amounts" VALUES ('laurenwags.github.io',5.0);
INSERT INTO "server_publisher_amounts" VALUES ('laurenwags.github.io',10.0);
INSERT INTO "server_publisher_amounts" VALUES ('laurenwags.github.io',20.0);
INSERT INTO "creds_batch" VALUES ('11c0dda2-97b2-4a59-802b-8b067f985090','6271eb7c-c401-4088-abd2-a8a533e57120',1,'["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"]','["Drf1IAmnuDyF8Jy2Z/2Ff/rQa69bwN8WhlLTmbA+PAI=","COmKIPhs/+V/dc/A7H9+kqHugwoGorZWMemvsPgMMQA=","qORNGjJG/PcfgYGCdH3Fa/Vpp5hh8NbhRBwGjjoXMVM=","JGCnqDvM3kTuOWNQsSDi5n7RCHdnO4usN07fO8uqJjU=","WuxcPhRKtiRRlyCig3Y/iuXg0YxEHthLMr5jgQEnrTk=","7hot5hDCB3OBJK8bX0Pse3hgibHzlf7rfAxUOwrgp3U=","SIOHt54ocWAOgPRof+jhUK8xwOx/UdI/ekKWZelJ3Hw=","8h3QJCLV3X65kyDsITpVVdasLM5IwACFxz/vrhIG60s=","2OYHNUnSHA7GX57b8jcrZYYjXxjJvJ5Xu1co0Dx8e00=","xqKZsCn9CxVns9HLN9mGUm2P5lWKdNFYdt7+WUb3Kxk=","0IhySiIkfP3mDC+cB2yfFvuqIkgLFJwi6FmdqPPdKFw=","avPU5iPbvUXZgsjovHrPAY9vsEjOuOc5jZv0E2SSpyI=","HJt2bct9+tQ01B+nDjFxe7UXd9eV0sjnEgRtluZTjlo=","AHGeR8in0aPVGzFxeht25j45Nfmc29qFoBM6joQxPkc=","8Ob7f3sWvApwIsYDgjLs5/cg66S0+kD6lCybcbECP3o=","QNN7rbmm09PPeCJ6HwwO0eqgSFqQlVmEovKl7wO7dEE=","6B4CkI+K069lBcWc1uzD9KXWILmwT1ymmT7og76Igyk=","9NProqbce/5o+dY9U28Iw2lVkyAtzRLwJsUPx+dp4EU=","guko9I7mIL8REV2sWiWk0bEreTFzdk9V9Y05MqQciA4=","phYe//MYjxfZdHXy99XK1QZkI0jEIDo8jojl/D+72Bc=","vGRj568ky7Yke77sCdXkGaXBaZBGHIunDHDL8CZuNQo=","KpUnEVhxmaeKkJzYqFvC9seBMzVOsE0g/ZWSqQO3axw=","uKd3UZbOvockBuOJiAVsVo9fHjpCiFgRHQKhsfpzHCU=","5nzGZLJv8P4QBw7zRp3Dj7l39fy8oF6MbCxTV0bZp3c=","OLrPPJWNOxxC0FO8AQz2/bW8Lm/jzSprck8QRJLiNDQ=","gPTy6WexzQv7jzMY8DLxhqmrTqTw0AaPq1kqUnzGjRQ=","JlcwTx+7CC+XimY/JkBoeRIHpoSUwGW20Icr7N+i2lE=","XIWVKjmN6+BLyTZfpwqiONplZaoXm95YvL09HyYw91o=","EgzPsK6PseEnUO9xc5A89q1Lbrv8ekTLIkBmfWkGnmQ=","JmKr8drUc66ll4deeAYIjwviwCwA4kEt38W7/L37uVk=","HptJMDfLLAY0gdlSQ3mKMkAKp3d0UAaOgQcbYonwyjY=","iq57asbS8b2wkzdS3VVuqK832bW3c9PAPkTKG3Xj7jQ=","jN0fx29fMZrkU0QZVRH6XONGsxDkzTLTInkXIFqbujQ=","2MLlz7LGHMeHSHy1MXxsSuIpyP/rKQ/cjVQhmscZiQo=","SDknCQOBTwPEtKk2DcMVjowzVrFsM1CeTJfa+RQLcTY=","+KqwZwXJrkNKJr+7SBQa+O+R66AE+T3RXUtU7oPUNgc=","Xtoe10iiaFklu8nr8byZB3gMX9d+Wq9WpH9tBcM9iGM=","khmWYFvNiVcgXESKCRzrZqFqzzpfaUkFqJohC14UqGQ=","WkMtDis4KUVyZOoKbv1hVapp966gl+1tjCt1MUVk8W0=","9vrb5hH91Vm9BIqM7LGqdsUnWbXvPIAq3fY+trBKRVQ=","GpfSbLMaXQ2WhQZB57f2we80QKna+eBQSxPB94Yptxw=","Tuo+lQ4fY/+uOS2Y1hB8102PoSem8qEnpblx9VokQ1E=","GJmqx46a1JaBxoI4x8oMgjAhwG+XJc2StiRWY9NhVzg=","ovzdX35L7YDlXnh2B3F8fgryaEwnJffLaW7w7VA+4SE=","srsqKcRvwzxGtM+cg/b71oqJOYKcpkfv9buzteKZlys=","UM7bImz77FQwJhsBKdUIFPX41HKcSy8+IiZcG7diWXM=","ELjqEba5KvHmCxgUuk8GWc0FIrZr3kcuHc6lYDuz7wQ=","jpA0O4w+EmBrvQRLlwcIstd4Gz7oFeIaswCsvUmX7GQ=","Cthv6XQusoxD/ZIkyNbTMAK6XS29tGbFtC+AcL4fgUk=","0Pb0lYgK1jrAjlXMjN0ukiyyifu5q8/Nl4SCgE6HV3o=","+tAmg9OF3a/eeW8FxuUBM11qL7+xKCmnzys/W29MUEc=","ToyomSO2ILZ64vrtM3bj0bogEs9n1oBOnpKsfyi2PV8=","EI19DxOZUpxYMVkEy8cFx970rorC0eZrW5yX5+vZ+2c=","BEQGkuR/L6tYXG5aalHv7pppqYZUypT0P1dbFpsnNVU=","AskfIzp8fE9TjX1DTh4tOEPR7FfUhfTce9J/uaNjG0M=","urnRg162Sh9GSvZ5ssPJpTwX6gCRObwG4J0yi3I56DY=","3n+1XS9aQFfhRycLUxvC7FACFuMJktINlKSlGTBRlFQ=","0MRiT1WTOXzIccY12nXQWyDQvb/oaqX1UITc3imAW2M=","1HFisBzkIOzGTI1DEEyRHjo1ZqDR+tof39pvv9kL9HM=","jJd2+NB3SXQyKOVco69wAPO0YB9mDeoeK+PV4RtR6XE=","GmoTVVwxyjaWiGZtR9S6IYB7GrZ8siVEnQKw4WqgEBA=","tBeFoLDmDagVKlYixtNXo7durtA3Z94FAXJOuLalzW0=","hFkj3lXMA0OXrA48SS7zbAKQ8UHBEaIeKLBwgctx8BY=","cPS7pJEg8K4c1ZUr/Ik7nLm/Xog7jTnSmK16ZV+jOFw=","1rAyvk12ZpH5+oXa/xEMtSD4bwxVXN2Ss4bk2M4Jeyg=","ngziNLhmvAExST6/w1sZOrUdWnHzyadHzrn1RLPBXWA=","HtG2ZVKUq+Vj5fOIfsg39NlfHKxekxXkR8DwdYkcKxY=","hshXx37az1do2C0HUZNvlz32QqsIeSHdlgRKPC7cZTw=","iEGKxllqKMf1JdRVv4FSDpVNj3rAwgnSy/FpyUI7ZHQ=","hmdGlwX5kV2aPA1tL9Vk+rWniNeN6vpxaZzzt8DYeXU=","UCai3l4/RCiKE63gOH6MB8MBq3kgREZHeG5jnQIVlhw=","xpR9ZluwnG/xowi8zPVQGpGNTT0DwZ/16ShgoU+IDkk=","knv5JSO5CNBBvi/UJs6+oKZx1+zMRXU4Dv+f7YTdaUs=","kqpz2CwJrLwsyr7Pf9dWr/Ka15afNMsIdt79+pETvFc=","RltROWoCXBei46ZYalkA4Yexs8itmupXVcEhcWaK6m8=","sCV+exmqKGypFxVzj/8CuLSmaUEVEQ8E7aLRukA5BEg=","hIHEBPq21K3OTUZ2nS0tTmckmWPY9W0BcjhoGcnRPEM=","tAjNZ9030Pq+rAXT85z/OXbooQ0oTH8NE/xI5JTVRVA=","yD8PwzPWNNE/i6CxIvm1vDnLQM3iaXAl5d53tBBERV0=","xLXq6l4bLVKNDoncl3MOJUOmaUg32yMdXMQmPmqOC24="]','["PoWDM5cJuECuiwUv13+IFfIPKFekY6neDZWxfvPt+1U=","PLgwa3ngWrdMrEjj7yKjsnd2HOZUwbXxc4osoVXS/ks=","GNexZy/zNmXygYp3DSaKzSVrRDDJRRjK0CEf0iaJTSU=","qmOIcYluQwrfDMR5ox7mZbct7vRoaFgCg518E0wiJXw=","xPcRFykbq/mnnjL/qwroIk+yFgf0jzZIdsfgdN/bf0k=","OtayL5OBW2iN/aRn7wWEBOonWAXaL/mEJkaim3fybmA=","6uhkz9+eHB+fZ1K3WN+/1DdajwOGvh9PGI4yqNug3yo=","jqTCQOKaHCFyY9nPzkfpD8nsU30WgUMA8RDbUGlmzVs=","UEy+6jupumgr1EZPI/7l+6IArNpXzamOvhIIoCqziX0=","AKlhzog3Al8goNWwYGs6i5myyey6m+w6toMtxMtSz2Y=","hoavlMg5d0TyXaOaIjAu01EK+vBnmmjFC1dKaZjTvV8=","Nu/X5L69JwTADkZ+Ll0L70vQllObWnr5MVXx9XScOAo=","QOuEaLk1tgT0/2jCUiACd0xcsISitYMNJOu+8kiKvhM=","5giVFzG9DT3wAjAplonqoQYIP68Qeo0UAwiszuiAUmI=","QA1J06dFvzFWmM/OecIm0XuQPlByxl7ZbFnlrKvmRyo=","vn2IY0krRBbj6oW1KH6GFjgOgGPgD9vvvm6Cni/Y9iw=","sLYOtOcRhPXqmTlI9ij5TjhZ/vP0bZS2NENfzQhbJyw=","wmAy23po3smWVwk4chf4HHHAqLTiuaFcY5/ugGxmnHc=","DD/Y971QJTvu4Pd7dtqZcMENYCZqHblET4fPwtPU2m8=","tN8iFIwpLH3ZAGgZN3brERxzN6k17AAXTAkH5TCxA3o=","RL4EXzJJIBUrmv4qbduMT0jCDbk1uI+s5uWcxz8W4jI=","ihpICTdljIifW+u3eJsgkWq4UwDvf9fWzbjYksVpzx4=","en1ZCjJs1I+0YJmR9H9UWGmGYe/BX20a/z2SaPSk7j8=","wiNqvx8WruK85vC/trnTdGto1vL9LOf9i4tZMqnCQBk=","YjpBkH0bQAa5ztW/a/gj2g5FHF9MFUNYYrPkzhZdkws=","5ODE4oV9fwP1kbOkZ1VmDN03wnnP9HGmvZDUjErAN1Q=","JgBWho5flz/FVWPSPqV/YTutraKAzLYFm2eO/AZmmzY=","5OaCcUU5Qu40OJFwL9oG9R79JXPDufDmAmwMfU8i3RI=","aEwqhWd3l44hbz5uUiPXkpGvki4gMABwBmdmSWsgY24=","MHWlyRr3Pb8TS5L8vtJgdUFyB+8QnJG3Eeq3pmHgX0M=","mD5O5YLA/TyvKDEfu2LMfHDn69fTYZEwXI0mqsO++lw=","ri+jchIwVM5y2GFGEebqR6z3CtTbt08juhipRdfcLUg=","8HiUgvOxaWGCdDViRBRXGB8V7d0qsxtkPBkDWrxdmHI=","XNhi5V62WmVGUeSYDvK7FpF90SqLg9tVsCCWipktDCg=","5kgR5o9ET8eTB21/G28kICeQWqn/J/k94sJ2GOrstRM=","rudNK/h1UDj5TZAgYiwvnjyeJpKu0LguWxZz6YRgLxU=","IrVI88058L7UFwJhECBibgEq2TqtNrFSZKiZiOulTlc=","oLju0CJmQX1QwJngjnWsrFVTtO4p2qpiuzuvq2u0XhU=","Jgx/b+a8qkpWrkdZWuxcxDOY5DC7ZYnhV4XwdOa7IiM=","epPp9J9SyMGfyEfSOqNIOcVzrFS2R0zeSX9sZdQib0I=","CIURf+BzbbdNO+vQ31CZrjq3DO9EYAQAvZb/Y91d+xk=","pM6q8SqB1BeZxp7SKgz1arJOR5b/YXntxrX+CcHPkxY=","3IL/zCu48xvugpYprni9GaBds5VQ5EUHqR2PmxOVDVA=","3olUJoAN1uSbNrWnCfvnAnwMAkDicaO3guas7PnLzBw=","FvJQ48BEtwGKVTIjl5lVW2oBgg26WPSVudDanJOAdVs=","plMBdmTraUmAL7CptIL9nzd63gCMNEqhGZkcLWi3O1I=","LEIDs+nL5/30P4/1zaGaCN0waUnKdtlVB5lh04o4BhE=","LLAxO8BFiqRv6hLIfiPIcGnsSO1huhsj5sBed3ru1i8=","gnFpaRq+G60ENkt1kbbHminRl1IR+lCxcTmSaML1cGE=","GJnG096AjrCwsZfBgiv3RNaVdxnqLVLYnTNETSAexQI=","EjHJriILs+Av/JtojfHChrg8TVfZSDKYeQYrUsCUGFA=","TDrB9G75btBVnSGxwVzySRMES5tcrhB4cbN0s3yASB4=","qvrIcu5DdbXX1X6SubiDaUQpV4mHVQWqFHVvyO+hjyU=","KDrsbuHAtynyNhVG9FwUacROCK/Cq+k0RVirs0loOxA=","Gkv5JfvLPNd+yEmQb276Rgb/pU8tmM75HC+F+mOojFw=","jCSlLu6jp5jZyxPDVplulTPm4WTQoGT3tZoOPd4bUnM=","DrqINPJXSlXJsoD21+w5Jbu0i4QpcMDYumcs8TS/0FE=","POx6WnM1CQFKHYl28qHvnGdMz6upuJoD0E2gsBRoFwY=","EJnxLiLw6D/be56PQn5Lfpqhmd3Kff5xZaE97HOFDV4=","pAELByzQKLGQUr3+bQ0TsKFpZODj3ocr1YGgDqCg5UE=","bEFiknPn6yu5vbnev3NZukBTizUG2regb1v8LghLhxk=","sNlNSlxhC7ZLKKiWr/iva5Iuncm0/+MjgRVFmlpIU08=","VicNfXFg7Wl2y+ZlepcUU3sceidgXLFlcParKBUzgXw=","ZAebmfalAMjhyn4oFbvGxLfAy9sUHIrkVDTmfmvCyFM=","mlLRjgstc0Hvjo9ijIDCwDb244b5+Qcr+i7NWUoGQ10=","pjq29F8gRw7fRz5R4/N+FWicRpo4fvgC2GPUWO2x1SU=","MGbfnrOPg+tHRufBaW5XsSzlYFYtkSw7Brk0yD0pr0o=","PnOIZkcrn1OAM/WyVtgbOBcfzrm2sHze97poZZblsQU=","xvWUzLFZwA9SBlzlno/7HCY3uvsGK/AEaLR73EUx6R8=","jmiohoCP9IuTh8i/NHU2zSpH5BHOJ2iRr1u4iKYVeTk=","arSCNhLaf8kAgT72ijn0VfGas2WgY7fyv5tL++pr7QY=","GjXCPm4tqY2QQOt7Y31P7kXvzI6pJnhbi8ANzIyiRWE=","APMC1Gvn0zBBVcD8Ra7qYQQJpZmAZRYqpTjs9aUM9E0=","3B9jxOIAm26SdWWq7vklilD6Uc5IALIdzGJ57Qz+HUU=","rsfhyjh8cNX7WpCtl+DXzJnZODevXs+M+Dl9Lobzmg0=","3KhL2s/6B+BOg8vf/3v/CWWSszHEIYCUxdpWml3YmAA=","Gu34ZgaAMNcEj55Kdpo8p/StD966Ehx64EgEx+0GAgs=","xrwD66fA5uQnj8NQrITVotlaoRuE5Q7dTlnK7l+GYBM=","YNZdoirrsVWKjFqSze1v0NhQWZFDIZRfKhiq2uesFDM=","9nKbvIBwaNrTIWI6N1ywvl8UvtOqIxsYmwdE3CgMnkY="]','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=','7iZYKKB7Or2SHel9+sScUA9GsbPMvbSxkga9XA1ulQXGJaQbyyn9UE3egS9ORBUFiLKOsM5kp4Lptw7LX79xBg==',4,'2020-05-29 08:10:30');
INSERT INTO "unblinded_tokens" VALUES (1,'CAuK3b4QJFJFt7+3YRcQsVJyyxkXHxb/+iFOVMIlcmbwqPkgp/ZvDVjBAQgycZwzAZiy+16qMfkV2BKbnqHaZsZPNLL/k7mzU8TRN+ATPd9OFzPxjaMl47VRY5WdTWBg','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'ffccd899-8cba-422e-a762-a58c47a728ac',1595870924,'2020-05-29 08:10:35',1594870924,'ew5suU8UjcRdwc2+dUjzsr1iOJpOMzRK',8);
INSERT INTO "unblinded_tokens" VALUES (2,'hMVLYdVOXqdQG6YIBQsnwspHicBNxCsGNyNOIFcPnI6rcNG95kKTsSlX24zDtGj+embVm5NnGAZExsj8nSEoupDg9SP7X0Rjs47u7lU4MJL6pXtzq9rG7k1XxSF/FM55','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (3,'9G2sE0OVjaL0q0irv9ORUcSLEDOTGlTkqdNKDJ8WdmupKLKHKCpLQg2o1pIM2A05RXuzFnmmRx2fNzOiLm8gTbiRU8IpYxtDbrJho7onf4jlpWVPKT00SveDjIQI9vFQ','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (4,'0SYyiiPh/wCriIvT5nrvclMWQ9vgBBYs2iNvauTjzAj2ASUPMaD0tzY7Hc+JYrjnEJsjGE8Sv9q0lAOYpUsI3VbFDjVHol1LZ4gG9Ocr8M6hVC+NhOmS86h+tefIaLIQ','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (5,'/pdeeRaAzPuixPkQMTAV0jh7N96MNF260LSrWgYvkg6oXmcOJL96R6TqWn/Xg04Gfa6tqHxCa5u/XRuAYnLdoWBLdsRrnbTOuCyeqz/ZSIC3nJas+ekarGjvs4gWJM0c','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (6,'w8z/jYtWFmEs2UApoZspA2r1arHfPLKe6iDkmeuLSf6OqMytazevGPhL3fJmiCFgIKlH9GJxV9eoE3I3eN4XtY5FNctfkBobBosTmBsYL3F+wyjd2VJaUD8G+cVgbjxr','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (7,'stDkJ7aVeYxwDeQ/2BMaGIF0OeJ8mzsHeWC9bvAoN3vZ00tg5n+v5H2+RxQjH9N/AKTMCpgofrzrY9jLopu80aCQLKyUHPwvBTFzAWcB52WKezKJSF3w/4dPmxMtWuIh','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (8,'I314N1AbpsngcTnCtF1yBvZwkgsG0CxH9r6PAz9XqqUGf2kSJydzt3kFJn+9uXWxXtwQVT4geeUjes7kQEm2Qwy4HU//AI40wss/eNo2wF89jJQXf7q/hEGMh2fgoe00','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (9,'p9OnZd7B+z5HowTYKMZfSiXZrwKa6mg9QqcyUcrNlGL+mfb67ELH0ZkzI3mIPcoTGtcfjlPEw8mPZ3PJSgUJ09TrCSOyMBiDInziQdmsNs0W4scEfeOldjt5at/XZaN5','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (10,'xflp6Qq67V2d+ImPjy510woQktdQhIL5z5TZO9toUhj912+viJ6qIEiwQf/AG+JI7sQzsLudysEM4H2mJ+i7HGQi0ZYoW5GnYz9WocOB3L8MXtsM91IzdYPIPXnAzgMh','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (11,'CAuK3b4QJFJFt7+3YRcQsVJyyxkXHxb/+iFOVMIlcmbwqPkgp/ZvDVjBAQgycZwzAZiy+16qMfkV2BKbnqHaZsZPNLL/k7mzU8TRN+ATPd9OFzPxjaMl47VRY5WdTWBg','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (12,'hMVLYdVOXqdQG6YIBQsnwspHicBNxCsGNyNOIFcPnI6rcNG95kKTsSlX24zDtGj+embVm5NnGAZExsj8nSEoupDg9SP7X0Rjs47u7lU4MJL6pXtzq9rG7k1XxSF/FM55','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (13,'9G2sE0OVjaL0q0irv9ORUcSLEDOTGlTkqdNKDJ8WdmupKLKHKCpLQg2o1pIM2A05RXuzFnmmRx2fNzOiLm8gTbiRU8IpYxtDbrJho7onf4jlpWVPKT00SveDjIQI9vFQ','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (14,'0SYyiiPh/wCriIvT5nrvclMWQ9vgBBYs2iNvauTjzAj2ASUPMaD0tzY7Hc+JYrjnEJsjGE8Sv9q0lAOYpUsI3VbFDjVHol1LZ4gG9Ocr8M6hVC+NhOmS86h+tefIaLIQ','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "unblinded_tokens" VALUES (15,'/pdeeRaAzPuixPkQMTAV0jh7N96MNF260LSrWgYvkg6oXmcOJL96R6TqWn/Xg04Gfa6tqHxCa5u/XRuAYnLdoWBLdsRrnbTOuCyeqz/ZSIC3nJas+ekarGjvs4gWJM0c','7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=',0.25,'11c0dda2-97b2-4a59-802b-8b067f985090',1595870924,'2020-05-29 08:10:35',0,NULL,0);
INSERT INTO "balance_report_info" VALUES ('2020_5',20.0,0.0,0.0,0.0,0.0);
CREATE INDEX IF NOT EXISTS "server_publisher_info_publisher_key_index" ON "server_publisher_info" (
	"publisher_key"
);
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
CREATE INDEX IF NOT EXISTS "unblinded_tokens_creds_id_index" ON "unblinded_tokens" (
	"creds_id"
);
CREATE INDEX IF NOT EXISTS "unblinded_tokens_redeem_id_index" ON "unblinded_tokens" (
	"redeem_id"
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
COMMIT;
