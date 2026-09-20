"""Run build tools with case-insensitively unique Windows environment names."""
import os
import subprocess
import sys

env = {name.upper(): value for name, value in os.environ.items()}
raise SystemExit(subprocess.call(sys.argv[1:], env=env))
