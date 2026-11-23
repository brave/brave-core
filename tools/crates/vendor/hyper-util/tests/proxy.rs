#![cfg(all(feature = "client-legacy", feature = "client-proxy"))]

use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::{TcpListener, TcpStream};
use tower_service::Service;

use hyper_util::client::legacy::connect::HttpConnector;
use hyper_util::client::legacy::connect::proxy::{SocksV4, SocksV5, Tunnel};

#[cfg(not(miri))]
#[tokio::test]
async fn test_tunnel_works() {
    let tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let addr = tcp.local_addr().expect("local_addr");

    let proxy_dst = format!("http://{addr}").parse().expect("uri");
    let mut connector = Tunnel::new(proxy_dst, HttpConnector::new());
    let t1 = tokio::spawn(async move {
        let _conn = connector
            .call("https://hyper.rs".parse().unwrap())
            .await
            .expect("tunnel");
    });

    let t2 = tokio::spawn(async move {
        let (mut io, _) = tcp.accept().await.expect("accept");
        let mut buf = [0u8; 64];
        let n = io.read(&mut buf).await.expect("read 1");
        assert_eq!(
            &buf[..n],
            b"CONNECT hyper.rs:443 HTTP/1.1\r\nHost: hyper.rs:443\r\n\r\n"
        );
        io.write_all(b"HTTP/1.1 200 OK\r\n\r\n")
            .await
            .expect("write 1");
    });

    t1.await.expect("task 1");
    t2.await.expect("task 2");
}

#[cfg(not(miri))]
#[tokio::test]
async fn test_socks_v5_without_auth_works() {
    let proxy_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let proxy_addr = proxy_tcp.local_addr().expect("local_addr");
    let proxy_dst = format!("http://{proxy_addr}").parse().expect("uri");

    let target_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let target_addr = target_tcp.local_addr().expect("local_addr");
    let target_dst = format!("http://{target_addr}").parse().expect("uri");

    let mut connector = SocksV5::new(proxy_dst, HttpConnector::new());

    // Client
    //
    // Will use `SocksV5` to establish proxy tunnel.
    // Will send "Hello World!" to the target and receive "Goodbye!" back.
    let t1 = tokio::spawn(async move {
        let conn = connector.call(target_dst).await.expect("tunnel");
        let mut tcp = conn.into_inner();

        tcp.write_all(b"Hello World!").await.expect("write 1");

        let mut buf = [0u8; 64];
        let n = tcp.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], b"Goodbye!");
    });

    // Proxy
    //
    // Will receive CONNECT command from client.
    // Will connect to target and success code back to client.
    // Will blindly tunnel between client and target.
    let t2 = tokio::spawn(async move {
        let (mut to_client, _) = proxy_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 513];

        // negotiation req/res
        let n = to_client.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], [0x05, 0x01, 0x00]);

        to_client.write_all(&[0x05, 0x00]).await.expect("write 1");

        // command req/rs
        let [p1, p2] = target_addr.port().to_be_bytes();
        let [ip1, ip2, ip3, ip4] = [0x7f, 0x00, 0x00, 0x01];
        let message = [0x05, 0x01, 0x00, 0x01, ip1, ip2, ip3, ip4, p1, p2];
        let n = to_client.read(&mut buf).await.expect("read 2");
        assert_eq!(&buf[..n], message);

        let mut to_target = TcpStream::connect(target_addr).await.expect("connect");

        let message = [0x05, 0x00, 0x00, 0x01, ip1, ip2, ip3, ip4, p1, p2];
        to_client.write_all(&message).await.expect("write 2");

        let (from_client, from_target) =
            tokio::io::copy_bidirectional(&mut to_client, &mut to_target)
                .await
                .expect("proxy");

        assert_eq!(from_client, 12);
        assert_eq!(from_target, 8)
    });

    // Target server
    //
    // Will accept connection from proxy server
    // Will receive "Hello World!" from the client and return "Goodbye!"
    let t3 = tokio::spawn(async move {
        let (mut io, _) = target_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 64];

        let n = io.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], b"Hello World!");

        io.write_all(b"Goodbye!").await.expect("write 1");
    });

    t1.await.expect("task - client");
    t2.await.expect("task - proxy");
    t3.await.expect("task - target");
}

