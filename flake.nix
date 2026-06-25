{
  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };

    mkSnipx = sender: pkgs.stdenv.mkDerivation {
      pname = "snipx";
      version = "0.1.0";
      src = ./.;

      nativeBuildInputs =
      [
        pkgs.gnumake
        pkgs.pkg-config
      ]
      ++ pkgs.lib.optionals sender [
        pkgs.go
      ];

      buildInputs = [
        pkgs.libx11
        pkgs.libxinerama
        pkgs.libxext
        pkgs.libpulseaudio
      ];

      buildPhase = ''
        make SENDER=${if sender then "true" else "false"}
      '';

      installPhase = ''
        make install SENDER=${if sender then "true" else "false"} PREFIX=$out
      '';
    };
  in {
    packages.${system} = {
      default = mkSnipx true;
      no-sender = mkSnipx false;
    };
  };
}
