#!/usr/bin/env python

# INTEL CONFIDENTIAL
# Copyright 2025 Intel Corporation
#
# This software and the related documents are Intel copyrighted materials,
# and your use of them is governed by the express license under which they
# were provided to you (License). Unless the License provides otherwise,
# you may not use, modify, copy, publish, distribute, disclose or
# transmit this software or the related documents without
# Intel's prior written permission.
#
# This software and the related documents are provided as is,
# with no express or implied warranties, other than those
# that are expressly stated in the License.

import sys
import argparse
import struct
import binascii
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend

# Constants for secure boot header
AST1030A1_HEADER_OFFSET = 0x400
AST_1030A1_ROT_HEADER_FORMAT = '<8I'
#As per flash layout the loader signature is at 0xFE00
MAX_BL1_IMAGE_SIZE = 65024  #((64*1024)-512))
ALIGNMENT_MASK = 511

# Define the size of r and s (48 bytes each for ECDSA SHA-384)
ECDSA_SHA384_KEY_HASH_SIZE = 48

# Define a debug flag, defaulting to False
DEBUG = False

def debug_print(message):
    """Only prints debug messages if DEBUG flag is True."""
    if DEBUG:
        print(message)

def BIT(off):
    return 1 << off

def insert_bytearray(src, dst, offset):
    if offset + len(src) > len(dst):
        dst.extend(bytearray(offset - len(dst) + len(src)))
    dst[offset:offset + len(src)] = src

### function to create secure boot header ###
def prepare_sb_header(input_file, output_file, revision_id):
    """
    Prepares a secure boot header for the AST 1030 image.

    Args:
        input_file (str): Path to the input binary file.
        output_file (str): Path to save the output file with the secure boot header.
        revision_id (int): Revision ID (0-64) for the secure boot header.

    Raises:
        ValueError: If the input file is empty or revision_id is out of range.
        Exception: For other unexpected errors.
    """
    with open(input_file, 'rb') as loader_image_fd:
        loader_image = bytearray(loader_image_fd.read())
        loader_image_len = len(loader_image)

        # Print the image and its length
        debug_print(f"loader_image_len = {loader_image_len}")

        if loader_image_len == 0:
            raise ValueError("Input file is empty.")

        if loader_image_len > MAX_BL1_IMAGE_SIZE:
            raise ValueError("The maximum size of BL1 image is 63 KBytes.")

        # Variables for different offsets and checksums
        flash_patch_offset = 0
        aes_data_offset = 0
        sign_image_size = 0
        signature_offset = 0
        revision_low = 0
        revision_high = 0
        loader_header_checksum = 0
        sign_image_size = (loader_image_len + ALIGNMENT_MASK) & (~ALIGNMENT_MASK)
        enc_offset = 0
        signature_offset = sign_image_size

        if revision_id < 0 or revision_id > 64:
            raise ValueError("revision_id is out of range. (0 <= revision_id <= 64)")

        # Generate the revision low and high based on the revision_id
        for i in range(revision_id):
            if i < 32:
                revision_low |= BIT(i)
            else:
                j = i - 32
                revision_high |= BIT(j)

        # Calculate the loader header checksum
        loader_header_checksum = -(aes_data_offset + enc_offset +
                                   sign_image_size + signature_offset +
                                   revision_low + revision_high +
                                   flash_patch_offset) & 0xFFFFFFFF
        
        # Pack the header
        loader_header = struct.pack(
            AST_1030A1_ROT_HEADER_FORMAT,
            aes_data_offset,
            enc_offset,
            sign_image_size,
            signature_offset,
            revision_low,
            revision_high,
            flash_patch_offset,
            loader_header_checksum
        )

        loader_image.extend(bytearray(sign_image_size - loader_image_len))
        output_image = bytearray(sign_image_size)

        insert_bytearray(loader_image, output_image, 0)
        insert_bytearray(loader_header, output_image, AST1030A1_HEADER_OFFSET)

        debug_print(f"sign_image_size = {sign_image_size}")
        debug_print(f"loader_header = {loader_header}")
        debug_print(f"loader_header_len = {len(loader_header)}")
        debug_print(f"aes_data_offset = {aes_data_offset}")
        debug_print(f"signature_offset = {signature_offset}")
        debug_print(f"flash_patch_offset = {flash_patch_offset}")
        debug_print(f"loader_header_checksum = {hex(loader_header_checksum)}")

        try:
            with open(output_file, 'wb') as out_fd:
                out_fd.write(output_image)
        except PermissionError as e:
            raise PermissionError(f"Cannot write to output file '{output_file}': {e}")

def reverse_bytes(data):
    """
    Reverse the byte order of the input data.
    """
    return data[::-1]

### function to change the signature endianness  ###
def change_sign_endianess(input_file):
    """
    Change the byte order (endianness) of the signature in the given binary file.
    The file is modified in-place by reversing the byte order of r and s.
    """
    # Read the binary file
    with open(input_file, "rb") as f:
        data = f.read()

    # Ensure the file contains enough data
    if len(data) < ECDSA_SHA384_KEY_HASH_SIZE * 2:
        raise ValueError("Input file does not contain enough data for r and s.")

    # Extract r and s
    r = data[:ECDSA_SHA384_KEY_HASH_SIZE]
    s = data[ECDSA_SHA384_KEY_HASH_SIZE:ECDSA_SHA384_KEY_HASH_SIZE * 2]

    # Reverse the byte order of r and s
    r_swapped = reverse_bytes(r)
    s_swapped = reverse_bytes(s)

    # Concatenate the swapped r and s with the rest of the file (if any)
    swapped_data = r_swapped + s_swapped + data[ECDSA_SHA384_KEY_HASH_SIZE * 2:]

    # Overwrite the input file with the swapped data
    with open(input_file, "wb") as f:
        f.write(swapped_data)

    print(f"Signature byte order swapped and saved to {input_file}")
    
