use std::{fs, io::Write, net::TcpStream, sync::Arc};

use rustls::{pki_types::ServerName, ClientConfig, ClientConnection, RootCertStore, Stream};
use webpki_root_certs::TLS_SERVER_ROOT_CERTS;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut roots = RootCertStore::empty();
    let (_, ignored) = roots.add_parsable_certificates(TLS_SERVER_ROOT_CERTS.iter().cloned());
    assert_eq!(ignored, 0, "{ignored} root certificates were ignored");
    let config = Arc::new(
        ClientConfig::builder()
            .with_root_certificates(roots)
            .with_no_client_auth(),
    );

    for &host in HOSTS {
        let server_name = ServerName::try_from(host)?;
        let mut conn = ClientConnection::new(config.clone(), server_name)?;
        let mut sock = TcpStream::connect((host, 443))?;
        let mut stream = Stream::new(&mut conn, &mut sock);

        eprintln!("connecting to {host}...");
        if let Err(err) = stream.write_all(b"GET / HTTP/1.1\r\n\r\n") {
            eprintln!("failed to write to {host}: {err}");
        }

        let Some(certs) = conn.peer_certificates() else {
            eprintln!("no certificates received for {host}");
            continue;
        };

        for (i, der) in certs.iter().enumerate() {
            let host_name = host.replace('.', "_");
            let fname = format!(
                "{}/src/tests/verification_real_world/{host_name}_valid_{}.crt",
                env!("CARGO_MANIFEST_DIR"),
                i + 1
            );
            fs::write(&fname, der.as_ref())?;
            eprintln!("wrote certificate to {fname}");
        }
    }

    Ok(())
}

// We use two different CAs for better coverage and...
const HOSTS: &[&str] = &[
    // This host is using EC-based certificates for coverage.
    "letsencrypt.org",
    // This host is using RSA-based certificates for coverage.
    "aws.amazon.com",
];
