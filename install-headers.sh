#!/bin/sh

install -d "${DESTDIR}/${MESON_INSTALL_PREFIX}/include/usbg/function"
install -m 0644 "${MESON_BUILD_ROOT}/usbg_version.h" "${DESTDIR}/${MESON_INSTALL_PREFIX}/include/usbg"
install -m 0644 "${MESON_SOURCE_ROOT}/include/usbg/usbg.h" "${DESTDIR}/${MESON_INSTALL_PREFIX}/include/usbg"
install -m 0644 "${MESON_SOURCE_ROOT}/include/usbg/function/"* "${DESTDIR}/${MESON_INSTALL_PREFIX}/include/usbg/function"
