#!/bin/bash
VERSION=$(head -n1 debian/changelog | awk '{ gsub(/[\)\(]+/, "") ; print $2 }')

echo irccmd version: $VERSION

if [ -z $VERSION ]
then
    echo "bailing; version not set"
    exit 1
fi

echo '!hudson'" build irccmd ARCH=amd64 DIST=lucid now" | irccmd --name=buildbot --channel=#dev | grep -m0 "Project irccmd build" > /dev/null
wget -q http://hudson.incas3.nl:8080/job/irccmd/ws/hudson-output/irccmd_"$VERSION"_amd64.deb     || :
wget -q http://hudson.incas3.nl:8080/job/irccmd/ws/hudson-output/irccmd_"$VERSION"_amd64.changes || :

echo '!hudson'" build irccmd ARCH=i386 DIST=lucid now" | irccmd --name=buildbot --channel=#dev | grep -m0 "Project irccmd build" > /dev/null
wget -q http://hudson.incas3.nl:8080/job/irccmd/ws/hudson-output/irccmd_"$VERSION"_i386.deb     || :
wget -q http://hudson.incas3.nl:8080/job/irccmd/ws/hudson-output/irccmd_"$VERSION"_i386.changes || :

dput -u irccmd_$VERSION_*.changes
rm irccmd_$VERSION_*.changes
rm irccmd_$VERSION_*.deb
rm irccmd_$VERSION_*.upload

