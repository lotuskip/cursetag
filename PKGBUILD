# Maintainer: Onni R. <onnir at iki dot fi>
pkgname=cursetag
pkgver=3
pkgrel=1
license=('None')
pkgdesc="an ncurses based audio metadata (tag) editor"
arch=('i686' 'x86_64')
url="http://github.com/lotuskip/cursetag"
depends=('taglib' 'ncurses')
source=(http://github.com/downloads/lotuskip/cursetag/$pkgname-$pkgver.tar.gz)
md5sums=('125497e5f462d48dd955640d5a9d6e0d')

build() {
  cd $srcdir/$pkgname
  make || return 1
}

package() {
  cd $srcdir/$pkgname
  mv -f cursetag ${pkgdir}/usr/bin
  mv -f cursetag.1 ${pkgdir}/usr/share/man/man1/
}