#[cfg(not(miri))]
#[tokio::test]
async fn test_socks_v5_with_auth_works() {
    let proxy_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let proxy_addr = proxy_tcp.local_addr().expect("local_addr");
    let proxy_dst = format!("http://{proxy_addr}").parse().expect("uri");

    let target_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let target_addr = target_tcp.local_addr().expect("local_addr");
    let target_dst = format!("http://{target_addr}").parse().expect("uri");

    let mut connector =
        SocksV5::new(proxy_dst, HttpConnector::new()).with_auth("user".into(), "pass".into());

    // Client
    //
    // Will use `SocksV5` to establish proxy tunnel.
    // Will send "Hello World!" to the target and receive "Goodbye!" back.
    let t1 = tokio::spawn(async move {
        let conn = connector.call(target_dst).await.expect("tunnel");
        let mut tcp = conn.into_inner();

        tcp.write_all(b"Hello World!").await.expect("write 1");

        let mut buf = [0u8; 64];
        let n = tcp.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], b"Goodbye!");
    });

    // Proxy
    //
    // Will receive CONNECT command from client.
    // Will connect to target and success code back to client.
    // Will blindly tunnel between client and target.
    let t2 = tokio::spawn(async move {
        let (mut to_client, _) = proxy_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 513];

        // negotiation req/res
        let n = to_client.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], [0x05, 0x01, 0x02]);

        to_client.write_all(&[0x05, 0x02]).await.expect("write 1");

        // auth req/res
        let n = to_client.read(&mut buf).await.expect("read 2");
        let [u1, u2, u3, u4] = b"user";
        let [p1, p2, p3, p4] = b"pass";
        let message = [0x01, 0x04, *u1, *u2, *u3, *u4, 0x04, *p1, *p2, *p3, *p4];
        assert_eq!(&buf[..n], message);

        to_client.write_all(&[0x01, 0x00]).await.expect("write 2");

        // command req/res
        let n = to_client.read(&mut buf).await.expect("read 3");
        let [p1, p2] = target_addr.port().to_be_bytes();
        let [ip1, ip2, ip3, ip4] = [0x7f, 0x00, 0x00, 0x01];
        let message = [0x05, 0x01, 0x00, 0x01, ip1, ip2, ip3, ip4, p1, p2];
        assert_eq!(&buf[..n], message);

        let mut to_target = TcpStream::connect(target_addr).await.expect("connect");

        let message = [0x05, 0x00, 0x00, 0x01, ip1, ip2, ip3, ip4, p1, p2];
        to_client.write_all(&message).await.expect("write 3");

        let (from_client, from_target) =
            tokio::io::copy_bidirectional(&mut to_client, &mut to_target)
                .await
                .expect("proxy");

        assert_eq!(from_client, 12);
        assert_eq!(from_target, 8)
    });

    // Target server
    //
    // Will accept connection from proxy server
    // Will receive "Hello World!" from the client and return "Goodbye!"
    let t3 = tokio::spawn(async move {
        let (mut io, _) = target_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 64];

        let n = io.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], b"Hello World!");

        io.write_all(b"Goodbye!").await.expect("write 1");
    });

    t1.await.expect("task - client");
    t2.await.expect("task - proxy");
    t3.await.expect("task - target");
}

