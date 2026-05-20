#FROM python:3.10-slim-bookworm
FROM python:3.9.21-alpine3.21

# Install OpenSSH
RUN apk add --no-cache openssh
# Set the working directory
WORKDIR /app

COPY src /app/src

# Copy the requirements file first
COPY requirements.txt /app/requirements/requirements.txt

# Install dependencies if requirements.txt is present
RUN if [ -f /app/requirements/requirements.txt ]; then \
        pip install --no-cache-dir -r /app/requirements/requirements.txt; \
    fi

# Copy the application code
COPY . .

# Create a named volume for the requirements file
VOLUME /app/requirements

# Create a named volume for the installed modules
VOLUME /app/venv

# Set the default serial port
#ENV ENV_SERIAL_PORT=/dev/ttyUSB0
ENV ENV_BIN_FOLDER=/app/src/
ENV ENV_BIN_FILE=uart_amc.bin

# Exclude __pycache__ from being copied into the Docker image
COPY .dockerignore /app/.dockerignore

# Copy the application code
COPY . .

# Command to run the application
CMD ["python", "src/main.py"]