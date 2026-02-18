#!/usr/bin/env python3
import sys
import os

def main():
    sys.stdout.write("Content-Type: text/plain\r\n\r\n")
    
    input_data = sys.stdin.read()
    
    sys.stdout.write("=== INPUT DATA ===\n")
    sys.stdout.write(input_data)
    sys.stdout.write("\n\n")
    
    sys.stdout.write("=== ENVIRONMENT VARIABLES ===\n")
    for key, value in sorted(os.environ.items()):
        sys.stdout.write(f"{key}={value}\n")
    
    sys.stdout.flush()

if __name__ == "__main__":
    main()
