// Copyright 2016 Joseph Birr-Pixton.
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHORS DISCLAIM ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#![cfg(any(feature = "ring", feature = "aws_lc_rs"))]

use core::time::Duration;

use pki_types::{CertificateDer, UnixTime};
use webpki::{anchor_from_trusted_cert, KeyUsage};

/* Checks we can verify netflix's cert chain.  This is notable
 * because they're rooted at a Verisign v1 root. */
#[cfg(feature = "alloc")]
#[test]
fn netflix() {
    let ee: &[u8] = include_bytes!("netflix/ee.der");
    let inter = CertificateDer::from(&include_bytes!("netflix/inter.der")[..]);
    let ca = CertificateDer::from(&include_bytes!("netflix/ca.der")[..]);

    let anchors = [anchor_from_trusted_cert(&ca).unwrap()];

    let time = UnixTime::since_unix_epoch(Duration::from_secs(1_492_441_716)); // 2017-04-17T15:08:36Z

    let ee = CertificateDer::from(ee);
    let cert = webpki::EndEntityCert::try_from(&ee).unwrap();
    assert!(cert
        .verify_for_usage(
            webpki::ALL_VERIFICATION_ALGS,
            &anchors,
            &[inter],
            time,
            KeyUsage::server_auth(),
            None,
            None,
        )
        .is_ok());
}

/* This is notable because it is a popular use of IP address subjectAltNames. */
#[cfg(feature = "alloc")]
#[test]
fn cloudflare_dns() {
    use pki_types::ServerName;

    let ee: &[u8] = include_bytes!("cloudflare_dns/ee.der");
    let inter = CertificateDer::from(&include_bytes!("cloudflare_dns/inter.der")[..]);
    let ca = CertificateDer::from(&include_bytes!("cloudflare_dns/ca.der")[..]);

    let ca_cert = CertificateDer::from(&ca[..]);
    let anchors = [anchor_from_trusted_cert(&ca_cert).unwrap()];

    let time = UnixTime::since_unix_epoch(Duration::from_secs(1_663_495_771));

    let ee = CertificateDer::from(ee);
    let cert = webpki::EndEntityCert::try_from(&ee).unwrap();
    assert!(cert
        .verify_for_usage(
            webpki::ALL_VERIFICATION_ALGS,
            &anchors,
            &[inter],
            time,
            KeyUsage::server_auth(),
            None,
            None,
        )
        .is_ok());

    let check_name = |name: &str| {
        let subject_name_ref = ServerName::try_from(name).unwrap();
        assert_eq!(
            Ok(()),
            cert.verify_is_valid_for_subject_name(&subject_name_ref)
        );
        println!("{:?} ok as name", name);
    };

    let check_addr = |addr: &str| {
        let subject_name_ref = ServerName::try_from(addr.as_bytes()).unwrap();
        assert_eq!(
            Ok(()),
            cert.verify_is_valid_for_subject_name(&subject_name_ref)
        );
        println!("{:?} ok as address", addr);
    };

    check_name("cloudflare-dns.com");
    check_name("wildcard.cloudflare-dns.com");
    check_name("one.one.one.one");
    check_addr("1.1.1.1");
    check_addr("1.0.0.1");
    check_addr("162.159.36.1");
    check_addr("162.159.46.1");
    check_addr("2606:4700:4700:0000:0000:0000:0000:1111");
    check_addr("2606:4700:4700:0000:0000:0000:0000:1001");
    check_addr("2606:4700:4700:0000:0000:0000:0000:0064");
    check_addr("2606:4700:4700:0000:0000:0000:0000:6400");
}

#[cfg(feature = "alloc")]
#[test]
fn wpt() {
    let ee = CertificateDer::from(&include_bytes!("wpt/ee.der")[..]);
    let ca = CertificateDer::from(&include_bytes!("wpt/ca.der")[..]);

    let anchors = [anchor_from_trusted_cert(&ca).unwrap()];

    let time = UnixTime::since_unix_epoch(Duration::from_secs(1_619_256_684)); // 2021-04-24T09:31:24Z
    let cert = webpki::EndEntityCert::try_from(&ee).unwrap();
    assert!(cert
        .verify_for_usage(
            webpki::ALL_VERIFICATION_ALGS,
            &anchors,
            &[],
            time,
            KeyUsage::server_auth(),
            None,
            None,
        )
        .is_ok());
}

#[test]
fn ed25519() {
    let ee = CertificateDer::from(&include_bytes!("ed25519/ee.der")[..]);
    let ca = CertificateDer::from(&include_bytes!("ed25519/ca.der")[..]);

    let anchors = [anchor_from_trusted_cert(&ca).unwrap()];

    let time = UnixTime::since_unix_epoch(Duration::from_secs(1_547_363_522)); // 2019-01-13T07:12:02Z

    let cert = webpki::EndEntityCert::try_from(&ee).unwrap();
    assert!(cert
        .verify_for_usage(
            webpki::ALL_VERIFICATION_ALGS,
            &anchors,
            &[],
            time,
            KeyUsage::server_auth(),
            None,
            None,
        )
        .is_ok());
}

