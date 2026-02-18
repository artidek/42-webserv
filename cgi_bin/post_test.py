import sys

def main():
    input_data = sys.stdin.read()
    
    # Send CGI headers
    sys.stdout.write("Content-Type: text/plain\r\n\r\n")
    
    sys.stdout.write(input_data)
    sys.stdout.flush()

if __name__ == "__main__":
    main()
