{
  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };

    mkSnipx = sender: pkgs.stdenv.mkDerivation {
      pname = "snipx";
      version = "0.0.0";
      src = ./.;

      nativeBuildInputs = with pkgs; [
        gnumake
        pkg-config
      ]
      ++ lib.optionals sender [
        go
      ];

      buildInputs = with pkgs; [
        libx11
        libxinerama
        libxext
        libpulseaudio
        libnotify
        glib
        gdk-pixbuf
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
