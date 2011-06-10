# Maintainer: Onni R. <onnir at iki dot fi>
pkgname=cursetag
pkgver=3
pkgrel=1
license=('None')
pkgdesc="an ncurses based audio metadata (tag) editor"
arch=('i686' 'x86_64')
url="http://github.com/lotuskip/cursetag"
depends=('taglib' 'ncurses')
source=(http://github.com/lotuskip/cursetag/raw/master/$pkgname-$pkgver.tar.gz)
md5sums=('16657f414cb4460226906546d681ce04')

build() {
  cd $srcdir/$pkgname
  aclocal || return 1
  automake --add-missing || return 1
  autoconf || return 1
  ./configure --prefix=/usr || return 1
  make || return 1
}

package() {
  cd $srcdir/$pkgname
  make DESTDIR=${pkgdir} install || return 1
}
