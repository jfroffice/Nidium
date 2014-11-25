#!/bin/bash

IS_ROOT=0
BASE_INSTALL_DIR="${HOME}/nidium"
if [ "$(id -u)" = "0" ]
then
    IS_ROOT=1
    BASE_INSTALL_DIR="/opt/nidium"
else
    echo "------------"
    echo "NOTE : You are running this script as \"$(whoami)\". Nidium will only be installed for this user."
    echo "If you want to install nidium system wide, run this script with administrator privileges."
    echo "------------"
fi

xdg-mime --version >/dev/null 2>&1
if [ $? != 0 ]
then
    echo "Nidium installer need the package xdg-utils. Install this package first."
    exit 1
fi

read -p "Where do you want to install nidium (leave blank for installing in $BASE_INSTALL_DIR) : " INSTALL_DIR

if [ "$INSTALL_DIR" = "" ] 
then
    INSTALL_DIR=$BASE_INSTALL_DIR
fi
INSTALL_DIR=`readlink -f $INSTALL_DIR`

echo 
echo -n "Before installing Nidium, you must agree with Nidium Software Licence Agreement"
i=1
while [ $i -le 4 ]
do
    sleep 0.6
    echo -n "."
    i=$((i+1))
done

less ./COPYING

read -p "Do you agree with Nidium licence terms ? (y/n) " -n 1 -r
echo    #  move to a new line
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    echo "You must agree with Nidium licence terms to install Nidium"
    exit 2
fi


echo "- Creating install dir ($INSTALL_DIR)"
mkdir -p $INSTALL_DIR/

if [ $? != 0 ]
then
    echo "Failed to create install directory : $INSTALL_DIR"
    exit 2
fi

echo "- Copying nidium files to $INSTALL_DIR"
cp dist/* $INSTALL_DIR/
if [ $? != 0 ]
then
    echo "Failed to copy nidium files to : $INSTALL_DIR"
    exit 2
fi

chmod +rx $INSTALL_DIR/nidium
chmod +rx $INSTALL_DIR/nidium-crash-reporter

printf "%s" "- Registering Nidium file association : *.nml;*.npa "

echo "Exec=$INSTALL_DIR/nidium %F" >> resources/nidium.desktop

if [ $IS_ROOT = 1 ]
then
    printf "(System wide)\n"
    xdg-mime install --novendor --mode system resources/x-application-nidium.xml
    cp resources/nidium.desktop /usr/local/share/applications/
    update-desktop-database /usr/local/share/applications/
    xdg-icon-resource install --mode system --novendor --context mimetypes --size 48 resources/nidium.png x-application-nidium
else
    printf "(Current user only)\n"
    xdg-mime install --novendor resources/x-application-nidium.xml
    mkdir -p ~/.local/share/applications/
    cp resources/nidium.desktop ~/.local/share/applications/
    update-desktop-database ~/.local/share/applications/
    xdg-icon-resource install --novendor --context mimetypes --size 48 resources/nidium.png x-application-nidium
fi

xdg-mime default nidium.desktop application/nidium

echo "Nidium is now installed. Have fun!"
