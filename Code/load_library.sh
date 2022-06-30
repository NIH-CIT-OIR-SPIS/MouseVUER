#!/bin/bash
USER_HOME=$(getent passwd $SUDO_USER | cut -d: -f6)
DEST="${USER_HOME}/ffmpeg_sources/ffmpeg"
#echo "${DEST}"
LIBLIST=("/usr/local/lib/" "$DEST/libavcodec/" "${DEST}/libavdevice/" "${DEST}/libavfilter/" "${DEST}/libavformat/" "${DEST}/libavutil/" "${DEST}/libpostproc/" "${DEST}/libswresample/" "${DEST}/libswscale/")
CONFILE="/etc/ld.so.conf"
#CONFILE="${HOME}/GH.conf"

for str in ${LIBLIST[@]};
do
    if [ -d "$str" ] &&  ! grep -q $str "$CONFILE"; then
        echo $str >> ${CONFILE}
        echo "SUCCESSFULLY ADDED $str LIBRARY PATH TO $CONFILE"
    else
        echo "EITHER DIR $str DOES NOT EXIST, OR $CONFILE already contains $str, ABORTING"
    fi
done

