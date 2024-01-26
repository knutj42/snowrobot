"""
This script is used by CTest instead of just running "python -m unittest", since that will not complain if it
doesn't find any tests. As an extra safety check this script will raise an error if the expected number of tests
are not found. 
"""
MIN_EXPECTED_NUMBER_OF_TESTCASES = 1

import io
import unittest
import os
import sys


if __name__ == "__main__":
    source_dir = os.environ["CMAKE_CURRENT_SOURCE_DIR"]
    binary_dir = os.environ["CMAKE_BINARY_DIR"]
    
    suite = unittest.defaultTestLoader.discover(source_dir)

    if suite.countTestCases() < MIN_EXPECTED_NUMBER_OF_TESTCASES:
        raise AssertionError(f"Found only {suite.countTestCases()} testcases, " 
                             f"but I expected to find at least {MIN_EXPECTED_NUMBER_OF_TESTCASES}")

    stream = io.StringIO()
    runner = unittest.TextTestRunner()
    result = runner.run(suite)
    stream.seek(0)
    print(stream.read())
    if not result.wasSuccessful():
        sys.exit(1)
