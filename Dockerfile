# Единая среда: GCC + make. Сборка консоли (main.c переносимый) + юнит-тесты.
FROM gcc:13-bookworm

RUN apt-get update \
    && apt-get install -y --no-install-recommends make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN make clean && make all && ./build/test.exe

CMD ["./build/test.exe"]
