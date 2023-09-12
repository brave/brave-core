mod fetch;
mod present;

use tracing::{error, instrument};

use crate::errors::{InternalError, SkusError};
use crate::models::*;
use crate::sdk::SDK;
use crate::{HTTPClient, StorageClient};

impl<U> SDK<U>
where
    U: HTTPClient + StorageClient,
{
    #[instrument]
    pub async fn delete_order_credentials(&self, order_id: &str) -> Result<(), SkusError> {
        let order = self.client.get_order(order_id).await?;

        if let Some(order) = order {
            for item in order.items {
                self.client.delete_item_creds(&item.id).await?;
            }
        }
        Ok(())
    }

    #[instrument]
    pub async fn matching_order_credential_summary(
        &self,
        order_id: &str,
        domain: &str,
    ) -> Result<Option<CredentialSummary>, SkusError> {
        let wrapped_order = self.client.get_order(order_id).await?;
        let order = wrapped_order.ok_or(InternalError::NotFound)?;
        if !order.location_matches(&self.environment, domain) {
            return Err(InternalError::OrderLocationMismatch.into());
        }

        let expires_at = order.expires_at;
        for item in &order.items {
            match item.credential_type {
                CredentialType::TimeLimitedV2 => {
                    let expires_at = self
                        .last_matching_time_limited_v2_credential(&item.id)
                        .await?
                        .map(|cred| cred.valid_to)
                        .or(expires_at);
                    if let Some(expires_at) = expires_at {
                        // attempt to refresh credentials if we're within 5 days of expiry
                        if self.now().naive_utc() > (expires_at - chrono::Duration::days(5)) {
                            error!("Within 5 days of expiry; refreshing order credentials.");
                            let refreshed = self.refresh_order_credentials(order_id).await;
                            if refreshed.is_err() {
                                error!("Error refreshing order credentials.");
                            }
                        }
                    }

                    if let Some(creds) = self.matching_time_limited_v2_credential(&item.id).await? {
                        let unblinded_creds =
                            creds.unblinded_creds.ok_or(InternalError::NotFound)?;
                        let remaining_credential_count = unblinded_creds
                            .into_iter()
                            .filter(|cred| !cred.spent)
                            .count();

                        let active = remaining_credential_count > 0;
                        return Ok(Some(CredentialSummary {
                            order,
                            remaining_credential_count, // number unspent
                            expires_at,
                            active,
                        }));
                    }

                    error!("No matches found for credential summary.");
                }
                CredentialType::SingleUse => {
                    let wrapped_creds = self.client.get_single_use_item_creds(&item.id).await?;
                    if let Some(creds) = wrapped_creds {
                        let unblinded_creds =
                            creds.unblinded_creds.ok_or(InternalError::NotFound)?;
                        let remaining_credential_count = unblinded_creds
                            .into_iter()
                            .filter(|cred| !cred.spent)
                            .count();

                        let expires_at = None;
                        let active = remaining_credential_count > 0;

                        return Ok(Some(CredentialSummary {
                            order,
                            remaining_credential_count,
                            expires_at,
                            active,
                        }));
                    } else {
                        continue;
                    }
                }
                CredentialType::TimeLimited => {
                    let expires_at = self
                        .last_matching_time_limited_credential(&item.id)
                        .await?
                        .map(|cred| cred.expires_at)
                        .or(expires_at);
                    if let Some(expires_at) = expires_at {
                        // attempt to refresh credentials if we're within 5 days of expiry
                        if self.now().naive_utc() > (expires_at - chrono::Duration::days(5)) {
                            error!("Within 5 days of expiry; refreshing order credentials.");
                            let refreshed = self.refresh_order_credentials(order_id).await;
                            if refreshed.is_err() {
                                error!("Error refreshing order credentials.");
                            }
                        }
                    }

                    if let Ok(Some(_)) = self.matching_time_limited_credential(&item.id).await {
                        return Ok(Some(CredentialSummary {
                            order,
                            remaining_credential_count: 1,
                            expires_at,
                            active: true,
                        }));
                    }

                    error!("No matches found for credential summary.");
                }
            };
        }
        Err(InternalError::NotFound.into())
    }

    #[instrument]
    pub async fn matching_time_limited_v2_credential(
        &self,
        item_id: &str,
    ) -> Result<Option<TimeLimitedV2Credential>, SkusError> {
        Ok(self
            .client
            .get_time_limited_v2_creds(item_id)
            .await?
            .and_then(|tlv2| {
                tlv2.unblinded_creds
                    .unwrap_or_default()
                    .into_iter()
                    .find(|cred| {
                        let now = self.now().naive_utc();
                        now < cred.valid_to && now > cred.valid_from
                    })
            }))
    }

    #[instrument]
    pub async fn matching_time_limited_credential(
        &self,
        item_id: &str,
    ) -> Result<Option<TimeLimitedCredential>, SkusError> {
        Ok(self
            .client
            .get_time_limited_creds(item_id)
            .await?
            .and_then(|creds| {
                creds.creds.into_iter().find(|cred| {
                    let now = self.now().naive_utc();
                    now < cred.expires_at && now > cred.issued_at
                })
            }))
    }

    #[instrument]
    pub async fn last_matching_time_limited_v2_credential(
        &self,
        item_id: &str,
    ) -> Result<Option<TimeLimitedV2Credential>, SkusError> {
        Ok(self
            .client
            .get_time_limited_v2_creds(item_id)
            .await?
            .and_then(|creds| creds.unblinded_creds.into_iter().flatten().last()))
    }

    #[instrument]
    pub async fn last_matching_time_limited_credential(
        &self,
        item_id: &str,
    ) -> Result<Option<TimeLimitedCredential>, SkusError> {
        Ok(self
            .client
            .get_time_limited_creds(item_id)
            .await?
            .and_then(|creds| creds.creds.into_iter().last()))
    }

    #[instrument]
    pub async fn matching_credential_summary(
        &self,
        domain: &str,
    ) -> Result<Option<CredentialSummary>, SkusError> {
        if let Some(orders) = self.client.get_orders().await? {
            for order in orders {
                if order.location_matches(&self.environment, domain) {
                    let wrapped_value = self
                        .matching_order_credential_summary(&order.id, domain)
                        .await;
                    if wrapped_value.is_err() {
                        continue;
                    }
                    return wrapped_value;
                }
            }
        }
        Ok(None)
    }
}

