#!/bin/bash
VERSION=$(head -n1 debian/changelog | awk '{ gsub(/[\)\(]+/, "") ; print $2 }')

echo irccmd version: $VERSION

if [ -z $VERSION ]
then
    echo "bailing; version not set"
    exit 1
fi

echo "building irccmd for amd64 @ lucid"
echo '!hudson'" build irccmd ARCH=amd64 DIST=lucid " | irccmd --name=buildbot -d --channel=#dev | grep -l "Project irccmd build"
wget -q http://hudson.incas3.nl:8080/job/irccmd/ws/hudson-output/irccmd_"$VERSION"_amd64.deb
wget -q http://hudson.incas3.nl:8080/job/irccmd/ws/hudson-output/irccmd_"$VERSION"_amd64.changes
wget -q http://hudson.incas3.nl:8080/job/irccmd/ws/hudson-output/irccmd_"$VERSION"_.dsc
wget -q http://hudson.incas3.nl:8080/job/irccmd/ws/hudson-output/irccmd_"$VERSION"_.tar.gz

echo "building irccmd for i386 @ lucid"
echo '!hudson'" build irccmd ARCH=i386 DIST=lucid " | irccmd --name=buildbot -d --channel=#dev | grep -l "Project irccmd build"
wget -q http://hudson.incas3.nl:8080/job/irccmd/ws/hudson-output/irccmd_"$VERSION"_i386.deb
wget -q http://hudson.incas3.nl:8080/job/irccmd/ws/hudson-output/irccmd_"$VERSION"_i386.changes

echo Uploading packages
dput -u irccmd_$VERSION_*.changes
rm irccmd_$VERSION_*.changes
rm irccmd_$VERSION_*.deb
rm irccmd_$VERSION_*.dsc
rm irccmd_$VERSION_*.tar.gz
rm irccmd_$VERSION_*.upload

