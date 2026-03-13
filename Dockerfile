FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    nano \
    curl \
    g++ \
    libpq-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN g++ -std=c++20 -O2 -I. -I/usr/include/postgresql \
    main.cpp handlers.cpp json_utils.cpp polls.cpp db.cpp core/core.cpp \
    -o blind_server -lpq

EXPOSE 8080

CMD ["./blind_server"]