#[cfg(test)]
mod tests {
    use async_std::task;
    use chrono::{naive::Days, TimeZone, Utc};
    use mockall::predicate;
    use serde_json::json;

    use crate::errors::InternalError;
    use crate::models::*;
    use crate::sdk::tests::*;
    use crate::sdk::SDK;
    use crate::Environment;

    #[test]
    fn matching_time_limited_v2_credential_no_creds_works() {
        task::block_on(async {
            let mut client = MockClient::new();
            client
                .expect_get_time_limited_v2_creds()
                .with(predicate::eq(SAMPLE_ITEM_ID))
                .times(1)
                .returning(|_| Ok(None));
            let sdk = SDK::new(client, Environment::Production, None, None);
            let result = sdk
                .matching_time_limited_v2_credential(SAMPLE_ITEM_ID)
                .await;
            assert_eq!(result, Ok(None));
        })
    }

    #[test]
    fn matching_time_limited_v2_credential_works() {
        task::block_on(async {
            let mut client = MockClient::new();
            client
                .expect_get_time_limited_v2_creds()
                .with(predicate::eq(SAMPLE_ITEM_ID))
                .times(2)
                .returning(|_| {
                    Ok(Some(
                        serde_json::from_str(SAMPLE_TIME_LIMITED_V2_CREDENTIALS).unwrap(),
                    ))
                });
            let mut sdk = SDK::new(client, Environment::Production, None, None);

            // Time outside of validity interval results in no credential
            sdk.now = Utc.timestamp_nanos(SAMPLE_TIME_LIMITED_V2_CREDENTIAL_LAST_VALID_NANOS)
                + Days::new(1);
            let result = sdk
                .matching_time_limited_v2_credential(SAMPLE_ITEM_ID)
                .await;
            assert_eq!(result, Ok(None));

            let expected = {
                let tmp: TimeLimitedV2Credentials =
                    serde_json::from_str(SAMPLE_TIME_LIMITED_V2_CREDENTIALS).unwrap();
                tmp.unblinded_creds.unwrap().pop().unwrap()
            };

            // Time in between validity interval results in the credential being returned
            sdk.now = Utc.timestamp_nanos(SAMPLE_TIME_LIMITED_V2_CREDENTIAL_LAST_VALID_NANOS);
            let result = sdk
                .matching_time_limited_v2_credential(SAMPLE_ITEM_ID)
                .await;
            assert_eq!(result, Ok(Some(expected)));
        })
    }

