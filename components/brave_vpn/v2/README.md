# Brave VPN 2.0 Architecture

This directory contains the implementation of Brave VPN 2.0 architecture.
More information in the spec: 
https://docs.google.com/document/d/1X6EWWqrFGeSXUBXW9mn0o5bsFyeOzZvmuMTcWdvJ_dc

## Key Files

- **`brave_vpn_service_impl.{h,cc}`** - Main service implementation for V2.

## Related Files

- **`browser/brave_vpn/brave_vpn_service_factory.{h,cc}`** - Brave VPN service
  factory that creates an implementation based on the enabled buildflags.
