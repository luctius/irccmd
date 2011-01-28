sudo dpkg-buildpackage -S

if [ ! -e /var/cache/pbuilder/lucid-amd64-base.tgz  ]
then
    sudo ARCH=i386 DIST=lucid pbuilder create
fi

if [ ! -e /var/cache/pbuilder/lucid-amd64-base.tgz  ]
then
    sudo ARCH=amd64 DIST=lucid pbuilder create
fi

if [ ! -e /var/cache/pbuilder/maverick-amd64-base.tgz  ]
then
    sudo ARCH=i386 DIST=maverick pbuilder create
fi

if [ ! -e /var/cache/pbuilder/maverick-amd64-base.tgz  ]
then
    sudo ARCH=amd64 DIST=maverick pbuilder create
fi

mkdir -p packages/lucid
mkdir -p packages/maverick

sudo ARCH=i386 DIST=lucid pbuilder update
sudo ARCH=i386 DIST=lucid CPPFLAGS_APPEND=-I/usr/include/libircclient/ pbuilder build --debbuildopts "-b" ../irccmd*.dsc
sudo mv /var/cache/pbuilder/lucid-i386-i386/result/irccmd* ./packages/lucid

sudo ARCH=amd64 DIST=lucid pbuilder update
sudo ARCH=amd64 DIST=lucid CPPFLAGS_APPEND=-I/usr/include/libircclient/ pbuilder build --debbuildopts "-b" ../irccmd*.dsc
sudo mv /var/cache/pbuilder/lucid-amd64-amd64/result/irccmd* ./packages/lucid

sudo ARCH=i386 DIST=maverick pbuilder update
sudo ARCH=i386 DIST=maverick CPPFLAGS_APPEND=-I/usr/include/libircclient/ pbuilder build --debbuildopts "-b" ../irccmd*.dsc
sudo mv /var/cache/pbuilder/maverick-i386-i386/result/irccmd* ./packages/maverick

sudo ARCH=amd64 DIST=maverick pbuilder update
sudo ARCH=amd64 DIST=maverick CPPFLAGS_APPEND=-I/usr/include/libircclient/ pbuilder build --debbuildopts "-b" ../irccmd*.dsc
sudo mv /var/cache/pbuilder/maverick-amd64-amd64/result/irccmd* ./packages/maverick

