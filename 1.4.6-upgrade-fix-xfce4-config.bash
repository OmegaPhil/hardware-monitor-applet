#!/bin/bash

# This script updates references to this plugin of 'hardware-monitor' with 'xfce4-hardware-monitor-plugin' to be ran after the v1.4.6 upgrade

# Rename plugin configuration files
OIFS="$IFS"
IFS=$'\n'
for filename in $(ls -1 ~/.config/xfce4/panel)
do
    [[ $filename =~ ^hardware-monitor-([[:digit:]]+)\.rc$ ]] && {
        mv ~/".config/xfce4/panel/$filename" ~/".config/xfce4/panel/xfce4-hardware-monitor-plugin-${BASH_REMATCH[1]}.rc" || echo "Unable to rename '$filename' plugin configuration file!" >&2
    }
done
IFS="$OIFS"

# Update panel config
echo "Creating panel config backup at '~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml.bak'..."
cp ~/".config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml" ~/".config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml.bak" || {
    echo "Backup creation failed!" >&2
    exit 1
}
sed 's/value="hardware-monitor"/value="xfce4-hardware-monitor-plugin"/g' ~/".config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml" > ~/".config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml.modified" || {
    echo "Unable to update panel config!" >&2
    exit 1
}
mv -f ~/".config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml.modified" ~/".config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml" || {
    echo "Unable to replace old panel config with new one!" >&2
    exit 1
}
echo "XFCE4 configuration has been updated - please log out now and back in again - once you've confirmed the change has worked, please delete the backup file '~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml.bak'"
