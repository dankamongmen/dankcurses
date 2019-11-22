* Update version in CMakeLists.txt
* Update Debian changelog with `dch -v $VERSION-1`
* git commit -a -m v$VERSION
* Tag with `git tag -a v$VERSION -m "v$VERSION"`
* `git push && git push origin --tags`
* Draft new release at https://github.com/dankamongmen/outcurses/releases
  * Title is "v$VERSION—some quip"
  * That's an em dash (U+2014, UTF-8 e2 80 94), get it right
* Build new Debian package
  * git clean -d -f -x
  * `tar -cJf ../outcurses_$VERSION.orig.tar.xz --exclude=.git --exclude=debian -C.. outcurses-$VERSION`
  * debuild
* Copy `../*outcurses*$VERSION*` to apt repo, import with `reprepro`
* Upload new AUR information
  * Update `pkgver` and `sha256sums` entries
  * `makepkg --printsrcinfo > .SRCINFO`
  * Test that package builds with `makepkg`
  * `git commit -a`
