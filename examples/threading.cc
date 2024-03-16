#include "nova/threading.h"
#include "nova/types.h"

int main() {
    using namespace std::chrono_literals;
    nova::context ctx;
    nova::thread t1("ttt", ctx, [] (std::stop_token st) { while (not st.stop_requested()) { std::this_thread::sleep_for(1s); } });

    std::this_thread::sleep_for(3s);

    return EXIT_SUCCESS;
}
