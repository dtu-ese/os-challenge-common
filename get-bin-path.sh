#!/bin/sh

ARCH=`uname -m`
OS=`uname -o`

if [ "$ARCH" = "aarch64" ]; then
	ARCH="arm64"
fi
if [ "$OS" = "GNU/Linux" ]; then
	OS="linux"
fi
if [ "$OS" = "Darwin" ]; then
	OS="macos"
fi

echo "$ARCH/bin/$OS"

