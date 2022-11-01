mod fetch;
mod present;

use chrono::Utc;
use tracing::instrument;

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
                        if Utc::now().naive_utc() > (expires_at - chrono::Duration::days(5)) {
                            let refreshed = self.refresh_order_credentials(order_id).await;
                            if refreshed.is_err() {
                                continue;
                            }
                        }
                    }

                    if let Some(creds) = self.matching_time_limited_v2_credential(&item.id).await? {
                        let unblinded_creds =
                            creds.unblinded_creds.ok_or(InternalError::NotFound)?;
                        let remaining_credential_count =
                            unblinded_creds.into_iter().filter(|cred| !cred.spent).count();

                        let active = remaining_credential_count > 0;
                        return Ok(Some(CredentialSummary {
                            order,
                            remaining_credential_count, // number unspent
                            expires_at,
                            active,
                        }));
                    }
                }
                CredentialType::SingleUse => {
                    let wrapped_creds = self.client.get_single_use_item_creds(&item.id).await?;
                    if let Some(creds) = wrapped_creds {
                        let unblinded_creds =
                            creds.unblinded_creds.ok_or(InternalError::NotFound)?;
                        let remaining_credential_count =
                            unblinded_creds.into_iter().filter(|cred| !cred.spent).count();

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
                        if Utc::now().naive_utc() > (expires_at - chrono::Duration::days(5)) {
                            let refreshed = self.refresh_order_credentials(order_id).await;
                            if refreshed.is_err() {
                                continue;
                            }
                        }
                    }
                    let active = matches!(
                        self.matching_time_limited_credential(&item.id).await,
                        Ok(Some(_))
                    );

                    return Ok(Some(CredentialSummary {
                        order,
                        remaining_credential_count: 1,
                        expires_at,
                        active,
                    }));
                }
            };
        } // for
        Err(InternalError::NotFound.into())
    }

    #[instrument]
    pub async fn matching_time_limited_v2_credential(
        &self,
        item_id: &str,
    ) -> Result<Option<TimeLimitedV2Credential>, SkusError> {
        Ok(self.client.get_time_limited_v2_creds(item_id).await?.and_then(|tlv2| {
            tlv2.unblinded_creds.unwrap_or_default().into_iter().find(|cred| {
                Utc::now().naive_utc() < cred.valid_to && Utc::now().naive_utc() > cred.valid_from
            })
        }))
    }

    #[instrument]
    pub async fn matching_time_limited_credential(
        &self,
        item_id: &str,
    ) -> Result<Option<TimeLimitedCredential>, SkusError> {
        Ok(self.client.get_time_limited_creds(item_id).await?.and_then(|creds| {
            creds.creds.into_iter().find(|cred| {
                Utc::now().naive_utc() < cred.expires_at && Utc::now().naive_utc() > cred.issued_at
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
                    let wrapped_value =
                        self.matching_order_credential_summary(&order.id, domain).await;
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
