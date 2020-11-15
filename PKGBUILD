# Maintainer: Onni R. <onnir at iki dot fi>
pkgname=cursetag
pkgver=5
pkgrel=1
license=('None')
pkgdesc="an ncurses based audio metadata (tag) editor"
arch=('i686' 'x86_64')
url="https://github.com/lotuskip/cursetag"
depends=('taglib' 'ncurses')
source=("git+$url")
md5sums=('SKIP')

pkgver() {
    cd "$pkgname"
    printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

prepare() {
  sed -i "s|VERSION=.*$|VERSION=\"\\\\\"${pkgver}\\\\\"\"|" "${srcdir}/${pkgname}/Makefile"

  # sed -i 's|ncursesw/ncurses.h|ncurses.h|' src/io.h
  sed -i '/[^#]/ s/\(^CPPFLAGS += -DCOMPLICATED_CURSES_HEADER.*$\)/#\1/' "${srcdir}/${pkgname}/Makefile"
}
build() {
  cd $srcdir/$pkgname
  make
}

package() {
  mkdir -p "${pkgdir}/usr/bin"
  mkdir -p "${pkgdir}/usr/share/man/man1"
  cd $srcdir/$pkgname
  mv -f cursetag ${pkgdir}/usr/bin
  mv -f cursetag.1 ${pkgdir}/usr/share/man/man1/
}
