{
  description = "Rust multihash implementation";
  inputs = {
    nixpkgs.url = github:nixos/nixpkgs;
    flake-utils = {
      url = github:numtide/flake-utils;
      inputs.nixpkgs.follows = "nixpkgs";
    };
    naersk = {
      url = github:yatima-inc/naersk;
      inputs.nixpkgs.follows = "nixpkgs";
    };
    utils = {
      url = github:yatima-inc/nix-utils;
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.naersk.follows = "naersk";
    };
  };

  outputs =
    { self
    , nixpkgs
    , flake-utils
    , utils
    , naersk
    }:
    let 
      # Only use the supported systems
      supportedSystems = builtins.attrNames naersk.lib;
    in
    flake-utils.lib.eachSystem supportedSystems (system:
    let
      lib = utils.lib.${system};
      pkgs = import nixpkgs { inherit system; };
      inherit (lib) buildRustProject testRustProject getRust;
      rustNightly = getRust { date = "2021-10-13"; sha256 = "2hYUzd1vkONFeibPF2ZVOWR5LhKGecA0+Dq4/fTyNMg="; };
      crateName = "rust-multihash";
      root = ./.;
      project = buildRustProject { inherit root; rust = rustNightly; };
    in
    {
      packages.${crateName} = project;
      checks.${crateName} = testRustProject { 
        inherit root;
        rust = rustNightly;
        # Avoid unstable_options in test
        cargoOptions = opt: [];
        cargoBuildOptions = opt: [ "-Z unstable-options" ] ++ opt;
        cargoTestOptions = opt: [ "--all-features" ] ++ opt;

      };

      defaultPackage = self.packages.${system}.${crateName};

      # To run with `nix run`
      apps.${crateName} = flake-utils.lib.mkApp {
        drv = project;
      };

      # `nix develop`
      devShell = pkgs.mkShell {
        inputsFrom = builtins.attrValues self.packages.${system};
        nativeBuildInputs = [ rustNightly ];
        buildInputs = with pkgs; [
          rust-analyzer
          clippy
          rustfmt
        ];
      };
    });
}
