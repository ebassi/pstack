mkdir -p m4 || exit $?
autoreconf -fi || exit $?
# Gnome uses NOCONFIGURE
test -n "$NOCONFIGURE" || "./configure" "$@"
