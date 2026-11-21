#!/usr/bin/env bash
# GamerTCP - Linux cross-build and packaging.
#
# Produces the same deliverables as build.bat + dist.bat, without Windows:
#
#   GamerTCP.exe                                  GUI + CLI executable
#   GamerTCP-Setup.exe                            per-user installer
#   dist/GamerTCP-<version>-source.zip            complete source
#   dist/GamerTCP-<version>-portable.zip          exe + documentation
#   dist/GamerTCP-<version>-setup.exe             the installer
#   dist/GamerTCP-<version>-SHA256SUMS.txt        hashes of the three above
#
# Zig cross-compiles the C++ and a MinGW-w64 windres compiles the resources.
# Both are overridable:  ZIG=/path/to/zig WINDRES=/path/to/windres ./build.sh
#
# Usage: ./build.sh [--no-installer] [--no-dist] [--clean]
#   --no-installer  build GamerTCP.exe only
#   --no-dist       build both executables, skip dist/ packaging
#   --clean         remove build/ before compiling
#
# The version comes from the newest "## [x.y.z]" heading in CHANGES.md, the
# same value selftest compares against the compiled version. selftest itself
# cannot run here, so the two are compared directly and a mismatch aborts.
set -euo pipefail

cd "$(dirname "$0")"

say() { printf '%s\n' "$*"; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

# ---------------------------------------------------------------- options

want_installer=1
want_dist=1
for arg in "$@"; do
    case "$arg" in
        --no-installer) want_installer=0; want_dist=0 ;;
        --no-dist)      want_dist=0 ;;
        --clean)        rm -rf build ;;
        -h|--help)      sed -n '2,23p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *)              die "unknown option: $arg (try --help)" ;;
    esac
done

# ---------------------------------------------------------------- toolchain

ZIG=${ZIG:-$HOME/tc/zig/zig}
WINDRES=${WINDRES:-$(command -v x86_64-w64-mingw32-windres ||
                     echo "$HOME/tc/bn/root/usr/bin/x86_64-w64-mingw32-windres")}
RCPP=${RCPP:-$HOME/tc/rcpp}

[ -x "$ZIG" ] || die "Zig not found at $ZIG.
  Install it from https://ziglang.org/download/ (0.14.x) or set ZIG=/path/to/zig."
[ -x "$WINDRES" ] || die "windres not found at $WINDRES.
  Install MinGW-w64 for resources (Debian: apt-get install binutils-mingw-w64-x86-64)
  or set WINDRES=/path/to/x86_64-w64-mingw32-windres."

BUILD=build
mkdir -p "$BUILD"

# windres needs a preprocessor that finds the Windows headers. The project ships
# a wrapper for that; otherwise generate one around this Zig.
if [ ! -x "$RCPP" ]; then
    RCPP="$BUILD/rcpp"
    printf '#!/bin/sh\nexec "%s" cc -target x86_64-windows-gnu -E "$@"\n' "$ZIG" > "$RCPP"
    chmod +x "$RCPP"
fi

# ---------------------------------------------------------------- version

version=$(sed -n 's/^## \[\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\)\].*/\1/p' CHANGES.md | head -n 1)
[ -n "$version" ] || die "cannot read the release version from CHANGES.md"

compiled=$(sed -n 's/.*VersionString() { return L"\([0-9][0-9.]*\)"; }.*/\1/p' src/engine.cpp | head -n 1)
[ -n "$compiled" ] || die "cannot read VersionString() from src/engine.cpp"
[ "$version" = "$compiled" ] ||
    die "CHANGES.md says $version but src/engine.cpp compiles $compiled; selftest would fail"

say "GamerTCP $version - cross-building with $(basename "$ZIG") and windres"

# ---------------------------------------------------------------- resources

say "resource compiler: $WINDRES"
( cd res &&
  "$WINDRES" --preprocessor="$RCPP" --use-temp-file -O coff \
      -i gamerTCP.rc -o "../$BUILD/gamerTCP.res.o" 2> >(grep -v 'digit exceeds base' >&2) &&
  "$WINDRES" --preprocessor="$RCPP" --use-temp-file -O coff \
      -i manifest_only.rc -o "../$BUILD/default-manifest.o" 2> >(grep -v 'digit exceeds base' >&2) )

# ---------------------------------------------------------------- executable

