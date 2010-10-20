# Maintainer: Onni R. <onnir at iki dot fi>
pkgname=cursetag
pkgver=1
pkgrel=1
license=('None')
pkgdesc="an ncurses based audio metadata (tag) editor"
arch=('i686' 'x86_64')
url="http://github.com/lotuskip/cursetag"
makedepends=('boost')
depends=('boost-libs' 'ncurses')
source=(http://tempoanon.net/lotuskip/tervat/$pkgname-$pkgver.tar.gz)
md5sums=('bb69f2d6bb4a85e7aa7ad9cc7e62d4f9')

build() {
  cd $srcdir/$pkgname
  aclocal || return 1
  automake --add-missing || return 1
  autoconf || return 1
  ./configure --prefix="$pkgdir/usr" || return 1
  make || return 1
}

package() {
	make install || return 1
}
