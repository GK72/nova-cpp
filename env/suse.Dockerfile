FROM opensuse/leap:15

RUN --mount=type=bind,from=package,target=/tmp/context \
    --mount=type=cache,target=/var/cache/zypp \
    zypper install \
        --no-confirm \
        --allow-unsigned-rpm \
            /tmp/context/nova-*-Linux.rpm
