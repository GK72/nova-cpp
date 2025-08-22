export COLOR_DEF='\033[0m'
export COLOR_BLUE='\033[0;34m'
export COLOR_RED='\033[1;31m'
export COLOR_GREY='\033[0;35m'

function log() {
    >&2 echo -e "[$(date '+%F %T.%6N %:z')] [run @$$] $*"
}

function log-info() {
    >&2 echo -e "${COLOR_DEF}[$(date '+%F %T.%6N %:z')] [run @$$] ${COLOR_BLUE}$*${COLOR_DEF}"
}

function log-error() {
    >&2 echo -e "${COLOR_DEF}[$(date '+%F %T.%6N %:z')] [run @$$] ${COLOR_RED}$*${COLOR_DEF}"
}
