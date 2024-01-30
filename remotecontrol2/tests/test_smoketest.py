import unittest

import os
import time

from utils import IntegrationTestBase

class SmokeTest(IntegrationTestBase):
    maxDiff = None

    def test_smoketest(self):
        reply = self.server_connection.send_message("ping")
        self.assertEqual(reply, "pong") 

        reply = self.client_connection.send_message("ping")
        self.assertEqual(reply, "pong")

        starttime = time.monotonic()
        while True:
            is_server_connected_request = "is server connected"
            reply = self.client_connection.send_message(is_server_connected_request)
            if reply == "true":
                break
            elapsed_time = time.monotonic() - starttime
            if elapsed_time > 60:
                raise AssertionError(
                    f"Timed out while waiting for the client to connect to the server. Last reply to the "
                    f"'{is_server_connected_request}' request was '{reply}'")



if __name__ == "__main__":
    unittest.main()