#[cfg(not(miri))]
#[tokio::test]
async fn test_socks_v5_with_server_resolved_domain_works() {
    let proxy_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let proxy_addr = proxy_tcp.local_addr().expect("local_addr");
    let proxy_addr = format!("http://{proxy_addr}").parse().expect("uri");

    let mut connector = SocksV5::new(proxy_addr, HttpConnector::new())
        .with_auth("user".into(), "pass".into())
        .local_dns(false);

    // Client
    //
    // Will use `SocksV5` to establish proxy tunnel.
    // Will send "Hello World!" to the target and receive "Goodbye!" back.
    let t1 = tokio::spawn(async move {
        let _conn = connector
            .call("https://hyper.rs:443".try_into().unwrap())
            .await
            .expect("tunnel");
    });

    // Proxy
    //
    // Will receive CONNECT command from client.
    // Will connect to target and success code back to client.
    // Will blindly tunnel between client and target.
    let t2 = tokio::spawn(async move {
        let (mut to_client, _) = proxy_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 513];

        // negotiation req/res
        let n = to_client.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], [0x05, 0x01, 0x02]);

        to_client.write_all(&[0x05, 0x02]).await.expect("write 1");

        // auth req/res
        let n = to_client.read(&mut buf).await.expect("read 2");
        let [u1, u2, u3, u4] = b"user";
        let [p1, p2, p3, p4] = b"pass";
        let message = [0x01, 0x04, *u1, *u2, *u3, *u4, 0x04, *p1, *p2, *p3, *p4];
        assert_eq!(&buf[..n], message);

        to_client.write_all(&[0x01, 0x00]).await.expect("write 2");

        // command req/res
        let n = to_client.read(&mut buf).await.expect("read 3");

        let host = "hyper.rs";
        let port: u16 = 443;
        let mut message = vec![0x05, 0x01, 0x00, 0x03, host.len() as u8];
        message.extend(host.bytes());
        message.extend(port.to_be_bytes());
        assert_eq!(&buf[..n], message);

        let mut message = vec![0x05, 0x00, 0x00, 0x03, host.len() as u8];
        message.extend(host.bytes());
        message.extend(port.to_be_bytes());
        to_client.write_all(&message).await.expect("write 3");
    });

    t1.await.expect("task - client");
    t2.await.expect("task - proxy");
}

#[cfg(not(miri))]
#[tokio::test]
async fn test_socks_v5_with_locally_resolved_domain_works() {
    let proxy_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let proxy_addr = proxy_tcp.local_addr().expect("local_addr");
    let proxy_addr = format!("http://{proxy_addr}").parse().expect("uri");

    let mut connector = SocksV5::new(proxy_addr, HttpConnector::new())
        .with_auth("user".into(), "pass".into())
        .local_dns(true);

    // Client
    //
    // Will use `SocksV5` to establish proxy tunnel.
    // Will send "Hello World!" to the target and receive "Goodbye!" back.
    let t1 = tokio::spawn(async move {
        let _conn = connector
            .call("https://hyper.rs:443".try_into().unwrap())
            .await
            .expect("tunnel");
    });

    // Proxy
    //
    // Will receive CONNECT command from client.
    // Will connect to target and success code back to client.
    // Will blindly tunnel between client and target.
    let t2 = tokio::spawn(async move {
        let (mut to_client, _) = proxy_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 513];

        // negotiation req/res
        let n = to_client.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], [0x05, 0x01, 0x02]);

        to_client.write_all(&[0x05, 0x02]).await.expect("write 1");

        // auth req/res
        let n = to_client.read(&mut buf).await.expect("read 2");
        let [u1, u2, u3, u4] = b"user";
        let [p1, p2, p3, p4] = b"pass";
        let message = [0x01, 0x04, *u1, *u2, *u3, *u4, 0x04, *p1, *p2, *p3, *p4];
        assert_eq!(&buf[..n], message);

        to_client.write_all(&[0x01, 0x00]).await.expect("write 2");

        // command req/res
        let n = to_client.read(&mut buf).await.expect("read 3");
        let message = [0x05, 0x01, 0x00];
        assert_eq!(&buf[..3], message);
        assert!(buf[3] == 0x01 || buf[3] == 0x04); // IPv4 or IPv6
        assert_eq!(n, 4 + 4 * (buf[3] as usize) + 2);

        let message = vec![0x05, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0, 0];
        to_client.write_all(&message).await.expect("write 3");
    });

    t1.await.expect("task - client");
    t2.await.expect("task - proxy");
}

