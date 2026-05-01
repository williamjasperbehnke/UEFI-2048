FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gnu-efi \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work
CMD ["make"]
