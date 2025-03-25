#include <unistd.h>

namespace ervan::smtp {
    int main_async() {
        while (true)
            sleep(1);

        return 0;
    }
}