#[cfg(not(miri))]
#[tokio::test]
async fn test_socks_v4_works() {
    let proxy_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let proxy_addr = proxy_tcp.local_addr().expect("local_addr");
    let proxy_dst = format!("http://{proxy_addr}").parse().expect("uri");

    let target_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let target_addr = target_tcp.local_addr().expect("local_addr");
    let target_dst = format!("http://{target_addr}").parse().expect("uri");

    let mut connector = SocksV4::new(proxy_dst, HttpConnector::new());

    // Client
    //
    // Will use `SocksV4` to establish proxy tunnel.
    // Will send "Hello World!" to the target and receive "Goodbye!" back.
    let t1 = tokio::spawn(async move {
        let conn = connector.call(target_dst).await.expect("tunnel");
        let mut tcp = conn.into_inner();

        tcp.write_all(b"Hello World!").await.expect("write 1");

        let mut buf = [0u8; 64];
        let n = tcp.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], b"Goodbye!");
    });

    // Proxy
    //
    // Will receive CONNECT command from client.
    // Will connect to target and success code back to client.
    // Will blindly tunnel between client and target.
    let t2 = tokio::spawn(async move {
        let (mut to_client, _) = proxy_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 512];

        let [p1, p2] = target_addr.port().to_be_bytes();
        let [ip1, ip2, ip3, ip4] = [127, 0, 0, 1];
        let message = [4, 0x01, p1, p2, ip1, ip2, ip3, ip4, 0];
        let n = to_client.read(&mut buf).await.expect("read");
        assert_eq!(&buf[..n], message);

        let mut to_target = TcpStream::connect(target_addr).await.expect("connect");

        let message = [0, 90, p1, p2, ip1, ip2, ip3, ip4];
        to_client.write_all(&message).await.expect("write");

        let (from_client, from_target) =
            tokio::io::copy_bidirectional(&mut to_client, &mut to_target)
                .await
                .expect("proxy");

        assert_eq!(from_client, 12);
        assert_eq!(from_target, 8)
    });

    // Target server
    //
    // Will accept connection from proxy server
    // Will receive "Hello World!" from the client and return "Goodbye!"
    let t3 = tokio::spawn(async move {
        let (mut io, _) = target_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 64];

        let n = io.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], b"Hello World!");

        io.write_all(b"Goodbye!").await.expect("write 1");
    });

    t1.await.expect("task - client");
    t2.await.expect("task - proxy");
    t3.await.expect("task - target");
}

#[cfg(not(miri))]
#[tokio::test]
async fn test_socks_v4a_with_server_resolved_domain_works() {
    let proxy_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let proxy_addr = proxy_tcp.local_addr().expect("local_addr");
    let proxy_dst = format!("http://{proxy_addr}").parse().expect("uri");

    let mut connector = SocksV4::new(proxy_dst, HttpConnector::new());

    // Client
    //
    // Will use `SocksV4` to establish a tunnel to a domain name, leaving
    // resolution to the proxy server (the SOCKS4a extension).
    let t1 = tokio::spawn(async move {
        let _conn = connector
            .call("https://hyper.rs:443".try_into().unwrap())
            .await
            .expect("tunnel");
    });

    // Proxy
    //
    // Will receive a SOCKS4a CONNECT command carrying the domain name, and
    // reply with a success status.
    let t2 = tokio::spawn(async move {
        let (mut to_client, _) = proxy_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 512];

        let host = "hyper.rs";
        let [p1, p2] = 443u16.to_be_bytes();

        // VN, CD, DSTPORT, DSTIP (0.0.0.x marks a SOCKS4a request), then the
        // NULL terminating an empty USERID, then the NULL-terminated domain.
        let mut message = vec![0x04, 0x01, p1, p2, 0x00, 0x00, 0x00, 0xFF, 0x00];
        message.extend(host.as_bytes());
        message.push(0x00);

        let n = to_client.read(&mut buf).await.expect("read");
        assert_eq!(&buf[..n], message);

        let message = [0, 90, p1, p2, 0, 0, 0, 0];
        to_client.write_all(&message).await.expect("write");
    });

    t1.await.expect("task - client");
    t2.await.expect("task - proxy");
}

