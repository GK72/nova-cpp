source "scripts/log.lib.sh"

function scroll-term() {
    local lines_shown=$1

    if (( lines_shown > 0 )); then
        tput cuu "${lines_shown}"
        for ((i=0; i<lines_shown; i++)); do
            tput el
            tput cud1   # move cursor down 1
        done
        tput cuu "${lines_shown}"
    fi
}

function draw-tail() {
    local lines_shown=$1
    shift
    set +e
    local output=("$@")
    set -e

    scroll-term "${lines_shown}"
    for line in "${output[@]}"; do
        printf "  ${COLOR_GREY}%s\n" "${line}${COLOR_DEF}"
    done
}

function live-logger() {
    local logfile="${1}"
    local statefile="${2}"

    lines_shown=0
    while true; do
        output=()
        while IFS= read -r line; do
            output+=("$line")
        done < <(tail -n 20 "$logfile")     # TODO: Parameterize tail lines.

        draw-tail "${lines_shown}" "${output[@]}"
        lines_shown=${#output[@]}
        echo "$lines_shown" > "${statefile}"

        sleep 0.1
    done
}

function live-tail() {
    local cmd=("$@")
    local tmp_prefix
    tmp_prefix=$(mktemp --dry-run)
    # TODO: Nicely check if the path is valid
    local logfile="${tmp_prefix}.log"
    local statefile="${tmp_prefix}.state"

    "${cmd[@]}" > "${logfile}" 2>&1 &
    local pid_cmd=$!

    live-logger "${logfile}" "${statefile}" &
    sleep 0.5
    local tail_pid=$!
    echo "Tail pid: ${tail_pid}" >> "${tmp_prefix}.debug"

    trap 'kill "$tail_pid" 2>/dev/null || true' EXIT INT TERM

    echo "Wait for cmd: ${pid_cmd}" >> "${tmp_prefix}.debug"
    wait "$pid_cmd"
    exit_code=$?
    echo "Exit code of cmd: $exit_code" >> "${tmp_prefix}.debug"

    echo "${pid_cmd} finished" >> "${tmp_prefix}.debug"

    kill "$tail_pid" 2>/dev/null || true
    wait "$tail_pid" 2>/dev/null || true
    trap - EXIT

    lines_shown=$(<"${statefile}")
    scroll-term "${lines_shown}"

    if [[ $exit_code -eq 0 ]]; then
        log-info "Command succeeded: ${cmd[*]}"
    else
        cat "${logfile}"
        log-error "Command failed: ${cmd[*]} (exit code ${exit_code})"
    fi

    # rm -f "${logfile}" "${statefile}"
    return "$exit_code"
}