# default-manifest.o is linked explicitly: build.bat's MinGW path relies on
# GCC's startfile spec for it, which Zig does not do, so a build without it
# silently loses the manifest.
# Zig predefines _WIN32_WINNT for the target, so the explicit 0x0601 below is
# reported as a redefinition; the project value is the one that must win.
say "compiling GamerTCP.exe"
"$ZIG" c++ -target x86_64-windows-gnu -std=c++17 -O2 -Wall -Wextra \
    -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601 -Wno-macro-redefined \
    -Wl,--subsystem,windows -static \
    src/main.cpp src/engine.cpp src/cli.cpp src/gui.cpp \
    "$BUILD/gamerTCP.res.o" "$BUILD/default-manifest.o" \
    -o GamerTCP.exe \
    -lcomctl32 -lcomdlg32 -ladvapi32 -lshell32 -lgdi32 -luser32 -ldwmapi -luxtheme -lpowrprof
[ -s GamerTCP.exe ] || die "the compiler did not produce GamerTCP.exe"

# ---------------------------------------------------------------- installer

if [ "$want_installer" = 1 ]; then
    say "compiling GamerTCP-Setup.exe"
    # The installer embeds these files as RCDATA, so it needs absolute paths.
    # Forward slashes: windres treats the backslash in a quoted path as an
    # escape and then cannot open the file.
    root=$(pwd)
    {
        printf '101 RCDATA "%s/GamerTCP.exe"\n' "$root"
        printf '102 RCDATA "%s/README.md"\n' "$root"
        printf '103 RCDATA "%s/CHANGES.md"\n' "$root"
        printf '104 RCDATA "%s/LICENSE.md"\n' "$root"
        printf '201 ICON "%s/res/gamerTCP.ico"\n' "$root"
    } > "$BUILD/setup.rc"

    "$WINDRES" --preprocessor="$RCPP" --use-temp-file -O coff \
        -i "$BUILD/setup.rc" -o "$BUILD/setup.res.o" 2> >(grep -v 'digit exceeds base' >&2)

    "$ZIG" c++ -target x86_64-windows-gnu -std=c++17 -O2 -Wall -Wextra \
        -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601 -Wno-macro-redefined \
        -Wl,--subsystem,console -static \
        installer/setup.cpp "$BUILD/setup.res.o" -o GamerTCP-Setup.exe \
        -lole32 -lshell32 -ladvapi32 -luser32 -luuid
    [ -s GamerTCP-Setup.exe ] || die "the compiler did not produce GamerTCP-Setup.exe"
fi
rm -f ./*.pdb

# ---------------------------------------------------------------- packaging

if [ "$want_dist" = 1 ]; then
    name="GamerTCP-$version"
    source_dir="dist/$name-source"
    binary_dir="dist/$name"

    say "packaging dist/$name-*.zip, .exe and SHA256SUMS"
    rm -rf "$source_dir" "$binary_dir"
    rm -f "dist/$name-source.zip" "dist/$name-portable.zip" "dist/$name-setup.exe" \
          "dist/$name-SHA256SUMS.txt"
    mkdir -p "$source_dir" "$binary_dir"

    # Source distribution: everything needed to rebuild both executables.
    cp build.bat dist.bat README.md CHANGES.md LICENSE.md "$source_dir/"
    cp -r src res installer "$source_dir/"

    # Portable distribution: the executable plus the documentation.
    cp GamerTCP.exe README.md CHANGES.md LICENSE.md "$binary_dir/"

    ( cd dist &&
      zip -qr "$name-source.zip" "$name-source" &&
      zip -qr "$name-portable.zip" "$name" )
    cp GamerTCP-Setup.exe "dist/$name-setup.exe"

    ( cd dist && sha256sum "$name-source.zip" "$name-portable.zip" "$name-setup.exe" \
        > "$name-SHA256SUMS.txt" && sha256sum -c "$name-SHA256SUMS.txt" )
fi

# ---------------------------------------------------------------- summary

say ""
say "Built:"
binaries="GamerTCP.exe"
[ "$want_installer" = 1 ] && binaries="$binaries GamerTCP-Setup.exe"
for file in $binaries; do
    say "  $(printf '%-28s' "$file") $(stat -c %s "$file") bytes"
done
if [ "$want_dist" = 1 ]; then
    for file in dist/GamerTCP-"$version"-source.zip dist/GamerTCP-"$version"-portable.zip \
                dist/GamerTCP-"$version"-setup.exe dist/GamerTCP-"$version"-SHA256SUMS.txt; do
        [ -f "$file" ] && say "  $(printf '%-28s' "$(basename "$file")") $(stat -c %s "$file") bytes"
    done
fi
say ""
say "Not run here: GamerTCP.exe selftest, which needs Windows."
