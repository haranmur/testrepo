import sys
import os

def setup_environment():
    # Add the src directory to the system path
    sys.path.append(os.path.join(os.path.dirname(__file__), 'src'))

if __name__ == "__main__":
    setup_environment()
