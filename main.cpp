#include "handlers.h"

#include <iostream>

int main() {
    httplib::Server server;
    RegisterHandlers(server);

    constexpr int kPort = 8080;
    std::cout << "Blind signature voting server is running on http://0.0.0.0:" << kPort << '\n';
    std::cout << "Health check: GET /health\n";
    std::cout << "Create poll: POST /polls/create\n";

    server.listen("0.0.0.0", kPort);
    return 0;
}