#[cfg(not(miri))]
#[tokio::test]
async fn test_socks_v5_optimistic_works() {
    let proxy_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let proxy_addr = proxy_tcp.local_addr().expect("local_addr");
    let proxy_dst = format!("http://{proxy_addr}").parse().expect("uri");

    let target_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let target_addr = target_tcp.local_addr().expect("local_addr");
    let target_dst = format!("http://{target_addr}").parse().expect("uri");

    let mut connector = SocksV5::new(proxy_dst, HttpConnector::new())
        .with_auth("ABC".into(), "XYZ".into())
        .send_optimistically(true);

    // Client
    //
    // Will use `SocksV5` to establish proxy tunnel.
    // Will send "Hello World!" to the target and receive "Goodbye!" back.
    let t1 = tokio::spawn(async move {
        let conn = tokio::time::timeout(
            std::time::Duration::from_secs(5),
            connector.call(target_dst),
        )
        .await
        .expect("handshake timed out")
        .expect("tunnel");

        let mut tcp = conn.into_inner();
        tcp.write_all(b"Hello World!").await.expect("write 1");

        let mut buf = [0u8; 64];
        let n = tcp.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], b"Goodbye!");
    });

    // Proxy
    //
    // Will receive SOCKS handshake from client.
    // Will connect to target and success code back to client.
    // Will blindly tunnel between client and target.
    let t2 = tokio::spawn(async move {
        let (mut to_client, _) = proxy_tcp.accept().await.expect("accept");
        let [p1, p2] = target_addr.port().to_be_bytes();

        let mut buf = [0; 22];
        let request = vec![
            5, 1, 2, // Negotiation
            1, 3, 65, 66, 67, 3, 88, 89, 90, // Auth ("ABC"/"XYZ")
            5, 1, 0, 1, 127, 0, 0, 1, p1, p2, // Reply
        ];

        let response = vec![
            5, 2, // Negotiation,
            1, 0, // Auth,
            5, 0, 0, 1, 127, 0, 0, 1, p1, p2, // Reply
        ];

        // Accept all handshake messages
        to_client.read_exact(&mut buf).await.expect("read");
        assert_eq!(request.as_slice(), buf);

        let mut to_target = TcpStream::connect(target_addr).await.expect("connect");

        // Send all handshake messages back
        to_client
            .write_all(response.as_slice())
            .await
            .expect("write");

        to_client.flush().await.expect("flush");

        let (from_client, from_target) =
            tokio::io::copy_bidirectional(&mut to_client, &mut to_target)
                .await
                .expect("proxy");

        assert_eq!(from_client, 12);
        assert_eq!(from_target, 8)
    });

    // Target server
    //
    // Will accept connection from proxy server
    // Will receive "Hello World!" from the client and return "Goodbye!"
    let t3 = tokio::spawn(async move {
        let (mut io, _) = target_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 64];

        let n = io.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], b"Hello World!");

        io.write_all(b"Goodbye!").await.expect("write 1");
    });

    t1.await.expect("task - client");
    t2.await.expect("task - proxy");
    t3.await.expect("task - target");
}