# Extract Qx and Qy values from the input data
def extract_qx_qy(data):
    qx_start = data.find("qx48:") + len("qx48:")
    qy_start = data.find("qy48:") + len("qy48:")
    qx = data[qx_start:qx_start + ECDSA_SHA384_KEY_HASH_SIZE]
    qy = data[qy_start:qy_start + ECDSA_SHA384_KEY_HASH_SIZE]
    return qx, qy

# Convert to binary, reverse byte order, and save to file
def save_to_bin_file(qx, qy, filename):
    # Convert the hexadecimal strings to bytes
    qx_bytes = binascii.unhexlify(qx.encode('latin1').hex())
    qy_bytes = binascii.unhexlify(qy.encode('latin1').hex())
    
    # Reverse byte order
    qx_bytes = qx_bytes[::-1]
    qy_bytes = qy_bytes[::-1]
    
    # Write to the output file
    with open(filename, "wb") as f:
        f.write(qx_bytes)
        f.write(qy_bytes)

### function to read qx and qy components from public key ###
def read_pbkey_qxqy(input_file, output_file):
    """
    Extracts Qx and Qy values from the input file, reverses their byte order,
    and saves the binary data to the output file.
    
    Parameters:
    - input_file: Path to the input file containing Qx and Qy values in text format.
    - output_file: Path to the output file where the binary data will be saved.
    """
    with open(input_file, "rb") as f:
        data = f.read().decode('latin1')

    # Extract Qx and Qy
    qx, qy = extract_qx_qy(data)
    
    # Save Qx and Qy in reversed byte order to the binary file
    save_to_bin_file(qx, qy, output_file)
    
    print(f"Processed data saved to {output_file}")
    
def load_pem_private_key(pem_file_path):
    """
    Load the private key from a PEM file.
    
    Parameters:
    - pem_file_path: Path to the PEM file containing the private key.
    
    Returns:
    - The private key object.
    """
    with open(pem_file_path, "rb") as pem_file:
        private_key = serialization.load_pem_private_key(
            pem_file.read(),
            password=None,
            backend=default_backend()
        )
    return private_key

def serialize_to_custom_format(private_key):
    """
    Serialize the private key to a custom format.
    
    Parameters:
    - private_key: The private key object to serialize.
    
    Returns:
    - Custom format string representing the private and public key components.
    """
    private_numbers = private_key.private_numbers()
    public_numbers = private_numbers.public_numbers

    qx = public_numbers.x.to_bytes(ECDSA_SHA384_KEY_HASH_SIZE, byteorder='big')
    qy = public_numbers.y.to_bytes(ECDSA_SHA384_KEY_HASH_SIZE, byteorder='big')
    d = private_numbers.private_value.to_bytes(ECDSA_SHA384_KEY_HASH_SIZE, byteorder='big')

    # Serialize to custom format with binary data
    custom_format = (
        f"(8:sequence"
        f"(10:public-key(22:ecdsa-secp384r1-sha384"
        f"(2:qx48:{qx.decode('latin1')})"
        f"(2:qy48:{qy.decode('latin1')})))"
        f"(11:private-key(22:ecdsa-secp384r1-sha384"
        f"(1:d48:{d.decode('latin1')}))))"
    )
    return custom_format

### function to key from pem to kp format  ###
def format_key_pemtokp(pem_file_path, kp_file_path):
    """
    Converts the PEM private key to a custom format and saves it to the specified file.
    
    Parameters:
    - pem_file_path: Path to the input PEM file containing the private key.
    - kp_file_path: Path to the output file where the custom key format will be saved.
    """
    try:
        # Load the private key and convert it to the custom format
        private_key = load_pem_private_key(pem_file_path)
        custom_key_format = serialize_to_custom_format(private_key)

        # Save the custom key format to the specified KP file
        with open(kp_file_path, "w", encoding="latin1") as kp_file:
            kp_file.write(custom_key_format)
        print(f"Custom key format successfully saved to {kp_file_path}")
    except Exception as e:
        print(f"An error occurred: {e}")

def extract_ecdsa_qxqy_from_pem(pem_file_path):
    """Extracts and prints Qx and Qy from a PEM-encoded ECDSA-384 public key."""
    with open(pem_file_path, 'rb') as pem_file:
        pem_data = pem_file.read()

    public_key = serialization.load_pem_public_key(pem_data)

    if not isinstance(public_key, ec.EllipticCurvePublicKey):
        raise ValueError("The provided public key is not an ECDSA key.")

    public_numbers = public_key.public_numbers()
    qx = public_numbers.x
    qy = public_numbers.y

    # Print the values directly
    print(f"Qx: {qx:0{96}x}")
    print(f"Qy: {qy:0{96}x}")
