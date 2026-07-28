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
        dbus
        zstd
        lz4
      ];

      buildPhase = ''
        make SENDER=${if sender then "1" else "0"}
      '';

      installPhase = ''
        make install SENDER=${if sender then "1" else "0"} PREFIX=$out
      '';
    };
  in {
    packages.${system} = {
      default = mkSnipx true;
      no-sender = mkSnipx false;
    };

    devShells.${system} = {
      default = pkgs.mkShell {
        inputsFromLayout = [ ];
        inputsFrom = [ self.packages.${system}.default ];
        packages = with pkgs; [ go ];
      };
      no-sender = pkgs.mkShell {
        inputsFrom = [ self.packages.${system}.no-sender ];
      };
    };
  };
}
