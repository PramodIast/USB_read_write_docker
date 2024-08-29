# Base image for ARM64 architecture
FROM arm64v8/ubuntu:20.04

# Set up environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    dosfstools \
    udisks2 \
    && rm -rf /var/lib/apt/lists/*

# Create a directory for the application
WORKDIR /usr/src/app

# Copy the C++ source code
COPY usb_file_operations.cpp .

# Compile the C++ program
RUN g++ -o usb_file_operations usb_file_operations.cpp

# Run the C++ program
CMD ["./usb_file_operations"]
