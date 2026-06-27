# --- STAGE 1: Builder ---
# In CI, we will build 'builder.Dockerfile' and tag it as 'expense-bot-builder'
# Then we start FROM it here.
ARG BUILDER_IMAGE=expense-bot-builder:latest
FROM ${BUILDER_IMAGE} AS compiler

WORKDIR /app
COPY . .

# Build the project (Fast! Dependencies are already in /opt/vcpkg)
RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
RUN cmake --build build --config Release -j $(nproc)

# --- STAGE 2: Runner ---
FROM ubuntu:22.04 AS runner
RUN apt-get update && apt-get install -y \
    libssl3 libpq5 ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=compiler /app/build/expense_bot .

RUN useradd -m botuser
USER botuser

ENTRYPOINT ["./expense_bot"]