    #[test]
    fn matching_order_credential_summary_expiring_order_works() {
        task::block_on(async {
            let mut client = MockClient::new();
            {
                client
                    .expect_get_order()
                    .with(predicate::eq(SAMPLE_ORDER_ID))
                    .times(1)
                    .returning(move |_| Ok(Some(serde_json::from_str(SAMPLE_ORDER).unwrap())));
            }
            client
                .expect_get_time_limited_v2_creds()
                .with(predicate::eq(SAMPLE_ITEM_ID))
                .times(2)
                .returning(|_| {
                    Ok(Some(
                        serde_json::from_str(SAMPLE_TIME_LIMITED_V2_CREDENTIALS).unwrap(),
                    ))
                });
            client
                .expect_execute()
                .with(predicate::always())
                .times(1)
                .returning(|_| {
                    let err = APIError {
                        code: 400,
                        message: "Bad Request".to_string(),
                        error_code: "".to_string(),
                        data: json!(null),
                    };
                    Err(InternalError::BadRequest(err).into())
                });
            let mut sdk = SDK::new(client, Environment::Production, None, None);

            let expected = CredentialSummary {
                order: serde_json::from_str(SAMPLE_ORDER).unwrap(),
                remaining_credential_count: 1,
                expires_at: Some(
                    Utc.with_ymd_and_hms(2023, 5, 17, 15, 04, 05)
                        .unwrap()
                        .naive_utc(),
                ),
                active: true,
            };

            sdk.now = Utc.timestamp_nanos(SAMPLE_TIME_LIMITED_V2_CREDENTIAL_LAST_VALID_NANOS);
            let result = sdk
                .matching_order_credential_summary(SAMPLE_ORDER_ID, "vpn.brave.com")
                .await;
            assert_eq!(result, Ok(Some(expected)));
        })
    }

    #[test]
    fn matching_credential_summary_multiple_orders_works() {
        task::block_on(async {
            let mut client = MockClient::new();
            client.expect_get_orders().times(1).returning(move || {
                Ok(Some(vec![
                    serde_json::from_str(SAMPLE_EXPIRED_ORDER).unwrap(),
                    serde_json::from_str(SAMPLE_ORDER).unwrap(),
                ]))
            });
            client
                .expect_get_order()
                .with(predicate::eq(SAMPLE_EXPIRED_ORDER_ID))
                .times(1)
                .returning(move |_| Ok(Some(serde_json::from_str(SAMPLE_EXPIRED_ORDER).unwrap())));
            client
                .expect_get_time_limited_v2_creds()
                .with(predicate::eq(SAMPLE_EXPIRED_ITEM_ID))
                .returning(|_| {
                    Ok(Some(
                        serde_json::from_str(SAMPLE_EXPIRED_TIME_LIMITED_V2_CREDENTIALS).unwrap(),
                    ))
                });
            client
                .expect_execute()
                .with(predicate::always())
                .times(1)
                .returning(|_| {
                    let err = APIError {
                        code: 400,
                        message: "Bad Request".to_string(),
                        error_code: "".to_string(),
                        data: json!(null),
                    };
                    Err(InternalError::BadRequest(err).into())
                });
            client
                .expect_get_order()
                .with(predicate::eq(SAMPLE_ORDER_ID))
                .times(1)
                .returning(move |_| Ok(Some(serde_json::from_str(SAMPLE_ORDER).unwrap())));
            client
                .expect_get_time_limited_v2_creds()
                .with(predicate::eq(SAMPLE_ITEM_ID))
                .times(2)
                .returning(|_| {
                    Ok(Some(
                        serde_json::from_str(SAMPLE_TIME_LIMITED_V2_CREDENTIALS).unwrap(),
                    ))
                });

            let expected = CredentialSummary {
                order: serde_json::from_str(SAMPLE_ORDER).unwrap(),
                remaining_credential_count: 1,
                expires_at: Some(
                    Utc.with_ymd_and_hms(2023, 5, 17, 15, 04, 05)
                        .unwrap()
                        .naive_utc(),
                ),
                active: true,
            };

            let mut sdk = SDK::new(client, Environment::Production, None, None);
            sdk.now = Utc.timestamp_nanos(SAMPLE_TIME_LIMITED_V2_CREDENTIAL_VALID_NANOS);
            let result = sdk.matching_credential_summary(SAMPLE_ITEM_DOMAIN).await;
            assert_eq!(result, Ok(Some(expected)));
        })
    }
}
