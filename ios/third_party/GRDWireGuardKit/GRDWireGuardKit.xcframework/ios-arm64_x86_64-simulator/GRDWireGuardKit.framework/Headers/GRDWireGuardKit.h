//
//  GRDWireGuardKit.h
//  GRDWireGuardKit
//
//  Created by Constantin Jacob on 23.11.22.
//

#ifndef GRDWireGuardKit_h
#define GRDWireGuardKit_h


// Note from CJ 2022-11-23
// The required headers from the WireGuard project to make GRDWireGuardKit
// self contained and portable have the imported specifically in the way it is done below
// and all the headers need to be publically exposed in GRDWireGuardKit so that the 
// importing app can use them
#import "GRDWireGuardKit/WireGuardKitC.h"
#import <GRDWireGuardKit/wireguard.h>
#import <GRDWireGuardKit/ringlogger.h>
#import <GRDWireGuardKit/key.h>
#import <GRDWireGuardKit/x25519.h>


#endif /* GRDWireGuardKit_h */