#[test]
#[cfg(feature = "alloc")]
fn critical_extensions() {
    let root = CertificateDer::from(&include_bytes!("critical_extensions/root-cert.der")[..]);
    let ca = CertificateDer::from(&include_bytes!("critical_extensions/ca-cert.der")[..]);

    let time = UnixTime::since_unix_epoch(Duration::from_secs(1_670_779_098));
    let anchors = [anchor_from_trusted_cert(&root).unwrap()];
    let intermediates = [ca];

    let ee = CertificateDer::from(
        &include_bytes!("critical_extensions/ee-cert-noncrit-unknown-ext.der")[..],
    );
    let ee_cert = webpki::EndEntityCert::try_from(&ee).unwrap();
    assert!(
        ee_cert
            .verify_for_usage(
                webpki::ALL_VERIFICATION_ALGS,
                &anchors,
                &intermediates,
                time,
                KeyUsage::server_auth(),
                None,
                None,
            )
            .is_ok(),
        "accept non-critical unknown extension"
    );

    let ee = CertificateDer::from(
        &include_bytes!("critical_extensions/ee-cert-crit-unknown-ext.der")[..],
    );
    assert!(
        matches!(
            webpki::EndEntityCert::try_from(&ee),
            Err(webpki::Error::UnsupportedCriticalExtension)
        ),
        "reject critical unknown extension"
    );
}

#[test]
fn read_root_with_zero_serial() {
    let ca = CertificateDer::from(&include_bytes!("misc/serial_zero.der")[..]);
    anchor_from_trusted_cert(&ca).expect("godaddy cert should parse as anchor");
}

#[test]
fn read_root_with_neg_serial() {
    let ca = CertificateDer::from(&include_bytes!("misc/serial_neg.der")[..]);
    anchor_from_trusted_cert(&ca).expect("idcat cert should parse as anchor");
}

#[test]
#[cfg(feature = "alloc")]
fn read_ee_with_neg_serial() {
    let ca = CertificateDer::from(&include_bytes!("misc/serial_neg_ca.der")[..]);
    let ee = CertificateDer::from(&include_bytes!("misc/serial_neg_ee.der")[..]);

    let anchors = [anchor_from_trusted_cert(&ca).unwrap()];

    let time = UnixTime::since_unix_epoch(Duration::from_secs(1_667_401_500)); // 2022-11-02T15:05:00Z

    let cert = webpki::EndEntityCert::try_from(&ee).unwrap();
    assert!(cert
        .verify_for_usage(
            webpki::ALL_VERIFICATION_ALGS,
            &anchors,
            &[],
            time,
            KeyUsage::server_auth(),
            None,
            None,
        )
        .is_ok());
}

#[test]
#[cfg(feature = "alloc")]
fn read_ee_with_large_pos_serial() {
    let ee = CertificateDer::from(&include_bytes!("misc/serial_large_positive.der")[..]);

    webpki::EndEntityCert::try_from(&ee).expect("should parse 20-octet positive serial number");
}

#[test]
fn list_netflix_names() {
    expect_cert_dns_names(
        include_bytes!("netflix/ee.der"),
        [
            "account.netflix.com",
            "ca.netflix.com",
            "netflix.ca",
            "netflix.com",
            "signup.netflix.com",
            "www.netflix.ca",
            "www1.netflix.com",
            "www2.netflix.com",
            "www3.netflix.com",
            "develop-stage.netflix.com",
            "release-stage.netflix.com",
            "www.netflix.com",
        ],
    );
}

#[test]
fn invalid_subject_alt_names() {
    expect_cert_dns_names(
        // same as netflix ee certificate, but with the last name in the list
        // changed to 'www.netflix:com'
        include_bytes!("misc/invalid_subject_alternative_name.der"),
        [
            "account.netflix.com",
            "ca.netflix.com",
            "netflix.ca",
            "netflix.com",
            "signup.netflix.com",
            "www.netflix.ca",
            "www1.netflix.com",
            "www2.netflix.com",
            "www3.netflix.com",
            "develop-stage.netflix.com",
            "release-stage.netflix.com",
            // NOT 'www.netflix:com'
        ],
    );
}

#[test]
fn wildcard_subject_alternative_names() {
    expect_cert_dns_names(
        // same as netflix ee certificate, but with the last name in the list
        // changed to 'ww*.netflix:com'
        include_bytes!("misc/dns_names_and_wildcards.der"),
        [
            "account.netflix.com",
            "*.netflix.com",
            "netflix.ca",
            "netflix.com",
            "signup.netflix.com",
            "www.netflix.ca",
            "www1.netflix.com",
            "www2.netflix.com",
            "www3.netflix.com",
            "develop-stage.netflix.com",
            "release-stage.netflix.com",
            "www.netflix.com",
        ],
    );
}

#[test]
fn no_subject_alt_names() {
    expect_cert_dns_names(include_bytes!("misc/no_subject_alternative_name.der"), [])
}

fn expect_cert_dns_names<'name>(
    cert_der: &[u8],
    expected_names: impl IntoIterator<Item = &'name str>,
) {
    let der = CertificateDer::from(cert_der);
    let cert = webpki::EndEntityCert::try_from(&der)
        .expect("should parse end entity certificate correctly");

    assert!(cert.valid_dns_names().eq(expected_names))
}
