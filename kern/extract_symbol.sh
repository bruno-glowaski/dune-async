#!/bin/sh

SYM=$1
TARGET="$2"
KVER=$(uname -r)
if [ -z $TARGET ]; then
  TARGET="/boot/System.map-$KVER"
  if [ -f $TARGET ]; then
    TARGET=/lib/modules/$KVER/build/System.map
    if [ -f $TARGET ]; then
      TARGET=/lib/modules/$KVER/build/vmlinux
      if [ -f $TARGET ]; then
        printf "Error: couldn't find a target to extract symbols from. Try providing a path to a System.map or vmlinux file after the symbol name." &>2
        exit 1
      fi
    fi
  fi
fi

case "$(file $TARGET | cut -d" " -f2)" in
  "ELF")
  # vmlinux 
  nm $TARGET | egrep " $SYM\$" | egrep " (R|T) " | cut -d" " -f1
  ;;
  "ASCII")
  # System.map
	cat $TARGET | egrep " $SYM\$" | cut -d" " -f1
  ;;
  *) echo default
    printf "Error: target is not System.map nor vmlinux." &>2
  ;;
esac