#[cfg(not(miri))]
#[tokio::test]
async fn test_socks_v5_with_ipv6_target_works() {
    let proxy_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let proxy_addr = proxy_tcp.local_addr().expect("local_addr");
    let proxy_dst = format!("http://{proxy_addr}").parse().expect("uri");

    let target_tcp = TcpListener::bind("[::1]:0").await.expect("bind");
    let target_addr = target_tcp.local_addr().expect("local_addr");
    let target_dst = format!("http://{target_addr}").parse().expect("uri");

    let mut connector = SocksV5::new(proxy_dst, HttpConnector::new());

    // Client
    //
    // Will use `SocksV5` to establish proxy tunnel to an IPv6 target.
    // The `host()` of the target URI is a bracketed IPv6 address (e.g. "[::1]"),
    // which must be stripped of its brackets before being parsed as an IP.
    // Will send "Hello World!" to the target and receive "Goodbye!" back.
    let t1 = tokio::spawn(async move {
        let conn = connector.call(target_dst).await.expect("tunnel");
        let mut tcp = conn.into_inner();

        tcp.write_all(b"Hello World!").await.expect("write 1");

        let mut buf = [0u8; 64];
        let n = tcp.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], b"Goodbye!");
    });

    // Proxy
    //
    // Will receive CONNECT command from client with an IPv6 address (ATYP 0x04).
    // Will connect to target and success code back to client.
    // Will blindly tunnel between client and target.
    let t2 = tokio::spawn(async move {
        let (mut to_client, _) = proxy_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 513];

        // negotiation req/res
        let n = to_client.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], [0x05, 0x01, 0x00]);

        to_client.write_all(&[0x05, 0x00]).await.expect("write 1");

        // command req/res
        let n = to_client.read(&mut buf).await.expect("read 2");
        let [p1, p2] = target_addr.port().to_be_bytes();
        let mut message = vec![0x05, 0x01, 0x00, 0x04];
        match target_addr.ip() {
            std::net::IpAddr::V6(v6) => message.extend_from_slice(&v6.octets()),
            _ => panic!("expected IPv6 target"),
        }
        message.extend_from_slice(&[p1, p2]);
        assert_eq!(&buf[..n], message);

        let mut to_target = TcpStream::connect(target_addr).await.expect("connect");

        let mut reply = vec![0x05, 0x00, 0x00, 0x04];
        match target_addr.ip() {
            std::net::IpAddr::V6(v6) => reply.extend_from_slice(&v6.octets()),
            _ => panic!("expected IPv6 target"),
        }
        reply.extend_from_slice(&[p1, p2]);
        to_client.write_all(&reply).await.expect("write 2");

        let (from_client, from_target) =
            tokio::io::copy_bidirectional(&mut to_client, &mut to_target)
                .await
                .expect("proxy");

        assert_eq!(from_client, 12);
        assert_eq!(from_target, 8)
    });

    // Target server
    //
    // Will accept connection from proxy server
    // Will receive "Hello World!" from the client and return "Goodbye!"
    let t3 = tokio::spawn(async move {
        let (mut io, _) = target_tcp.accept().await.expect("accept");
        let mut buf = [0u8; 64];

        let n = io.read(&mut buf).await.expect("read 1");
        assert_eq!(&buf[..n], b"Hello World!");

        io.write_all(b"Goodbye!").await.expect("write 1");
    });

    t1.await.expect("task - client");
    t2.await.expect("task - proxy");
    t3.await.expect("task - target");
}

#[cfg(not(miri))]
#[tokio::test]
async fn test_socks_v4_with_ipv6_target_fails() {
    let proxy_tcp = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let proxy_addr = proxy_tcp.local_addr().expect("local_addr");
    let proxy_dst = format!("http://{proxy_addr}").parse().expect("uri");

    // SOCKSv4 does not support IPv6. A bracketed IPv6 target URI must be
    // rejected with `SocksV4Error::IpV6` after the brackets are stripped and
    // the remaining host is parsed as an IPv6 address.
    let target_addr = std::net::SocketAddr::new(std::net::Ipv6Addr::LOCALHOST.into(), 1234);
    let target_dst = format!("http://{target_addr}").parse().expect("uri");

    let mut connector = SocksV4::new(proxy_dst, HttpConnector::new());

    // Client
    //
    // Will use `SocksV4` to attempt a proxy tunnel to an IPv6 target.
    // SOCKSv4 does not support IPv6, so this should fail.
    let t1 = tokio::spawn(async move {
        let result = connector.call(target_dst).await;
        assert!(result.is_err(), "expected SOCKSv4 to reject IPv6 target");
        let err = result.unwrap_err().to_string();
        assert!(
            err.contains("IPV6 is not supported"),
            "unexpected error: {err}"
        );
    });

    // Proxy
    //
    // Will accept the underlying connection from the client. No SOCKSv4
    // handshake is sent because the client rejects the IPv6 target before
    // serializing the request.
    let t2 = tokio::spawn(async move {
        let (_to_client, _) = proxy_tcp.accept().await.expect("accept");
    });

    t1.await.expect("task - client");
    t2.await.expect("task - proxy");
}
